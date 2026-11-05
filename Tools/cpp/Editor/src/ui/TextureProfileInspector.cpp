#include <EditorPCH.hpp>
#include <ui/TextureProfileInspector.hpp>

#if WP_EDITOR_TEXTURE_PROFILE
#    include <Workphone/Workphone.hpp>
#    include <WPGraphics/ClawTexture.hpp>
#    include <editor/EditorManager.hpp>
#    include <editor/Project.hpp>

namespace workphone::editor
{
    namespace
    {
        constexpr u64 previewResultLimit = 256ull * 256 * 4 + 4096;
        const Array<String> targets{
            "pc", "windows-dx11-dds-bgra8-v1", "windows-dx11-dds-bgra8-srgb-v1",
            "windows-dx11-dds-bc1-v1", "windows-dx11-dds-bc1-srgb-v1",
            "windows-dx11-dds-bc3-v1", "windows-dx11-dds-bc3-srgb-v1", "Custom"
        };
        const Array<String> filters{ "none", "colour", "data", "normal", "cutout", "roughness" };
        const Array<String> compression{ "inherit", "none", "bc1", "bc3", "bc7", "astc", "pvrtc", "etc2" };
        const Array<String> channels{ "Colour", "Red", "Green", "Blue", "Alpha", "Compression error" };

        const char *formatName( u32 format )
        {
            switch( format )
            {
            case 71: return "BC1";
            case 72: return "BC1 sRGB";
            case 77: return "BC3";
            case 78: return "BC3 sRGB";
            case 87: return "BGRA8 DDS";
            case 91: return "BGRA8 sRGB DDS";
            case 98: return "BC7";
            case 99: return "BC7 sRGB";
            default: return "BGRA8 portable";
            }
        }

        s32 compressionIndex( const String &format )
        {
            const auto it = std::find( compression.begin(), compression.end(), format );
            return it == compression.end() ? 0 : s32( it - compression.begin() );
        }

        String profileSummary( const render::TextureProfileSettings &settings )
        {
            return filters[size_t( settings.mip.filter )] + ", " + compression[compressionIndex( settings.compression )] +
                   ", atlas " + StringUtil::toString( settings.mip.atlasColumns ) + ", cutoff " +
                   StringUtil::toString( settings.mip.alphaCutoff ) + ", max " +
                   ( settings.maximumSize ? StringUtil::toString( settings.maximumSize ) : String( "project" ) );
        }
    }

    TextureProfileInspector::TextureProfileInspector( SmartPtr<ui::IUIManager> ui,
        SmartPtr<ui::IUIWindow> parent, SmartPtr<IEventListener> listener )
    {
        m_grid = ui->addElementByType<ui::IUIPropertyGrid>();
        m_grid->setVisible( false, false );
        m_grid->addObjectListener( listener );
        parent->addChild( m_grid );
        m_image = ui->addElementByType<ui::IUIImage>();
        m_image->setVisible( false, false );
        parent->addChild( m_image );
    }

    TextureProfileInspector::~TextureProfileInspector()
    {
        // WorldStreamProducer destruction drains its own worker callbacks. Their
        // captures are immutable and never refer to the inspector or the device.
        m_jobs.reset();
        retireImage();
    }

    bool TextureProfileInspector::isTextureDescriptor( SmartPtr<ISharedObject> selection )
    {
        auto file = dynamic_pointer_cast<FileSelection>( selection );
        return file && StringUtil::make_lower( Path::getFileExtension( file->getFilePath() ) ) == ".texres";
    }

    bool TextureProfileInspector::update( SmartPtr<ISharedObject> selection )
    {
        auto app = core::IApplicationManager::instancePtr();
        auto editor = EditorManager::getSingletonPtr();
        SmartPtr<ISharedObject> identity = editor ? editor->getProject() : nullptr;
        const auto project = app ? app->getProjectPath() : String();
        const bool selected = !project.empty() && isTextureDescriptor( selection );
        const auto path = selected ? static_pointer_cast<FileSelection>( selection )->getFilePath() : String();
        if( path != m_path || project != m_project || identity != m_projectIdentity )
        {
            discardPreview();
            m_projectIdentity = identity;
            m_project = project;
            m_path = path;
            m_document = {};
            m_displayed = nullptr;
            m_grid->setProperties( nullptr );
            if( selected ) reload();
        }
        m_grid->setVisible( selected, false );
        if( selected ) takePreview();
        return selected;
    }

