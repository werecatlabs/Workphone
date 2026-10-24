#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Graphics/TextureManager.hpp>
#include <Workphone/Interface/System/IFactoryManager.hpp>
#include <Workphone/Interface/Graphics/IGraphicsCubemap.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>
#include <Workphone/Interface/Graphics/IFont.hpp>
#include <Workphone/Interface/Graphics/IMaterial.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <Workphone/Interface/Graphics/IVideoTexture.hpp>
#include <Workphone/Interface/System/IStateMessage.hpp>
#include <Workphone/Interface/System/IStateContext.hpp>
#include <Workphone/Interface/System/IState.hpp>
#include <Workphone/Core/DebugTrace.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Thread/ScopedLock.hpp>
#include <mutex>

namespace
{
    // Headless/offline caches still need atomic create-or-retrieve and eviction.
    std::recursive_mutex g_headlessTextureLifecycle;
}

namespace workphone
{
    namespace render
    {

        WP_CLASS_REGISTER_DERIVED( workphone::render, TextureManager, ITextureManager );
        WP_CLASS_REGISTER_DERIVED( workphone::render, TextureManager::StateListener, IStateListener );

        TextureManager::TextureManager()
        {
        }

        TextureManager::~TextureManager()
        {
        }

        void TextureManager::load( SmartPtr<ISharedObject> data )
        {
            ScopedLock<TextureManager> lifecycle( this );
            m_textures.reserve( 1024 );
        }

        void TextureManager::unload( SmartPtr<ISharedObject> data )
        {
            ScopedLock<TextureManager> lifecycle( this );
            m_textures.releaseStorage();
        }

        SmartPtr<IResource> TextureManager::create( const String &name )
        {
            return nullptr;
        }

        SmartPtr<IResource> TextureManager::create( const String &uuid, const String &name )
        {
            return nullptr;
        }

        void TextureManager::destroyResource( SmartPtr<IResource> resource )
        {
            ScopedLock<TextureManager> lifecycle( this );
            if( !resource ) return;
            if( resource->isDerived<ITexture>() )
            {
                auto texture = workphone::static_pointer_cast<ITexture>( resource );
                if( texture )
                {
                    // Detach before callbacks: denied optional notifications or
                    // a reentrant lookup must not leave a ghost manager owner.
                    m_textures.erase( std::remove( m_textures.begin(), m_textures.end(), texture ),
                                      m_textures.end() );
                    texture->unload( nullptr );
                }
            }
        }

        void TextureManager::destroyAll()
        {
            ScopedLock<TextureManager> lifecycle( this );
            auto textures = m_textures.snapshot();
            for( auto &texture : textures )
            {
                texture->unload( nullptr );
            }

            textures.clear();
        }

        SmartPtr<IResource> TextureManager::loadResource( const String &name )
        {
            return nullptr;
        }

