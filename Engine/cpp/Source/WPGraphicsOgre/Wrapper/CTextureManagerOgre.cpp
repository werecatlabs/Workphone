#include <WPGraphicsOgre/WPGraphicsOgrePCH.hpp>
#include <WPGraphicsOgre/Wrapper/CTextureManagerOgre.hpp>
#include <WPGraphicsOgre/Wrapper/CTextureOgre.hpp>
#include <WPGraphicsOgre/Wrapper/CVideoTextureOgre.hpp>
#include <WPGraphicsOgre/Wrapper/CCubemap.hpp>
#include <WPGraphicsOgre/Wrapper/CRenderTexture.hpp>
#include <Workphone/Workphone.hpp>
#include <OgreTextureManager.h>

namespace workphone
{
    namespace render
    {

        WP_CLASS_REGISTER_DERIVED( workphone::render, CTextureManagerOgre, TextureManager );
        WP_CLASS_REGISTER_DERIVED( workphone::render, CTextureManagerOgre::TextureListener,
                                   IEventListener );

        CTextureManagerOgre::CTextureManagerOgre()
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto factoryManager = applicationManager->getFactoryManager();

            m_textureListener = factoryManager->make_ptr<TextureListener>();
        }

        CTextureManagerOgre::~CTextureManagerOgre()
        {
            unload( nullptr );
        }

        void CTextureManagerOgre::preUpdate()
        {
            for( auto rt : m_renderTargets )
            {
                rt->preUpdate();
            }
        }

        void CTextureManagerOgre::update()
        {
            for( auto rt : m_renderTargets )
            {
                rt->update();
            }
        }

        void CTextureManagerOgre::postUpdate()
        {
            for( auto rt : m_renderTargets )
            {
                rt->postUpdate();
            }
        }

        void CTextureManagerOgre::load( SmartPtr<ISharedObject> data )
        {
            setLoadingState( LoadingState::Loaded );
        }