    void TextureProfileInspector::reload()
    {
        discardPreview();
        ++m_documentGeneration;
        m_document = {};
        const auto target = m_target == s32( targets.size() - 1 ) ? m_customTarget : targets[m_target];
        if( !render::openTextureProfileDocument( m_project, m_path, target, m_document, m_status ) )
        {
            refresh();
            return;
        }
        m_draft = m_document.effective;
        m_status = m_document.overridden ? "Saved target override loaded." : "Inheriting descriptor defaults.";
        refresh();
    }

    void TextureProfileInspector::refresh()
    {
        auto properties = make_ptr<Properties>();
        properties->setName( "Texture target profile " + StringUtil::toString( m_documentGeneration ) );
        properties->setPropertyAsEnum( "Target", m_target, targets );
        if( m_target == s32( targets.size() - 1 ) ) properties->setProperty( "Custom target name", m_customTarget );
        if( !m_document.original.empty() )
        {
            properties->setProperty( "Image source", m_document.source, true );
            properties->setProperty( "Descriptor defaults", profileSummary( m_document.defaults ), true );
            properties->setProperty( "Saved target settings", profileSummary( m_document.effective ), true );
            properties->setPropertyAsEnum( "Mip semantic", s32( m_draft.mip.filter ), filters );
            properties->setProperty( "Atlas columns", s32( m_draft.mip.atlasColumns ) );
            properties->setProperty( "Alpha cutoff", m_draft.mip.alphaCutoff );
            properties->setPropertyAsEnum( "Compression", compressionIndex( m_draft.compression ), compression );
            properties->setProperty( "Maximum size (0 = project)", s32( m_draft.maximumSize ) );
            properties->setProperty( "Preview mip (0 = largest)", m_mip );
            properties->setPropertyAsEnum( "Preview channel", m_channel, channels );
            properties->setButtonPressed( "Build preview", false );
            properties->setButtonPressed( "Save target override", false );
            properties->setButtonPressed( "Clear target override", false );
        }
        properties->setButtonPressed( "Reopen descriptor", false );
        properties->setProperty( "Texture profile status", m_status, true );
        properties->getPropertyObject( "Texture profile status" ).setAttribute( "valueTooltip", "true" );
        properties->getPropertyObject( "Texture profile status" ).setAttribute( "description", "Latest save or preview result." );
        m_grid->setProperties( properties );
        m_displayed = std::move( properties );
    }

    bool TextureProfileInspector::handle( SmartPtr<ISharedObject> sender, SmartPtr<Properties> displayed,
        const String &name, const String &value, bool isButton )
    {
        if( !ownsEvent( sender ) ) return false;
        if( !m_displayed || displayed->getName() != m_displayed->getName() || m_path.empty() ) return true;

        if( !isButton && name == "Target" )
        {
            m_target = std::clamp( StringUtil::parseInt( value ), 0, s32( targets.size() - 1 ) );
            reload();
        }
        else if( !isButton && name == "Custom target name" )
        {
            m_customTarget = value;
            reload();
        }
        else if( isButton && name == "Reopen descriptor" ) reload();
        else if( m_document.original.empty() ) return true;
        else if( isButton )
        {
            if( name == "Build preview" ) requestPreview();
            else if( name == "Save target override" || name == "Clear target override" )
            {
                const auto *settings = name == "Save target override" ? &m_draft : nullptr;
                String error;
                if( !render::saveTextureProfileOverride( m_document, settings, error ) ) setStatus( error );
                else
                {
                    reload();
                    if( !m_document.original.empty() )
                        setStatus( m_document.overridden ? "Target override saved. Next cook uses these settings."
                                                        : "Target inherits descriptor defaults. Next cook uses those settings." );
                }
            }
        }
        else
        {
            if( name == "Mip semantic" )
                m_draft.mip.filter = render::TextureMipFilter( std::clamp( StringUtil::parseInt( value ), 0, 5 ) );
            else if( name == "Atlas columns" )
                m_draft.mip.atlasColumns = u32( std::max( 0, StringUtil::parseInt( value ) ) );
            else if( name == "Alpha cutoff" ) m_draft.mip.alphaCutoff = StringUtil::parseFloat( value );
            else if( name == "Compression" )
            {
                const auto index = std::clamp( StringUtil::parseInt( value ), 0, s32( compression.size() - 1 ) );
                m_draft.compression = index ? compression[index] : String();
            }
            else if( name == "Maximum size (0 = project)" )
                m_draft.maximumSize = u32( StringUtil::parseInt( value ) );
            else if( name == "Preview mip (0 = largest)" ) m_mip = StringUtil::parseInt( value );
            else if( name == "Preview channel" )
                m_channel = std::clamp( StringUtil::parseInt( value ), 0, s32( channels.size() - 1 ) );
            else return true;
            discardPreview();
            setStatus( "Preview settings changed. Build preview to inspect the draft." );
        }
        return true;
    }

