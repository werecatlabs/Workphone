#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Graphics/MaterialTexture.hpp>
#include <Workphone/Graphics/MaterialTechnique.hpp>
#include <Workphone/Graphics/MaterialPass.hpp>
#include <Workphone/Graphics/Material.hpp>
#include <Workphone/Interface/Animation/IAnimator.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <Workphone/Interface/Graphics/ITextureManager.hpp>
#include <Workphone/Interface/Database/IResourceDatabase.hpp>
#include <Workphone/Interface/System/IStateMessage.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <cstring>

namespace workphone::render
{
    namespace
    {
        template <class T> bool exactTextureValue( const T &a, const T &b )
        {
            return std::memcmp( &a, &b, sizeof( T ) ) == 0;
        }
        bool exactTextureValue( const Vector3<real_Num> &a, const Vector3<real_Num> &b )
        {
            return exactTextureValue( a.x, b.x ) && exactTextureValue( a.y, b.y ) &&
                   exactTextureValue( a.z, b.z );
        }
        bool exactTextureValue( const ColourF &a, const ColourF &b )
        {
            return exactTextureValue( a.r, b.r ) && exactTextureValue( a.g, b.g ) &&
                   exactTextureValue( a.b, b.b ) && exactTextureValue( a.a, b.a );
        }
    }

    const String MaterialTexture::textureStr = String( "texture" );
    const String MaterialTexture::generateStr = String( "Generate" );

    WP_CLASS_REGISTER_DERIVED( workphone::render, MaterialTexture, MaterialNode<IMaterialTexture> );

    MaterialTexture::MaterialTexture()
    {
        load( nullptr );
    }

    MaterialTexture::~MaterialTexture()
    {
        unload( nullptr );
    }

    void MaterialTexture::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            // Empty slots are created during authoring before graphics startup.
            // Only an actual texture upload needs a graphics service.
            if( !getTexture() )
                return;
            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
            WP_ASSERT( graphicsSystem );