        void CTextureManagerOgre::unload( SmartPtr<ISharedObject> data )
        {
            try
            {
                WP_ASSERT( isValid() );

                if( const auto &loadingState = getLoadingState();
                    loadingState != LoadingState::Unloaded )
                {
                    setLoadingState( LoadingState::Unloading );

                    for( auto &rtt : m_renderTargets )
                    {
                        rtt->unload( nullptr );
                    }

                    m_renderTargets.clear();

                    auto textures = m_textures.snapshot();
                    for( auto &t : textures )
                    {
                        t->unload( nullptr );
                    }

                    m_textures.clear();

                    setLoadingState( LoadingState::Unloaded );
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        SmartPtr<ITexture> CTextureManagerOgre::createManual( const String &name, const String &group,
                                                              u8 texType, u32 width, u32 height,
                                                              u32 depth, s32 num_mips, u8 format,
                                                              s32 usage /*= TU_DEFAULT*/ )
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto factoryManager = applicationManager->getFactoryManager();
            WP_ASSERT( factoryManager );

            auto eTexType = static_cast<Ogre::TextureType>( texType );
            auto ePixelFormat = static_cast<Ogre::PixelFormat>( format );

            auto textureManager = Ogre::TextureManager::getSingletonPtr();
            auto tex = textureManager->createManual( name.c_str(), group.c_str(), eTexType, width,
                                                     height, depth, num_mips, ePixelFormat, usage );

            auto texture = factoryManager->make_ptr<CTextureOgre>();
            texture->initialise( tex );

            return texture;
        }

        SmartPtr<IResource> CTextureManagerOgre::create( const String &name )
        {
            try
            {
                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                auto fileSystem = applicationManager->getFileSystem();

                auto factoryManager = applicationManager->getFactoryManager();
                WP_ASSERT( factoryManager );

                auto texture = factoryManager->make_ptr<CTextureOgre>();
                WP_ASSERT( texture );

                texture->addObjectListener( m_textureListener.load() );

                auto handle = texture->getHandle();
                WP_ASSERT( handle );

                texture->setName( name );

                auto uuid = StringUtil::getUUID();
                handle->setUUID( uuid );

                FileInfo fileInfo;
                if( fileSystem->findFileInfo( name, fileInfo, false ) )
                {
                    texture->setFileSystemId( fileInfo.fileId );
                }
                else if( fileSystem->findFileInfo( name, fileInfo, true ) )
                {
                    texture->setFileSystemId( fileInfo.fileId );
                }

                texture->setFilePath( name );

                m_textures.push_back( texture );
                return texture;
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }

            return nullptr;
        }

        SmartPtr<IResource> CTextureManagerOgre::create( const String &uuid, const String &name )
        {
            try
            {
                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                auto fileSystem = applicationManager->getFileSystem();

                auto factoryManager = applicationManager->getFactoryManager();
                WP_ASSERT( factoryManager );

                auto texture = factoryManager->make_ptr<CTextureOgre>();
                WP_ASSERT( texture );

                texture->addObjectListener( m_textureListener.get() );

                auto handle = texture->getHandle();
                WP_ASSERT( handle );

                texture->setName( name );
                handle->setUUID( uuid );

                FileInfo fileInfo;
                if( fileSystem->findFileInfo( name, fileInfo, false ) )
                {
                    texture->setFileSystemId( fileInfo.fileId );
                }
                else if( fileSystem->findFileInfo( name, fileInfo, true ) )
                {
                    texture->setFileSystemId( fileInfo.fileId );
                }

                texture->setFilePath( name );

                m_textures.push_back( texture );
                return texture;
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }

            return nullptr;
        }

        SmartPtr<IResource> CTextureManagerOgre::loadResource( const String &name )
        {
            try
            {
                WP_ASSERT( !StringUtil::isNullOrEmpty( name ) );

                auto textureResource = getByName( name );
                if( textureResource )
                {
                    auto texture = workphone::static_pointer_cast<CTextureOgre>( textureResource );
                    texture->load( nullptr );
                    return texture;
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }

            return nullptr;
        }

        Pair<SmartPtr<IResource>, bool> CTextureManagerOgre::createOrRetrieve( const String &uuid,
                                                                               const String &path,
                                                                               const String &type )
        {
            try
            {
                auto textureResource = getById( uuid );
                if( textureResource )
                {
                    auto texture = workphone::static_pointer_cast<CTextureOgre>( textureResource );
                    return Pair<SmartPtr<IResource>, bool>( texture, false );
                }

                auto texture = create( uuid, path );
                WP_ASSERT( texture );

                return Pair<SmartPtr<IResource>, bool>( texture, true );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }

            return Pair<SmartPtr<IResource>, bool>( nullptr, false );
        }

        Pair<SmartPtr<IResource>, bool> CTextureManagerOgre::createOrRetrieve( const String &path )
        {
            try
            {
                auto textureResource = getByName( path );
                if( textureResource )
                {
                    auto texture = workphone::static_pointer_cast<CTextureOgre>( textureResource );
                    return Pair<SmartPtr<IResource>, bool>( texture, false );
                }

                auto texture = create( path );
                WP_ASSERT( texture );

                return Pair<SmartPtr<IResource>, bool>( texture, true );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }

            return Pair<SmartPtr<IResource>, bool>( nullptr, false );
        }

        SmartPtr<IResource> CTextureManagerOgre::loadFromFile( const String &filePath )
        {
            try
            {
                WP_ASSERT( !StringUtil::isNullOrEmpty( filePath ) );

                auto materialName = Path::getFileNameWithoutExtension( filePath );

                auto textures = m_textures.snapshot();
                for( auto material : textures )
                {
                    auto currentMaterialName = material->getName();

                    if( materialName == currentMaterialName )
                    {
                        return material;
                    }
                }

                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                auto fileSystem = applicationManager->getFileSystem();
                WP_ASSERT( fileSystem );

                auto stream = fileSystem->open( filePath );
                if( stream )
                {
                    auto material = create( filePath );

                    FileInfo fileInfo;
                    if( fileSystem->findFileInfo( filePath, fileInfo ) )
                    {
                        auto fileId = fileInfo.fileId;
                        material->setFileSystemId( fileId );
                    }

                    return material;
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }

            return nullptr;
        }

        SmartPtr<IVideoTexture> CTextureManagerOgre::createVideoTexture( const String &name )
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto factoryManager = applicationManager->getFactoryManager();
            WP_ASSERT( factoryManager );

            auto videoTexture = factoryManager->make_ptr<CVideoTextureOgre>();
            return videoTexture;
        }

        SmartPtr<IGraphicsCubemap> CTextureManagerOgre::addCubemap()
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto factoryManager = applicationManager->getFactoryManager();
            WP_ASSERT( factoryManager );

            auto cubemap = factoryManager->make_ptr<CCubemap>();
            return cubemap;
        }

        SmartPtr<ITexture> CTextureManagerOgre::createRenderTexture()
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto factoryManager = applicationManager->getFactoryManager();
            WP_ASSERT( factoryManager );

            auto graphicsSystem = applicationManager->getGraphicsSystem();
            WP_ASSERT( graphicsSystem );

            auto texture = factoryManager->make_ptr<CTextureOgre>();
            texture->setUsageFlags( (u32)TextureUsage::TU_RENDERTARGET );

            auto renderTarget = factoryManager->make_ptr<CRenderTexture>();
            renderTarget->setTexture( texture );
            renderTarget->load( nullptr );
            texture->setRenderTarget( renderTarget );

            graphicsSystem->loadObject( texture );

            m_textures.push_back( texture );
            addRenderTexture( texture );

            return texture;
        }

        void CTextureManagerOgre::destroyRenderTexture( SmartPtr<ITexture> texture )
        {
            if( texture )
            {
                texture->unload( nullptr );

                m_textures.erase( std::remove( m_textures.begin(), m_textures.end(), texture ),
                                  m_textures.end() );

                auto it = std::find( m_renderTargets.begin(), m_renderTargets.end(), texture );
                if( it != m_renderTargets.end() )
                {
                    m_renderTargets.erase( it );
                }
            }
        }

        SmartPtr<ITexture> CTextureManagerOgre::createCubeMap( const Array<String> &textures )
        {
            return nullptr;
        }

        SmartPtr<ITexture> CTextureManagerOgre::createCubeMap(
            const Array<SmartPtr<ITexture>> &textures )
        {
            return nullptr;
        }

        SmartPtr<ITexture> CTextureManagerOgre::createCubeMap( SmartPtr<IMaterial> material )
        {
            return nullptr;
        }

        void CTextureManagerOgre::_getObject( void **ppObject ) const
        {
            *ppObject = Ogre::TextureManager::getSingletonPtr();
        }

        void CTextureManagerOgre::addRenderTexture( SmartPtr<ITexture> texture )
        {
            m_renderTargets.push_back( texture );
        }

        void CTextureManagerOgre::removeRenderTexture( SmartPtr<ITexture> texture )
        {
            m_renderTargets.erase(
                std::remove( m_renderTargets.begin(), m_renderTargets.end(), texture ),
                m_renderTargets.end() );
        }

        void CTextureManagerOgre::createRenderTextures()
        {
            m_renderTargets.clear();
        }

        CTextureManagerOgre::TextureListener::TextureListener() = default;

        CTextureManagerOgre::TextureListener::~TextureListener() = default;

        Parameter CTextureManagerOgre::TextureListener::handleEvent(
            EventType eventType, hash_type eventValue, const Array<Parameter> &arguments,
            SmartPtr<ISharedObject> sender, SmartPtr<ISharedObject> object, SmartPtr<IEvent> event )
        {
            return {};
        }

        void CTextureManagerOgre::TextureListener::loadingStateChanged( ISharedObject *sharedObject,
                                                                        LoadingState oldState,
                                                                        LoadingState newState )
        {
            if( newState == LoadingState::Loaded )
            {
            }
        }

        bool CTextureManagerOgre::TextureListener::destroy( void *ptr )
        {
            return false;
        }
    }  // end namespace render
}  // namespace workphone
