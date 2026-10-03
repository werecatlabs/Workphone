#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Graphics/Texture.hpp>
#include <Workphone/Interface/Graphics/IRenderTarget.hpp>
#include <Workphone/Interface/IO/IFileSystem.hpp>
#include <Workphone/Interface/IO/IStream.hpp>
#include <Workphone/Interface/System/IState.hpp>
#include <Workphone/State/States/State.hpp>
#include <Workphone/State/States/TextureStateData.hpp>
#include <Workphone/State/Messages/StateMessageVector2.hpp>
#include <Workphone/Core/Exception.hpp>
#include <Workphone/Core/Path.hpp>
#include <Workphone/Core/LogManager.hpp>

namespace workphone::render
{
    const String Texture::sizeStr = String( "Size" );

    WP_CLASS_REGISTER_DERIVED( workphone::render, Texture, ResourceGraphics<ITexture> );
    WP_CLASS_REGISTER_DERIVED( workphone::render, Texture::StateListener, IStateListener );

    u32 Texture::m_ext = 0;

    Texture::Texture()
    {
    }

    Texture::~Texture()
    {
        try
        {
            destroyStateObject();
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void Texture::update()
    {
        auto usageFlags = getUsageFlags();
        if( ( usageFlags & int( TextureUsage::TU_RENDERTARGET ) ) != 0 )
        {
            if( auto rt = getRenderTarget() )
            {
                rt->update();
            }
        }
    }

    void Texture::save()
    {
        //if( m_texture )
        {
            // m_texture->writeContentsToFile( "test.png", 0, 100 );
        }
    }

    void Texture::saveToFile( const String &filePath )
    {
    }

    void Texture::loadFromFile( const String &filePath )
    {
        try
        {
            WP_ASSERT( !StringUtil::isNullOrEmpty( filePath ) );

            auto materialName = Path::getFileNameWithoutExtension( filePath );

            // auto pTextures = getTextures();
            // if( pTextures )
            //{
            //     auto &textures = *pTextures;
            //     for( auto material : textures )
            //     {
            //         auto handle = material->getHandle();
            //         auto currentMaterialName = handle->getName();

            //        if( materialName == currentMaterialName )
            //        {
            //            return material;
            //        }
            //    }
            //}

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto fileSystem = applicationManager->getFileSystem();
            WP_ASSERT( fileSystem );

            auto stream = fileSystem->open( filePath );
            if( stream )
            {
                FileInfo fileInfo;
                if( fileSystem->findFileInfo( filePath, fileInfo ) )
                {
                    auto fileId = fileInfo.fileId;
                    setFileSystemId( fileId );
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void Texture::load( SmartPtr<ISharedObject> data )
    {
    }

    void Texture::reload( SmartPtr<ISharedObject> data )
    {
        try
        {
            unload( data );
            load( data );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void Texture::unload( SmartPtr<ISharedObject> data )
    {
        if( auto renderTarget = getRenderTarget() )
        {
            // Guard against self-reference: some render-target implementations
            // may point back to the texture object (i.e. the object implements
            // both ITexture and IRenderTarget). Calling unload() on the
            // render target when it is the same object can cause re-entrant
            // destruction (delete this) while we're still executing, leading
            // to use-after-free and access violations. Skip the unload call
            // when the render target is 'this'.
            if( renderTarget.get() != (void *)this )
            {
                renderTarget->unload( nullptr );
            }

            setRenderTarget( nullptr );
        }

        ResourceGraphics<ITexture>::unload( data );
    }

    auto Texture::getRenderTarget() const -> SmartPtr<IRenderTarget>
    {
        return m_renderTarget;
    }

    void Texture::setRenderTarget( SmartPtr<IRenderTarget> rt )
    {
        m_renderTarget = rt;
    }

    void Texture::copyToTexture( SmartPtr<ITexture> &target )
    {
    }

    void Texture::copyData( void *data, const Vector2I &size )
    {
    }

    auto Texture::getSize() const -> Vector2I
    {
        if( auto context = getStateContext() )
        {
            if( auto state = context->getStateDataById<TextureStateData>( getId() ) )
            {
                return state->size;
            }
        }

        return Vector2I::zero();
    }

    void Texture::setSize( const Vector2I &size )
    {
        if( auto context = getStateContext() )
        {
            if( auto state = context->invalidateStateDataById<TextureStateData>( getId() ) )
            {
                state->size = size;
            }
        }
    }

    void Texture::_getObject( void **ppObject ) const
    {
    }

    auto Texture::getActualSize() const -> Vector2I
    {
        if( auto context = getStateContext() )
        {
            if( auto state = context->getStateDataById<TextureStateData>( getId() ) )
            {
                return state->actualSize;
            }
        }

        return Vector2I::zero();
    }

    void Texture::getTextureGPU( void **ppTexture ) const
    {
        *ppTexture = nullptr;
    }

    void Texture::getTextureFinal( void **ppTexture ) const
    {
        *ppTexture = nullptr;
    }

    auto Texture::getTextureHandle() const -> size_t
    {
        return m_textureHandle;
    }

    auto Texture::getUsageFlags() const -> u32
    {
        return m_usageFlags;
    }

    void Texture::setUsageFlags( u32 usageFlags )
    {
        m_usageFlags = usageFlags;
    }

    void Texture::fromData( SmartPtr<ISharedObject> data )
    {
    }

    auto Texture::toData() const -> SmartPtr<ISharedObject>
    {
        auto data = workphone::make_ptr<Properties>();

        auto handle = getHandle();
        auto uuidStr = handle->getUUIDAsString();
        data->setProperty( "uuid", uuidStr );
        return data;
    }

    auto Texture::getProperties() const -> SmartPtr<Properties>
    {
        auto properties = ResourceGraphics<ITexture>::getProperties();

        auto size = getSize();
        properties->setProperty( Texture::sizeStr, size );

        return properties;
    }

    void Texture::setProperties( SmartPtr<Properties> properties )
    {
        ResourceGraphics<ITexture>::setProperties( properties );
    }

    auto Texture::getChildObjects() const -> Array<SmartPtr<ISharedObject>>
    {
        auto objects = Array<SmartPtr<ISharedObject>>();
        objects.reserve( 4 );

        if( auto renderTarget = getRenderTarget() )
        {
            objects.emplace_back( renderTarget );
        }

        return objects;
    }

    auto Texture::getTextureType() const -> TextureType
    {
        return m_textureType;
    }

    void Texture::setTextureType( TextureType textureType )
    {
        m_textureType = textureType;
    }

    void Texture::createStateObject()
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        WP_ASSERT( graphicsSystem );

        auto factoryManager = applicationManager->getFactoryManager();
        WP_ASSERT( factoryManager );

        auto stateManager = applicationManager->getStateManager();
        WP_ASSERT( stateManager );

        auto stateContext = stateManager->addStateContext();
        WP_ASSERT( stateContext );

        stateContext->setOwner( this );

        auto stateListener = factoryManager->make_ptr<StateListener>();
        stateListener->setOwner( this );
        stateContext->addStateListener( stateListener );

        auto state = factoryManager->make_ptr<State>();
        stateContext->addState( state );

        auto textureState = factoryManager->make_ptr<TextureStateData>();
        state->setData( textureState );

        auto stateTask = graphicsSystem->getStateTask();
        stateContext->setTaskId( stateTask );
    }

    bool Texture::handleStateMessage( const SmartPtr<IStateMessage> &message )
    {
        if( message->isDerived<StateMessageVector2I>() )
        {
            auto vector2Message = workphone::static_pointer_cast<StateMessageVector2I>( message );
            auto value = vector2Message->getValue();
            auto type = vector2Message->getType();

            if( type == STATE_MESSAGE_TEXTURE_SIZE )
            {
                setSize( value );
            }

            return true;
        }

        return false;
    }

    bool Texture::handleStateChanged( SmartPtr<IState> &state )
    {
        if( auto rtt = getRenderTarget() )
        {
            if( rtt->handleStateChanged( state ) )
            {
                return true;
            }
        }

        return false;
    }

    bool Texture::StateListener::handleStateMessage( const SmartPtr<IStateMessage> &message )
    {
        if( auto owner = getOwner() )
        {
            return owner->handleStateMessage( message );
        }

        return false;
    }

    bool Texture::StateListener::handleStateChanged( SmartPtr<IState> &state )
    {
        if( auto owner = getOwner() )
        {
            return owner->handleStateChanged( state );
        }

        return false;
    }

    auto Texture::StateListener::getOwner() const -> SmartPtr<Texture>
    {
        auto p = m_owner.load();
        return p.lock();
    }

    void Texture::StateListener::setOwner( SmartPtr<Texture> owner )
    {
        m_owner = owner;
    }

    Texture::StateListener::StateListener() = default;

    Texture::StateListener::~StateListener() = default;

}  // namespace workphone::render