    bool TextureProfileInspector::ownsEvent( SmartPtr<ISharedObject> sender ) const
    {
        return sender == m_grid;
    }

    void TextureProfileInspector::requestPreview()
    {
        if( m_mip < 0 || m_mip >= 15 )
        {
            setStatus( "Preview mip must be between 0 and 14." );
            return;
        }
        String error;
        if( !m_jobs )
        {
            m_jobs = WorldStreamingScheduler::shared()->createProducer( { 1, 1, 1, previewResultLimit }, error );
            if( !m_jobs ) { setStatus( error ); return; }
        }
        WorldStreamRequest request;
        request.key = ++m_next;
        request.pendingBytes = m_document.original.size() + 8192;
        request.maximumResultBytes = previewResultLimit;
        request.execute = [document = m_document, draft = m_draft, mip = u32( m_mip ),
                           channel = render::TexturePreviewChannel( m_channel )]( const StreamCancellation &stop ) {
            WorldStreamResult result;
            if( stop.cancelled() ) return result;
            auto preview = std::make_shared<render::TextureProfilePreview>();
            if( render::buildTextureProfilePreview( document, draft, mip, channel, *preview, result.error ) &&
                !stop.cancelled() )
            {
                result.bytes = preview->image.size() + 4096;
                result.payload = std::move( preview );
            }
            return result;
        };
        if( !m_jobs->replace( { request }, error ) ) { setStatus( error ); return; }
        m_pending = request.key;
        setStatus( "Building target preview..." );
    }

    void TextureProfileInspector::takePreview()
    {
        WorldStreamResult result;
        if( !m_pending || !m_jobs->take( result ) || result.key != m_pending ) return;
        m_pending = 0;
        if( !result.payload ) { setStatus( result.error ); return; }
        auto preview = std::static_pointer_cast<const render::TextureProfilePreview>( result.payload );
        String error;
        if( !render::isTextureProfilePreviewCurrent( m_document, *preview, error ) )
        {
            retireImage();
            setStatus( error );
            return;
        }

        auto status = String( formatName( preview->format ) ) + ", " + StringUtil::toString( preview->width ) +
            " x " + StringUtil::toString( preview->height ) + ", " + StringUtil::toString( preview->mipCount ) +
            " mips, viewing mip " + StringUtil::toString( preview->mip ) + ". " +
            StringUtil::toString( preview->encodedBytes ) + " encoded bytes, " +
            StringUtil::toString( preview->residentBytes ) + " estimated resident bytes.";
        if( preview->hasReference )
            status += " Sampled mean channel error " + StringUtil::toString( preview->meanAbsoluteError ) + "/255.";
        if( preview->image.empty() )
        {
            retireImage();
            setStatus( status + " This encoding or source has no image for the selected preview channel." );
            return;
        }

        auto texture = make_ptr<render::ClawTexture>();
        const Vector2I size( s32( preview->imageWidth ), s32( preview->imageHeight ) );
        if( !texture || !texture->copyByteData( preview->image, size, error ) )
        {
            setStatus( error.empty() ? String( "Cannot allocate the preview texture." ) : error );
            return;
        }
        // Native upload is deferred to the existing ImGui image render path.
        // Each result gets a new immutable texture, so a submitted frame keeps
        // its previous GPU view until ImGui drops the frame's resource owner.
        m_image->setSize( Vector2F( f32( size.x ), f32( size.y ) ) );
        m_image->setTexture( texture );
        m_image->setVisible( true, false );
        setStatus( status );
    }

    void TextureProfileInspector::discardPreview()
    {
        if( m_pending && m_jobs ) m_jobs->cancel( m_pending );
        m_pending = 0;
        retireImage();
    }

    void TextureProfileInspector::retireImage()
    {
        m_image->setVisible( false, false );
        m_image->setTexture( nullptr );
    }

    void TextureProfileInspector::setStatus( const String &status )
    {
        const auto message = status.empty() ? String( "Texture preview failed." ) : status;
        if( m_status != message )
        {
            m_status = message;
            refresh();
        }
    }
}
#endif