            if( auto texture = getTexture() )
            {
                if( !texture->isLoaded() )
                {
                    graphicsSystem->loadObject( texture );
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void MaterialTexture::createTextureUnitState()
    {
        //auto parent = getParent();
        //auto parentPass = fb::static_pointer_cast<CMaterialPass>( parent );
        //if( parentPass )
        //{
        //    auto pass = parentPass->getPass();
        //    WP_ASSERT( pass );

        //    if( pass )
        //    {
        //        //m_textureUnitState = pass->createTextureUnitState();
        //    }
        //}
    }

    void MaterialTexture::reload( SmartPtr<ISharedObject> data )
    {
    }

    void MaterialTexture::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            const auto &loadingState = getLoadingState();
            if( loadingState != LoadingState::Unloaded )
            {
                // Capture the live owner before clearing/detaching the slot.
                // Unload retains its authoring path after releasing the resource.
                auto name = getTextureName();
                auto unresolved = name.empty() ? nullptr : std::make_unique<String>( name );
                setTexture( nullptr );
                setParent( nullptr );
                m_unresolvedTextureName = std::move( unresolved );
                setAnimator( nullptr );
                MaterialNode<IMaterialTexture>::unload( data );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    auto MaterialTexture::getTextureName() const -> String
    {
        if( auto texture = getTexture() )
        {
            auto path = texture->getFilePath();
            return path.empty() ? texture->getName() : path;
        }

        return m_unresolvedTextureName ? *m_unresolvedTextureName : String();
    }

    void MaterialTexture::setTextureName( const String &texturePath )
    {
        setTextureNameImpl( texturePath, false );
    }

    size_t MaterialTexture::getUnresolvedTextureNameObjectBytes() const
    {
        return m_unresolvedTextureName ? sizeof( String ) : 0;
    }

    void MaterialTexture::setTextureNameImpl( const String &texturePath, bool loadResource )
    {
        if( texturePath.empty() )
        {
            setTexture( nullptr );
            return;
        }
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto resourceDatabase = applicationManager ? applicationManager->getResourceDatabase() : nullptr;
        auto texture = resourceDatabase ? resourceDatabase->loadResourceByType<ITexture>( texturePath )
                                        : nullptr;
        if( texture )
        {
            if( loadResource && !texture->isLoaded() )
                texture->load( nullptr );
            // Do not publish an intermediate null binding when resolution succeeds.
            setTexture( texture );
            return;
        }
        // Retry resource resolution above even for unchanged unresolved names.
        if( m_unresolvedTextureName && *m_unresolvedTextureName == texturePath )
            return;
        auto unresolved = std::make_unique<String>( texturePath );
        setTexture( nullptr );
        m_unresolvedTextureName = std::move( unresolved );
    }

    auto MaterialTexture::getTexture() const -> SmartPtr<ITexture>
    {
        SmartPtr<ITexture> texture;
        if( auto parent = dynamic_pointer_cast<MaterialPass>( getParent() ) )
            if( parent->getRenderTextureBinding( this, texture ) ) return texture;
        return m_texture;
    }

    void MaterialTexture::setTexture( SmartPtr<ITexture> texture )
    {
        m_unresolvedTextureName.reset();
        if( auto parent = dynamic_pointer_cast<MaterialPass>( getParent() ) )
        {
            SmartPtr<ITexture> current;
            if( parent->getRenderTextureBinding( this, current ) )
            {
                m_texture = nullptr;
                parent->updateRenderTexture( this, texture );
                return;
            }
            m_texture = texture;
            parent->updateRenderTexture( this, texture );
            return;
        }
        m_texture = texture;
    }

    void MaterialTexture::setParent( SmartPtr<IMaterialNode> parent )
    {
        // Detached/pre-state units retain a binding; verified attached slots use
        // the pass owner. Capture before changing identity/context membership.
        m_texture = getTexture();
        MaterialNode<IMaterialTexture>::setParent( parent );
        SmartPtr<ITexture> current;
        if( auto pass = dynamic_pointer_cast<MaterialPass>( parent ) )
            if( pass->getRenderTextureBinding( this, current ) ) m_texture = nullptr;
    }

    void MaterialTexture::onCanonicalBindingChanged()
    {
        m_texture = nullptr;
        m_unresolvedTextureName.reset();
    }

    void MaterialTexture::setScale( const Vector3<real_Num> &scale )
    {
        if( exactTextureValue( getScale(), scale ) ) return;
        if( !m_textureAuthoring ) m_textureAuthoring = std::make_unique<TextureAuthoringData>();
        m_textureAuthoring->scale = scale;
        releaseDefaultAuthoringData();
    }

    Vector3<real_Num> MaterialTexture::getScale() const
    {
        return m_textureAuthoring ? m_textureAuthoring->scale : Vector3<real_Num>::unit();
    }

    size_t MaterialTexture::getTextureAuthoringStorageBytes() const
    {
        return m_textureAuthoring ? sizeof( TextureAuthoringData ) : 0;
    }

    void MaterialTexture::releaseDefaultAuthoringData()
    {
        if( m_textureAuthoring && exactTextureValue( m_textureAuthoring->scale, Vector3<real_Num>::unit() ) &&
            exactTextureValue( m_textureAuthoring->tint, ColourF::White ) && !m_textureAuthoring->animator )
            m_textureAuthoring.reset();
    }

    auto MaterialTexture::getAnimator() const -> SmartPtr<IAnimator>
    {
        return m_textureAuthoring ? m_textureAuthoring->animator : nullptr;
    }

    void MaterialTexture::setAnimator( SmartPtr<IAnimator> animator )
    {
        if( getAnimator() == animator ) return;
        if( !m_textureAuthoring ) m_textureAuthoring = std::make_unique<TextureAuthoringData>();
        m_textureAuthoring->animator = animator;
        releaseDefaultAuthoringData();
    }

    auto MaterialTexture::getTint() const -> ColourF
    {
        return m_textureAuthoring ? m_textureAuthoring->tint : ColourF::White;
    }

    void MaterialTexture::setTint( const ColourF &tint )
    {
        if( exactTextureValue( getTint(), tint ) ) return;
        if( !m_textureAuthoring ) m_textureAuthoring = std::make_unique<TextureAuthoringData>();
        m_textureAuthoring->tint = tint;
        releaseDefaultAuthoringData();
    }

    void MaterialTexture::_getObject( void **ppObject )
    {
        *ppObject = nullptr;
    }

    auto MaterialTexture::toData() const -> SmartPtr<ISharedObject>
    {
        auto properties = textureToData( getTexture() );
        if( properties )
            properties->setProperty( "texturePath", getTextureName() );
        return properties;
    }

    SmartPtr<Properties> MaterialTexture::textureToData(SmartPtr<ITexture> texture)
    {
        try
        {
            // A detached texture slot can be serialized before graphics startup.
            auto properties = workphone::make_ptr<Properties>();

            if( texture )
            {
                auto handle = texture->getHandle();
                properties->setProperty( MaterialTexture::textureStr, handle->getUUIDAsString(),
                                         Properties::resourceStr );
                properties->getPropertyObject( MaterialTexture::textureStr )
                    .setAttribute( "resourceType", "Texture" );
                // Keep a path fallback for materials used without an asset database entry.
                properties->setProperty( "texturePath", texture->getFilePath() );
            }

            return properties;
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return nullptr;
    }

    void MaterialTexture::fromData( SmartPtr<ISharedObject> data )
    {
        try
        {
            auto properties = workphone::dynamic_pointer_cast<Properties>( data );
            if( !properties )
                return;
            auto applicationManager = core::IApplicationManager::instancePtr();
            auto graphicsSystem = applicationManager ? applicationManager->getGraphicsSystem() : nullptr;
            ScopedLock lock( graphicsSystem );
            auto resourceDatabase = applicationManager ? applicationManager->getResourceDatabase() : nullptr;
            auto sTextureUUID = properties->getProperty( MaterialTexture::textureStr );
            // Older material files stored a texture path in the texture field.
            // Only parse identifiers here; retain legacy paths as a fallback.
            const auto isUUID = sTextureUUID.size() == 36u && sTextureUUID[8] == '-' &&
                                sTextureUUID[13] == '-' && sTextureUUID[18] == '-' &&
                                sTextureUUID[23] == '-';
            if( resourceDatabase && isUUID )
            {
                auto uuid = StringUtil::parseUUID( sTextureUUID );
                if( auto textureResource = resourceDatabase->loadResourceById( uuid ) )
                {
                    auto texture = workphone::static_pointer_cast<ITexture>( textureResource );

                    if( !texture->isLoaded() )
                    {
                        texture->load( nullptr );
                    }

                    setTexture( texture );
                }
            }
            if( !getTexture() )
            {
                auto path = properties->getProperty( "texturePath" );
                if( path.empty() && !isUUID )
                    path = sTextureUUID;
                if( !path.empty() )
                    setTextureNameImpl( path, true );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    auto MaterialTexture::getProperties() const -> SmartPtr<Properties>
    {
        try
        {
            auto properties = MaterialNode<IMaterialTexture>::getProperties();

            properties->setProperty( scaleStr, getScale() );
            properties->setProperty( tintStr, getTint() );

            auto texture = getTexture();
            properties->setProperty( texturePathStr, texture );

            auto material = getMaterial();
            if( material )
            {
                auto materialType = material->getMaterialType();
                switch( materialType )
                {
                case MaterialType::Standard:
                case MaterialType::StandardSpecular:
                case MaterialType::StandardTriPlanar:
                case MaterialType::TerrainStandard:
                case MaterialType::TerrainSpecular:
                case MaterialType::TerrainDiffuse:
                case MaterialType::Skybox:
                {
                    //auto textureType =
                    //    static_cast<SkyboxTextureTypes>( getTextureType() );
                    //switch( textureType )
                    //{
                    //case SkyboxCubeTextureTypes::Cube:
                    //{
                    //    properties->setProperty( MaterialTexture::generateStr, "GenerateButton", "button", false );
                    //}
                    //break;
                    //default:
                    //{
                    //}
                    //break;
                    //};
                }
                break;
                case MaterialType::SkyboxCubemap:
                case MaterialType::UI:
                case MaterialType::Custom:
                {
                }
                break;
                default:
                {
                }
                break;
                };
            }

            return properties;
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return nullptr;
    }

    void MaterialTexture::setProperties( SmartPtr<Properties> properties )
    {
        try
        {
            if( !properties ) return;
            Vector3<real_Num> scale;
            if( properties->getPropertyValue( scaleStr, scale ) ) setScale( scale );
            ColourF tint;
            if( properties->getPropertyValue( tintStr, tint ) ) setTint( tint );

            auto texture = SmartPtr<ITexture>();
            if( properties->hasProperty( texturePathStr ) )
            {
                properties->getPropertyValue( texturePathStr, texture );
                setTexture( texture );
            }

            if( properties->hasProperty( "Generate" ) )
            {
                auto &generateButton = properties->getPropertyObject( "Generate" );
                if( generateButton.getAttribute( "click" ) == "true" )
                {
                    auto applicationManager = core::IApplicationManager::instance();
                    auto graphicsSystem = applicationManager ? applicationManager->getGraphicsSystem() : nullptr;
                    auto textureManager = graphicsSystem ? graphicsSystem->getTextureManager() : nullptr;
                    if( !textureManager ) return;
                    Array<SmartPtr<ITexture>> skyboxTextures;

                    auto parent = getParent();
                    auto pass = workphone::static_pointer_cast<IMaterialPass>( parent );

                    auto textureUnits = pass->getTextureUnits();
                    for( auto textureUnit : textureUnits )
                    {
                        auto texture = textureUnit->getTexture();
                        skyboxTextures.push_back( texture );
                    }

                    textureManager->createCubeMap( skyboxTextures );
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    auto MaterialTexture::getChildObjects() const -> Array<SmartPtr<ISharedObject>>
    {
        try
        {
            auto objects = MaterialNode<IMaterialTexture>::getChildObjects();
            objects.reserve( 2 );

            objects.emplace_back( getAnimator() );
            objects.emplace_back( getTexture() );
            return objects;
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return {};
    }

    auto MaterialTexture::getTextureType() const -> u32
    {
        return m_textureType;
    }

    void MaterialTexture::setTextureType( u32 textureType )
    {
        m_textureType = textureType;
    }

    bool MaterialTexture::MaterialTextureStateListener::handleStateMessage(
        const SmartPtr<IStateMessage> &message )
    {
        return false;
    }

    bool MaterialTexture::MaterialTextureStateListener::handleStateChanged( SmartPtr<IState> &state )
    {
        return false;
    }

    MaterialTexture::MaterialTextureStateListener::MaterialTextureStateListener() = default;

    MaterialTexture::MaterialTextureStateListener::~MaterialTextureStateListener() = default;

}  // namespace workphone::render