        SmartPtr<IResource> TextureManager::getByName( const String &name )
        {
            ScopedLock<TextureManager> lifecycle( this );
            try
            {
                ScopedLock<decltype( m_textures )> texturesLock( &m_textures );
                for( const auto &texture : m_textures )
                {
                    if( texture && texture->getName() == name )
                    {
                        return texture;
                    }
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }

            return nullptr;
        }

        SmartPtr<IResource> TextureManager::getById( const String &uuid )
        {
            ScopedLock<TextureManager> lifecycle( this );
            try
            {
                ScopedLock<decltype( m_textures )> texturesLock( &m_textures );
                for( const auto &texture : m_textures )
                {
                    if( texture )
                    {
                        auto handle = texture->getHandle();

                        if( handle && handle->getUUIDAsString() == uuid )
                        {
                            return texture;
                        }
                    }
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }

            return nullptr;
        }

        SmartPtr<IResource> TextureManager::cloneResource( const String &name,
                                                           const String &clonedResourceName )
        {
            return nullptr;
        }

        SmartPtr<IResource> TextureManager::cloneResource( SmartPtr<IResource> resource,
                                                           const String &clonedResourceName )
        {
            return nullptr;
        }

        Pair<SmartPtr<IResource>, bool> TextureManager::createOrRetrieve( const String &uuid,
                                                                          const String &path,
                                                                          const String &type )
        {
            return Pair<SmartPtr<IResource>, bool>();
        }

        Pair<SmartPtr<IResource>, bool> TextureManager::createOrRetrieve( const String &path )
        {
            return Pair<SmartPtr<IResource>, bool>();
        }

        void TextureManager::saveToFile( const String &filePath, SmartPtr<IResource> resource )
        {
        }

        SmartPtr<ITexture> TextureManager::createManual( const String &name, const String &group,
                                                         u8 texType, u32 width, u32 height, u32 depth,
                                                         s32 num_mips, u8 format, s32 usage /*= 0 */ )
        {
            return nullptr;
        }

        SmartPtr<IVideoTexture> TextureManager::createVideoTexture( const String &name )
        {
            return nullptr;
        }

        SmartPtr<IGraphicsCubemap> TextureManager::addCubemap()
        {
            return nullptr;
        }

        SmartPtr<ITexture> TextureManager::createRenderTexture()
        {
            return nullptr;
        }

        void TextureManager::destroyRenderTexture( SmartPtr<ITexture> texture )
        {
        }

        SmartPtr<ITexture> TextureManager::createCubeMap( const Array<String> &skyboxTextures )
        {
            return nullptr;
        }

        SmartPtr<ITexture> TextureManager::createCubeMap(
            const Array<SmartPtr<ITexture>> &skyboxTextures )
        {
            return nullptr;
        }

        SmartPtr<ITexture> TextureManager::createCubeMap( SmartPtr<IMaterial> material )
        {
            return nullptr;
        }

        auto TextureManager::cloneTexture( SmartPtr<ITexture> texture, const String &clonedTextureName )
            -> SmartPtr<ITexture>
        {
            auto clonedTexture = create( clonedTextureName );

            auto data = texture->toData();
            clonedTexture->fromData( data );

            return clonedTexture;
        }

        auto TextureManager::cloneTexture( const String &name, const String &clonedTextureName )
            -> SmartPtr<ITexture>
        {
            auto texture = getByName( name );
            return cloneTexture( texture, clonedTextureName );
        }

        Array<SmartPtr<ITexture>> TextureManager::getTextures() const
        {
            ScopedLock<TextureManager> lifecycle( this );
            return m_textures.snapshot();
        }

        u32 TextureManager::releaseUnusedResources( u32 /*maximumCandidates*/ )
        {
            return 0;
        }

        SmartPtr<IStateContext> TextureManager::getStateContext() const
        {
            return m_stateContext;
        }

        void TextureManager::setStateContext( SmartPtr<IStateContext> stateContext )
        {
            m_stateContext = stateContext;
        }

        bool TextureManager::handleStateMessage( const SmartPtr<IStateMessage> &message )
        {
            auto textures = m_textures.snapshot();
            for( auto &texture : textures )
            {
                if( message->getSender() == texture )
                {
                    if( texture )
                    {
                        if( texture->handleStateMessage( message ) )
                        {
                            return true;
                        }
                    }
                }
            }

            return false;
        }

        bool TextureManager::handleStateChanged( SmartPtr<IState> &state )
        {
            for( auto &texture : m_textures )
            {
                if( state->getOwnerPtr() == texture )
                {
                    if( texture )
                    {
                        if( texture->handleStateChanged( state ) )
                        {
                            return true;
                        }
                    }
                }
            }

            return false;
        }

        void TextureManager::lock()
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            auto graphicsSystem = applicationManager ? applicationManager->getGraphicsSystemPtr() : nullptr;
            if( graphicsSystem ) graphicsSystem->lock();
            else g_headlessTextureLifecycle.lock();
        }

        bool TextureManager::try_lock()
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            auto graphicsSystem = applicationManager ? applicationManager->getGraphicsSystemPtr() : nullptr;
            return graphicsSystem ? graphicsSystem->try_lock() : g_headlessTextureLifecycle.try_lock();
        }

        void TextureManager::unlock()
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            auto graphicsSystem = applicationManager ? applicationManager->getGraphicsSystemPtr() : nullptr;
            if( graphicsSystem ) graphicsSystem->unlock();
            else g_headlessTextureLifecycle.unlock();
        }

        TextureManager::StateListener::StateListener() = default;

        TextureManager::StateListener::~StateListener() = default;

        void TextureManager::StateListener::unload( SmartPtr<ISharedObject> data )
        {
            m_owner = nullptr;
        }

        bool TextureManager::StateListener::handleStateMessage( const SmartPtr<IStateMessage> &message )
        {
            if( auto owner = getOwnerPtr() )
            {
                return owner->handleStateMessage( message );
            }

            return false;
        }

        bool TextureManager::StateListener::handleStateChanged( SmartPtr<IState> &state )
        {
            if( auto owner = getOwnerPtr() )
            {
                return owner->handleStateChanged( state );
            }

            return false;
        }

        SmartPtr<TextureManager> TextureManager::StateListener::getOwner() const
        {
            auto p = m_owner.load();
            return p.lock();
        }

        void TextureManager::StateListener::setOwner( SmartPtr<TextureManager> owner )
        {
            m_owner = owner;
        }

    }  // namespace render
}  // namespace workphone
