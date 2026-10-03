#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/Wrapper/CTextureManagerOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CTextureOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CVideoTextureOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CCubemapOgreNext.hpp>
#include <WPGraphicsOgreNext/OgreUtil.hpp>
#include <Workphone/Core/DebugTrace.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Core/Path.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <Workphone/Interface/Graphics/IMaterial.hpp>
#include <Workphone/Interface/IO/IFileSystem.hpp>
#include <Workphone/Interface/IO/IStream.hpp>
#include <Workphone/Interface/System/IFactoryManager.hpp>
#include <Workphone/Interface/System/IStateManager.hpp>
#include <Workphone/Interface/System/IStateContext.hpp>
#include <Ogre.h>
#include <OgreFileSystem.h>
#include <OgreImage2.h>
#include <OgrePixelFormatGpuUtils.h>
#include <OgreTextureBox.h>
#include <OgreTextureGpuManager.h>
#include <OgreLwString.h>

#if defined WP_PLATFORM_APPLE
#    include <WPGraphicsOgreNext/Wrapper/Apple/CTextureOgreApple.hpp>
#endif

namespace
{
    Ogre::TextureTypes::TextureTypes getOgreTextureType( workphone::u8 texType )
    {
        using namespace workphone;

        if( texType == static_cast<u8>( TextureType::TEX_TYPE_1D ) )
        {
            return Ogre::TextureTypes::Type1D;
        }
        if( texType == static_cast<u8>( TextureType::TEX_TYPE_2D_ARRAY ) )
        {
            return Ogre::TextureTypes::Type2DArray;
        }
        if( texType == static_cast<u8>( TextureType::TEX_TYPE_3D ) )
        {
            return Ogre::TextureTypes::Type3D;
        }
        if( texType == static_cast<u8>( TextureType::TEX_TYPE_CUBE_MAP ) )
        {
            return Ogre::TextureTypes::TypeCube;
        }

        return Ogre::TextureTypes::Type2D;
    }

    Ogre::PixelFormatGpu getOgrePixelFormat( workphone::u8 format )
    {
        using namespace workphone;

        if( format == static_cast<u8>( PixelFormat::PF_L8 ) )
        {
            return Ogre::PFG_R8_UNORM;
        }
        if( format == static_cast<u8>( PixelFormat::PF_L16 ) )
        {
            return Ogre::PFG_R16_UNORM;
        }
        if( format == static_cast<u8>( PixelFormat::PF_BYTE_RGBA ) ||
            format == static_cast<u8>( PixelFormat::PF_R8G8B8A8 ) )
        {
            return Ogre::PFG_RGBA8_UNORM;
        }
        if( format == static_cast<u8>( PixelFormat::PF_BYTE_BGRA ) ||
            format == static_cast<u8>( PixelFormat::PF_B8G8R8A8 ) ||
            format == static_cast<u8>( PixelFormat::PF_A8R8G8B8 ) )
        {
            return Ogre::PFG_BGRA8_UNORM;
        }
        if( format == static_cast<u8>( PixelFormat::PF_FLOAT32_R ) )
        {
            return Ogre::PFG_R32_FLOAT;
        }
        if( format == static_cast<u8>( PixelFormat::PF_FLOAT32_RGBA ) )
        {
            return Ogre::PFG_RGBA32_FLOAT;
        }

        return Ogre::PFG_RGBA8_UNORM;
    }

    Ogre::uint32 getOgreTextureFlags( workphone::s32 usage )
    {
        using namespace workphone;

        const auto usageFlags = static_cast<u32>( usage );
        auto textureFlags = static_cast<Ogre::uint32>( Ogre::TextureFlags::ManualTexture );

        if( ( usageFlags & static_cast<u32>( TextureUsage::TU_RENDERTARGET ) ) != 0u )
        {
            textureFlags |= static_cast<Ogre::uint32>( Ogre::TextureFlags::RenderToTexture );
        }

        if( ( usageFlags & static_cast<u32>( TextureUsage::TU_AUTOMIPMAP ) ) != 0u )
        {
            textureFlags |= static_cast<Ogre::uint32>( Ogre::TextureFlags::AllowAutomipmaps );
        }

        return textureFlags;
    }
}  // namespace

namespace workphone::render
{

    WP_CLASS_REGISTER_DERIVED( workphone::render, CTextureManagerOgreNext, TextureManager );
    WP_CLASS_REGISTER_DERIVED( workphone::render, CTextureManagerOgreNext::TextureListener,
                               IEventListener );
    WP_CLASS_REGISTER_DERIVED( workphone::render, CTextureManagerOgreNext::StateListener,
                               IStateListener );

    CTextureManagerOgreNext::CTextureManagerOgreNext()
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto factoryManager = applicationManager->getFactoryManager();
        WP_ASSERT( factoryManager );

        auto stateManager = applicationManager->getStateManager();
        WP_ASSERT( stateManager );

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        WP_ASSERT( graphicsSystem );

        static const auto TextureManagerStr = String( "CTextureManagerOgreNext" );
        setName( TextureManagerStr );

        m_textureListener = factoryManager->make_ptr<TextureListener>();

        auto stateContext = stateManager->addStateContext();
        stateContext->setOwner( this );
        setStateContext( stateContext );
        stateContext->setTaskId( TaskId::Render );

        auto stateListener = factoryManager->make_ptr<StateListener>();
        stateListener->setOwner( this );
        stateContext->addStateListener( stateListener );
        m_textureStateListener = stateListener;
    }

    CTextureManagerOgreNext::~CTextureManagerOgreNext() = default;

    void CTextureManagerOgreNext::update()
    {
        ScopedLock lock( this );

        /*
        for( auto rt : m_renderTargets )
        {
            rt->update();
        }*/

        Pair<SmartPtr<ITexture>, u32> p;
        while( m_transitionTextures.try_pop( p ) )
        {
            auto state = (Ogre::GpuResidency::GpuResidency)p.second;
            auto renderTexture = workphone::static_pointer_cast<CTextureOgreNext>( p.first );
            if( auto ogreTexture = renderTexture->getTexture() )
            {
                if( ogreTexture->getResidencyStatus() != state )
                {
                    ogreTexture->scheduleTransitionTo( state );
                }
            }
        }

        auto textureLoadingOptions = workphone::make_ptr<CTextureOgreNext::TextureLoadingOptions>();
        textureLoadingOptions->reloadTextureObject = true;
        textureLoadingOptions->unloadViewports = false;

        SmartPtr<ITexture> texture;
        while( m_reloadTextures.try_pop( texture ) )
        {
            texture->reload( textureLoadingOptions );

            //auto renderTexture = fb::static_pointer_cast<CTextureOgreNext>( texture );
            //if( auto ogreTexture = renderTexture->getTexture() )
            //{
            //    if( ogreTexture->getResidencyStatus() != Ogre::GpuResidency::OnStorage )
            //    {
            //        ogreTexture->scheduleTransitionTo( Ogre::GpuResidency::OnStorage );
            //    }
            //}

            //auto pair = std::make_pair( texture, Ogre::GpuResidency::Resident );
            //m_transitionTextures.push( pair );
        }
    }

    void CTextureManagerOgreNext::postUpdate()
    {
        for( auto rt : m_renderTargets )
        {
            rt->postUpdate();
        }
    }

    void CTextureManagerOgreNext::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );
            TextureManager::load( data );
            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void CTextureManagerOgreNext::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            WP_ASSERT( isValid() );

            const auto &loadingState = getLoadingState();
            if( loadingState != LoadingState::Unloaded )
            {
                setLoadingState( LoadingState::Unloading );

                auto stateContext = getStateContext();
                if( stateContext )
                {
                    stateContext->setOwner( nullptr );

                    if( m_textureStateListener )
                    {
                        stateContext->removeStateListener( m_textureStateListener );
                    }
                }

                if( m_textureStateListener )
                {
                    m_textureStateListener->setOwner( nullptr );
                    m_textureStateListener = nullptr;
                }

                if( stateContext )
                {
                    if( auto applicationManager = core::IApplicationManager::instancePtr() )
                    {
                        if( auto stateManager = applicationManager->getStateManagerPtr() )
                        {
                            stateManager->removeStateContext( stateContext );
                        }
                    }

                    setStateContext( nullptr );
                }

                auto textures = m_textures.snapshot();
                for( auto &t : textures )
                {
                    t->unload( nullptr );
                }

                m_textures.clear();

                for( auto &rtt : m_renderTargets )
                {
                    rtt->unload( nullptr );
                }

                if( m_textureListener )
                {
                    m_textureListener->unload( nullptr );
                    m_textureListener = nullptr;
                }

                m_renderTargets.clear();

                TextureManager::unload( data );

                setLoadingState( LoadingState::Unloaded );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    auto CTextureManagerOgreNext::createManual( const String &name, const String &group, u8 texType,
                                                u32 width, u32 height, u32 depth, s32 num_mips,
                                                u8 format, s32 usage /*= TU_DEFAULT*/ )
        -> SmartPtr<ITexture>
    {
        try
        {
            auto root = Ogre::Root::getSingletonPtr();
            WP_ASSERT( root );
            auto renderSystem = root->getRenderSystem();
            WP_ASSERT( renderSystem );
            auto textureManager = renderSystem->getTextureGpuManager();
            WP_ASSERT( textureManager );

            const auto ogreTexType = getOgreTextureType( texType );
            const auto ogreFormat = getOgrePixelFormat( format );
            const auto textureFlags = getOgreTextureFlags( usage );

            // Create the Ogre texture
            Ogre::TextureGpu *ogreTexture =
                textureManager->createTexture( name.c_str(), Ogre::GpuPageOutStrategy::Discard,
                                               textureFlags, ogreTexType, group.c_str() );

            WP_ASSERT( ogreTexture );

            ogreTexture->setResolution( width, height, depth );
            ogreTexture->setPixelFormat( ogreFormat );
            if( num_mips > 0 )
                ogreTexture->setNumMipmaps( static_cast<uint8_t>( num_mips ) );

            // Schedule transition to Resident so it is usable
            ogreTexture->scheduleTransitionTo( Ogre::GpuResidency::Resident );

            // Create the wrapper texture object
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto factoryManager = applicationManager->getFactoryManager();
            WP_ASSERT( factoryManager );

            auto texture = factoryManager->make_ptr<CTextureOgreNext>();
            WP_ASSERT( texture );

            texture->setName( name );
            texture->setTexture( ogreTexture );
            texture->setSize( Vector2I( width, height ) );
            texture->setUsageFlags( static_cast<u32>( usage ) );
            texture->setLoadingState( LoadingState::Loaded );
            texture->addObjectListener( m_textureListener );

            m_textures.push_back( texture );
            return texture;
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
        return nullptr;
    }

    auto CTextureManagerOgreNext::create( const String &name ) -> SmartPtr<IResource>
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto fileSystem = applicationManager->getFileSystem();

            auto factoryManager = applicationManager->getFactoryManager();
            WP_ASSERT( factoryManager );

            auto texture = factoryManager->make_ptr<CTextureOgreNext>( this );
            WP_ASSERT( texture );

            texture->addObjectListener( m_textureListener );

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

    auto CTextureManagerOgreNext::create( const String &uuid, const String &name ) -> SmartPtr<IResource>
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto fileSystem = applicationManager->getFileSystem();

            auto factoryManager = applicationManager->getFactoryManager();
            WP_ASSERT( factoryManager );

            auto texture = factoryManager->make_ptr<CTextureOgreNext>( this );
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

    auto CTextureManagerOgreNext::loadResource( const String &name ) -> SmartPtr<IResource>
    {
        try
        {
            WP_ASSERT( !StringUtil::isNullOrEmpty( name ) );

            auto textureResource = getByName( name );
            if( textureResource )
            {
                auto texture = workphone::static_pointer_cast<CTextureOgreNext>( textureResource );
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

    auto CTextureManagerOgreNext::createOrRetrieve( const String &uuid, const String &path,
                                                    const String &type )
        -> Pair<SmartPtr<IResource>, bool>
    {
        try
        {
            if( auto textureResource = getById( uuid ) )
            {
                auto texture = workphone::static_pointer_cast<CTextureOgreNext>( textureResource );
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

    auto CTextureManagerOgreNext::createOrRetrieve( const String &path )
        -> Pair<SmartPtr<IResource>, bool>
    {
        try
        {
            if( auto textureResource = getByName( path ) )
            {
                auto texture = workphone::static_pointer_cast<CTextureOgreNext>( textureResource );
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

    void CTextureManagerOgreNext::saveToFile( const String &filePath, SmartPtr<IResource> resource )
    {
        if( resource )
        {
            resource->saveToFile( filePath );
        }
    }

    auto CTextureManagerOgreNext::loadFromFile( const String &filePath ) -> SmartPtr<IResource>
    {
        try
        {
            WP_ASSERT( !StringUtil::isNullOrEmpty( filePath ) );

            auto textureName = Path::getFileNameWithoutExtension( filePath );

            auto textures = m_textures.snapshot();
            for( auto texture : textures )
            {
                auto currentTextureName = texture->getName();

                if( textureName == currentTextureName )
                {
                    return texture;
                }
            }

            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto fileSystem = applicationManager->getFileSystemPtr();
            WP_ASSERT( fileSystem );

            if( auto texture = create( filePath ) )
            {
                FileInfo fileInfo;
                if( fileSystem->findFileInfo( filePath, fileInfo ) )
                {
                    auto fileId = fileInfo.fileId;
                    texture->setFileSystemId( fileId );
                }

                return texture;
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return nullptr;
    }

    auto CTextureManagerOgreNext::createVideoTexture( const String &name ) -> SmartPtr<IVideoTexture>
    {
        try
        {
            WP_LOG_ERROR( "CTextureManagerOgreNext::createVideoTexture is not implemented. Name: " +
                          name );
        }
        catch( std::exception &e )
        {
            WP_LOG_ERROR( "Exception in CTextureManagerOgreNext::createVideoTexture. Name: " + name );
            WP_LOG_EXCEPTION( e );
        }
        return nullptr;
    }

    auto CTextureManagerOgreNext::addCubemap() -> SmartPtr<IGraphicsCubemap>
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto factoryManager = applicationManager->getFactoryManager();
            WP_ASSERT( factoryManager );

            auto cubemap = factoryManager->make_ptr<CCubemapOgreNext>();
            WP_ASSERT( cubemap );

            return cubemap;
        }
        catch( std::exception &e )
        {
            WP_LOG_ERROR( "Exception in CTextureManagerOgreNext::addCubemap" );
            WP_LOG_EXCEPTION( e );
        }
        return nullptr;
    }

    auto CTextureManagerOgreNext::createRenderTexture() -> SmartPtr<ITexture>
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto factoryManager = applicationManager->getFactoryManagerPtr();
        WP_ASSERT( factoryManager );

#if defined WP_PLATFORM_WIN32
        auto texture = factoryManager->make_ptr<CTextureOgreNext>();
        texture->setResourceManager( this );
        texture->setUsageFlags( (u32)TextureUsage::TU_RENDERTARGET );
        texture->load( nullptr );
        m_textures.push_back( texture );
        return texture;
#elif defined WP_PLATFORM_APPLE
        auto texture = factoryManager->make_ptr<CTextureOgreApple>();
        texture->setResourceManager( this );
        texture->setUsageFlags( ITexture::TU_RENDERTARGET );
        texture->load( nullptr );
        addTexture( texture );
        return texture;
#endif
    }

    void CTextureManagerOgreNext::destroyRenderTexture( SmartPtr<ITexture> texture )
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

    auto CTextureManagerOgreNext::createCubeMap( const Array<String> &textureNames )
        -> SmartPtr<ITexture>
    {
        if( textureNames.size() < 6 )
        {
            WP_LOG_ERROR( "Cube map requires at least 6 texture faces." );
            return nullptr;
        }

        try
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto fileSystem = applicationManager->getFileSystem();
            WP_ASSERT( fileSystem );

            auto root = Ogre::Root::getSingletonPtr();
            auto renderSystem = root->getRenderSystem();
            if( !renderSystem )
            {
                WP_LOG_ERROR( "No render system found. Cannot create skybox cubemap." );
                return nullptr;
            }

            auto textureManager = renderSystem->getTextureGpuManager();

            const auto &faceName0 = textureNames.front();

            if( StringUtil::isNullOrEmpty( faceName0 ) )
            {
                return nullptr;
            }

            auto uuid = StringUtil::getUUID( faceName0 );

            static const String ddsExt = ".dds";
            auto textureName = StringUtil::toString( uuid ) + ddsExt;

            if( auto textureResource = getByName( textureName ) )
            {
                return textureResource;
            }

            if( !fileSystem->isExistingFile( textureName, true, true ) )
            {
                using namespace Ogre;

                Image2 faceImage;

                const bool bOpenSuccess = OgreUtil::loadIntoImage( faceName0, faceImage );

                if( !bOpenSuccess )
                {
                    WP_LOG_ERROR( "Could not determine resolution nor pixel format for face: %s. Aborting", faceName0.c_str() );
                    return nullptr;
                }

                const auto width = faceImage.getWidth();
                const auto height = faceImage.getHeight();
                const auto pixelFormat = faceImage.getPixelFormat();

                const uint8_t maxNumMipmaps = 2;
                auto numMipmaps = PixelFormatGpuUtils::getMaxMipmapCount( width, height );
                numMipmaps = std::min( numMipmaps, maxNumMipmaps );

                const auto requiredBytes = PixelFormatGpuUtils::calculateSizeBytes(
                    width, height, 1u, 6u, pixelFormat, numMipmaps, 4u );

                void *data = OGRE_MALLOC_SIMD( requiredBytes, MEMCATEGORY_GENERAL );

                auto finalCubemap = OGRE_NEW Image2();
                finalCubemap->loadDynamicImage( data, width, height, 6u, TextureTypes::TypeCube,
                                                pixelFormat, true, numMipmaps );
                OGRE_FREE( data, MEMCATEGORY_GENERAL );

                for( uint8_t mip = 0u; mip < numMipmaps; ++mip )
                {
                    for( uint32_t face = 0u; face < 6u; ++face )
                    {
                        if( mip != 0u || face != 0u )
                        {
                            const auto &faceName = textureNames[face];
                            OgreUtil::loadIntoImage( faceName, faceImage );
                        }

                        auto dstBox = finalCubemap->getData( mip );
                        if( faceImage.getWidth() != dstBox.width ||
                            faceImage.getHeight() != dstBox.height )
                        {
                            faceImage.resize( dstBox.width, dstBox.height );
                        }

                        auto srcBox = faceImage.getData( 0 );

                        dstBox.sliceStart = face;
                        //dstBox.numSlices = 1u;
                        dstBox.copyFrom( srcBox );
                        dstBox.numSlices = 6u;
                    }
                }

                //finalCubemap.save( outputFilename, 0u, numMipmaps );
                //auto uuid = StringUtil::getUUID() + Path::getFileExtension( faceNames[0] );

                auto texture = textureManager->createTexture(
                    textureName.c_str(), GpuPageOutStrategy::Discard, TextureFlags::RenderToTexture,
                    TextureTypes::TypeCube, ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME );

                texture->setResolution( faceImage.getWidth(), faceImage.getHeight(), 6u );
                texture->setPixelFormat( faceImage.getPixelFormat() );
                //texture->scheduleTransitionTo( Ogre::GpuResidency::Resident, finalCubemap );
                texture->scheduleReupload( finalCubemap );
                OGRE_DELETE( finalCubemap );

                //auto cacheFolder = applicationManager->getCachePath();
                //if( !StringUtil::isNullOrEmpty( cacheFolder ) )
                //{
                //    auto textureFilePath = cacheFolder + Path::separatorStr + textureName;
                //    faceImage.save( textureFilePath, 0, 2 );
                //}

                auto graphicsSystemTexture = create( textureName );
                auto ogreNextSystemTexture =
                    workphone::static_pointer_cast<CTextureOgreNext>( graphicsSystemTexture );
                ogreNextSystemTexture->setTexture( texture );
                return ogreNextSystemTexture;
            }

            return nullptr;
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return nullptr;
    }

    auto CTextureManagerOgreNext::createCubeMap( const Array<SmartPtr<ITexture>> &textures )
        -> SmartPtr<ITexture>
    {
        try
        {
            if( textures.empty() )
            {
                return nullptr;
            }

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto fileSystem = applicationManager->getFileSystem();
            WP_ASSERT( fileSystem );

            const char *faceNames[6] = { "_px", "_nx", "_py", "_ny", "_pz", "_nz" };

            std::string folderPath;
            const uint8_t maxNumMipmaps = 1;  // (uint8_t)atoi(argv[3]) + 1u;
            std::string extension = "";       // argv[2];
            auto outputFilename = Path::getWorkingDirectory() + "/output.ktx";  // argv[4];
            String filename;

            auto faceTex0 = textures.front();
            if( !faceTex0 )
            {
                return nullptr;
            }

            auto fileId = faceTex0->getFileSystemId();
            WP_ASSERT( !fileId.is_nil() );

            FileInfo fileInfo;
            if( fileSystem->findFileInfo( fileId, fileInfo ) )
            {
                folderPath = StringUtil::str( fileInfo.path );
                filename = StringUtil::str( fileInfo.fileName );
            }

            if( !folderPath.empty() && folderPath[folderPath.size() - 1u] != '/' )
            {
                folderPath.push_back( '/' );
            }

            using namespace Ogre;
            //char tmpBuffer[256];

            // LwString filename( LwString::FromEmptyPointer( tmpBuffer, sizeof( tmpBuffer ) ) );

            Image2 faceImage;
            // filename.a( "m", 0, faceNames[0], ".", extension );

            const bool bOpenSuccess = OgreUtil::loadIntoImage( filename, faceImage );

            if( !bOpenSuccess )
            {
                WP_LOG_ERROR( "Could not determine resolution nor pixel format for face: %s. Aborting", faceName0.c_str() );
                return nullptr;
            }

            const uint32_t width = faceImage.getWidth();
            const uint32_t height = faceImage.getHeight();
            const PixelFormatGpu pixelFormat = faceImage.getPixelFormat();

            uint8_t numMipmaps = PixelFormatGpuUtils::getMaxMipmapCount( width, height );
            numMipmaps = std::min( numMipmaps, maxNumMipmaps );
            const size_t requiredBytes = PixelFormatGpuUtils::calculateSizeBytes(
                width, height, 1u, 6u, pixelFormat, numMipmaps, 4u );

            void *data = OGRE_MALLOC_SIMD( requiredBytes, MEMCATEGORY_GENERAL );
            auto finalCubemap = OGRE_NEW Image2();
            finalCubemap->loadDynamicImage( data, width, height, 6u, TextureTypes::TypeCube, pixelFormat,
                                           true, numMipmaps );

            for( uint8_t mip = 0u; mip < numMipmaps; ++mip )
            {
                for( uint32_t face = 0u; face < 6u; ++face )
                {
                    auto currentFaceTex = textures[face];
                    if( currentFaceTex )
                    {
                        auto currentFileId = currentFaceTex->getFileSystemId();
                        WP_ASSERT( !currentFileId.is_nil() );

                        FileInfo currentFileInfo;
                        if( fileSystem->findFileInfo( currentFileId, currentFileInfo ) )
                        {
                            folderPath = StringUtil::str( currentFileInfo.path );
                            filename = StringUtil::str( currentFileInfo.fileName );
                        }

                        if( mip != 0u || face != 0u )
                        {
                            OgreUtil::loadIntoImage( filename.c_str(), faceImage );
                        }
                    }

                    TextureBox srcBox = faceImage.getData( 0u );
                    TextureBox dstBox = finalCubemap->getData( mip );

                    dstBox.sliceStart = face;
                    dstBox.numSlices = 1u;
                    dstBox.copyFrom( srcBox );
                    dstBox.numSlices = 6u;
                }
            }

            auto textureName = StringUtil::getUUID() + "_cubemap";
            auto textureManager = Ogre::Root::getSingletonPtr()->getRenderSystem()->getTextureGpuManager();

            auto texture = textureManager->createTexture(
                textureName.c_str(), Ogre::GpuPageOutStrategy::Discard, Ogre::TextureFlags::RenderToTexture,
                Ogre::TextureTypes::TypeCube, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME );

            texture->setResolution( width, height, 6u );
            texture->setPixelFormat( pixelFormat );
            texture->scheduleReupload( finalCubemap );

            auto graphicsSystemTexture = create( textureName );
            auto ogreNextSystemTexture =
                workphone::static_pointer_cast<CTextureOgreNext>( graphicsSystemTexture );
            ogreNextSystemTexture->setTexture( texture );
            return ogreNextSystemTexture;
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return nullptr;
    }

    auto CTextureManagerOgreNext::createCubeMap( SmartPtr<IMaterial> material ) -> SmartPtr<ITexture>
    {
        try
        {
            WP_LOG_ERROR(
                "CTextureManagerOgreNext::createSkyBoxCubeMap(IMaterial) is not implemented." );
        }
        catch( std::exception &e )
        {
            WP_LOG_ERROR( "Exception in CTextureManagerOgreNext::createSkyBoxCubeMap(IMaterial)" );
            WP_LOG_EXCEPTION( e );
        }

        return nullptr;
    }

    void CTextureManagerOgreNext::_getObject( void **ppObject ) const
    {
        WP_ASSERT( ppObject );

        if( auto root = Ogre::Root::getSingletonPtr() )
        {
            if( auto renderSystem = root->getRenderSystem() )
            {
                auto textureManager = renderSystem->getTextureGpuManager();
                *ppObject = textureManager;
            }
        }
    }

    void CTextureManagerOgreNext::queueReload( SmartPtr<ITexture> texture )
    {
        ScopedLock lock( this );
        m_reloadTextures.push( texture );
    }

    void CTextureManagerOgreNext::queueTransition( SmartPtr<ITexture> texture, s32 state )
    {
        auto pair = workphone::make_pair( texture, state );
        m_transitionTextures.push( pair );
    }

    bool CTextureManagerOgreNext::handleStateChanged( SmartPtr<IState> &state )
    {
        return TextureManager::handleStateChanged( state );
    }

    auto CTextureManagerOgreNext::TextureListener::handleEvent( EventType eventType,
                                                                hash_type eventValue,
                                                                const Array<Parameter> &arguments,
                                                                SmartPtr<ISharedObject> sender,
                                                                SmartPtr<ISharedObject> object,
                                                                SmartPtr<IEvent> event ) -> Parameter
    {
        return {};
    }

    void CTextureManagerOgreNext::TextureListener::loadingStateChanged( ISharedObject *sharedObject,
                                                                        LoadingState oldState,
                                                                        LoadingState newState )
    {
        if( newState == LoadingState::Loaded )
        {
        }
    }

    auto CTextureManagerOgreNext::TextureListener::destroy( void *ptr ) -> bool
    {
        return false;
    }

    void CTextureManagerOgreNext::TextureListener::setOwner( SmartPtr<CTextureManagerOgreNext> owner )
    {
        m_owner = owner;
    }

    SmartPtr<CTextureManagerOgreNext> CTextureManagerOgreNext::TextureListener::getOwner() const
    {
        auto p = m_owner.load();
        return p.lock();
    }

    CTextureManagerOgreNext::TextureListener::TextureListener() = default;

    CTextureManagerOgreNext::TextureListener::~TextureListener() = default;

    workphone::SmartPtr<workphone::render::CTextureManagerOgreNext>
    CTextureManagerOgreNext::StateListener::getOwner() const
    {
        auto p = m_owner.load();
        return p.lock();
    }

    void CTextureManagerOgreNext::StateListener::setOwner( SmartPtr<CTextureManagerOgreNext> owner )
    {
        m_owner = owner;
    }

    bool CTextureManagerOgreNext::StateListener::handleStateChanged( SmartPtr<IState> &state )
    {
        if( auto owner = getOwner() )
        {
            return owner->handleStateChanged( state );
        }

        return false;
    }

    bool CTextureManagerOgreNext::StateListener::handleStateMessage(
        const SmartPtr<IStateMessage> &message )
    {
        if( auto owner = getOwner() )
        {
            return owner->handleStateMessage( message );
        }

        return false;
    }

    CTextureManagerOgreNext::StateListener::StateListener() = default;

    CTextureManagerOgreNext::StateListener::~StateListener() = default;

}  // namespace workphone::render
