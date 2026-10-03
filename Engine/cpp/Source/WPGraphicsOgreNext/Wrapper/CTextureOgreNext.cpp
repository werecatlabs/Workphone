#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/Wrapper/CTextureOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CRenderTargetOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CRenderTextureOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CTextureManagerOgreNext.hpp>
#include <Workphone/Workphone.hpp>
#include <OgreStagingTexture.h>
#include <OgrePixelFormatGpuUtils.h>
#include <OgreTextureBox.h>
#include <OgreTextureGpuManager.h>
#include <Ogre.h>
#include <OgreRoot.h>

#include <cstdint>
#include <cstring>
#include <memory>

#if defined WP_PLATFORM_WIN32
#    if WP_BUILD_RENDERER_DX11
#        include <WPGraphicsOgreNext/Ogre/RenderSystems/Direct3D11/include/OgreD3D11TextureGpu.h>
#    elif WP_BUILD_RENDERER_GL3PLUS
#        include <WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/include/OgreGL3PlusPrerequisites.h>
#        include <WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/include/OgreGL3PlusTextureGpu.h>
#    elif WP_BUILD_RENDERER_OPENGL
#        include <WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/include/OgreGL3PlusPrerequisites.h>
#        include <WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/include/OgreGL3PlusTextureGpu.h>
#    endif
#elif defined WP_PLATFORM_LINUX
#    if WP_BUILD_RENDERER_OPENGL
#        include <WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/include/OgreGL3PlusPrerequisites.h>
#        include <WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/include/OgreGL3PlusTextureGpu.h>
#    endif
#endif

namespace workphone::render
{
    namespace
    {
        Ogre::TextureGpuManager *getOgreTextureManager()
        {
            auto root = Ogre::Root::getSingletonPtr();
            WP_ASSERT( root );
            if( !root )
            {
                return nullptr;
            }

            auto renderSystem = root->getRenderSystem();
            WP_ASSERT( renderSystem );
            if( !renderSystem )
            {
                return nullptr;
            }

            auto textureManager = renderSystem->getTextureGpuManager();
            WP_ASSERT( textureManager );
            return textureManager;
        }

        String getRenderTextureName( u32 id )
        {
            return String( "RTT_" ) + StringUtil::toString( id );
        }

        void destroyOgreImage( Ogre::Image2 *image )
        {
            if( image )
            {
                OGRE_DELETE image;
            }
        }

        bool isValidTextureSize( const Vector2I &size )
        {
            return size.X() > 0 && size.Y() > 0;
        }

        Vector2I getSupportedRenderTextureSize( const Vector2I &size )
        {
            auto textureSize = size;

#ifdef WP_PLATFORM_APPLE
            textureSize.X() = std::min( textureSize.X(), 2560 );
            textureSize.Y() = std::min( textureSize.Y(), 1440 );
#endif

            return textureSize;
        }

        bool isSupportedRenderTextureSize( const Vector2I &size )
        {
            constexpr auto maxRenderTextureSize = 4096;
            return isValidTextureSize( size ) && size.X() < maxRenderTextureSize &&
                   size.Y() < maxRenderTextureSize;
        }

        bool isRenderTargetTexture( u32 usageFlags, const SmartPtr<IRenderTarget> &renderTarget )
        {
            return ( usageFlags & (u32)TextureUsage::TU_RENDERTARGET ) != 0 || renderTarget;
        }

        void setTextureStateSize( SmartPtr<IStateContext> stateContext, const Vector2I &size )
        {
            if( !stateContext )
            {
                return;
            }

            if( auto state = stateContext->invalidateStateData<TextureStateData>( false ) )
            {
                state->size = size;
            }
        }

        void setRenderTargetStateSize( SmartPtr<IRenderTarget> renderTarget, const Vector2I &size )
        {
            if( !renderTarget || !renderTarget->isDerived<RenderTexture>() )
            {
                return;
            }

            auto renderTexture = workphone::static_pointer_cast<RenderTexture>( renderTarget );
            if( auto stateContext = renderTexture->getStateContext() )
            {
                if( auto data = stateContext->getStateData<RenderTargetStateData>() )
                {
                    data->size = size;
                }
            }
        }
    }  // namespace

    CTextureOgreNext::CTextureOgreNext()
    {
        setObjectFlag( OBJECT_FLAG_TRIGGER_EVENTS, true );

        static const auto name = String( "CTextureOgreNext" );
        setName( name );

        m_usageFlags = (u32)TextureUsage::TU_STATIC;
        createStateObject();
    }

    CTextureOgreNext::CTextureOgreNext( IResourceManager *resourceManager )
    {
        setObjectFlag( OBJECT_FLAG_TRIGGER_EVENTS, true );

        setResourceManager( resourceManager );

        static const auto name = String( "CTextureOgreNext" );
        setName( name );

        m_usageFlags = (u32)TextureUsage::TU_STATIC;
        createStateObject();
    }

    CTextureOgreNext::~CTextureOgreNext()
    {
        try
        {
            unload( nullptr );
            destroyStateObject();
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void CTextureOgreNext::save()
    {
        ScopedLock lock( this );

        if( auto texture = getTexture() )
        {
            auto filePath = getFilePath();
            WP_ASSERT( !StringUtil::isNullOrEmpty( filePath ) );
            if( StringUtil::isNullOrEmpty( filePath ) )
            {
                return;
            }

            texture->writeContentsToFile( filePath.c_str(), 0, 100 );
        }
    }

    void CTextureOgreNext::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            using namespace Ogre;

            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
            WP_ASSERT( graphicsSystem );

            auto factoryManager = applicationManager->getFactoryManagerPtr();
            WP_ASSERT( factoryManager );

            auto graphicsSystemTextureManager = workphone::static_pointer_cast<CTextureManagerOgreNext>(
                graphicsSystem->getTextureManager() );
            WP_ASSERT( graphicsSystemTextureManager );

            auto textureManager = getOgreTextureManager();
            WP_ASSERT( textureManager );
            if( !textureManager )
            {
                setLoadingState( LoadingState::Unloaded );
                return;
            }

            ScopedLock lock( this );

            WP_ASSERT( !isLoaded() );
            WP_ASSERT( m_texture == nullptr );

            setLoadingState( LoadingState::Loading );

            auto usageFlags = getUsageFlags();
            if( ( usageFlags & (u32)TextureUsage::TU_RENDERTARGET ) != 0 )
            {
                setObjectFlag( OBJECT_FLAG_TRIGGER_EVENTS, true );
                setObjectFlag( OBJECT_FLAG_GLOBAL_EVENTS, true );

                auto iblSpecularFlag = 0u;
                auto textureFlags = TextureFlags::RenderToTexture |   //
                                    TextureFlags::AllowAutomipmaps |  //
                                    iblSpecularFlag;

                auto size = getSize();
                if( size.X() <= 0 || size.Y() <= 0 )
                {
                    size = Vector2I( 512, 512 );
                }

                WP_ASSERT( size.X() > 0 );
                WP_ASSERT( size.Y() > 0 );

                const auto textureName = getRenderTextureName( m_ext++ ) + "_" + StringUtil::getUUID();
                auto rt =
                    textureManager->createTexture( textureName.c_str(), GpuPageOutStrategy::Discard,
                                                   textureFlags, TextureTypes::Type2D );
                WP_ASSERT( rt );
                if( !rt )
                {
                    setLoadingState( LoadingState::Unloaded );
                    return;
                }

                rt->scheduleTransitionTo( GpuResidency::OnStorage );
                rt->setResolution( size.X(), size.Y() );

                // mDynamicCubemap->setNumMipmaps(Ogre::PixelFormatGpuUtils::getMaxMipmapCount(resolution));
                // if (mIblQuality != MipmapsLowest)
                //{
                //	// Limit max mipmap to 16x16
                //	mDynamicCubemap->setNumMipmaps(mDynamicCubemap->getNumMipmaps() - 4u);
                // }

                //rt->setPixelFormat( PFG_RGBA16_FLOAT );
                rt->setPixelFormat( PFG_RGBA8_UNORM );
                //rt->setPixelFormat( PFG_RGBA8_UNORM_SRGB );
                rt->scheduleTransitionTo( GpuResidency::Resident );
                // rt->scheduleTransitionTo( GpuResidency::OnStorage );
                //graphicsSystemTextureManager->queueTransition( this, GpuResidency::Resident );

                setTexture( rt );
                WP_ASSERT( getTexture() == rt );

                void *textureHandle = nullptr;
                getTextureFinal( &textureHandle );
                WP_ASSERT( textureHandle || rt->getResidencyStatus() != GpuResidency::Resident );

                m_textureHandle = reinterpret_cast<size_t>( textureHandle );

                auto rtt = getRenderTarget();
                if( !rtt )
                {
                    auto renderTarget = factoryManager->make_ptr<CRenderTextureOgreNext>();
                    WP_ASSERT( renderTarget );
                    //renderTarget->setRenderTarget( rt );
                    renderTarget->setTexture( this );
                    rtt = renderTarget;
                    setRenderTarget( rtt );
                }
                else
                {
                    auto pRenderTarget = workphone::static_pointer_cast<CRenderTextureOgreNext>( rtt );
                    WP_ASSERT( pRenderTarget );
                    //pRenderTarget->setRenderTarget( rtt );
                    pRenderTarget->setTexture( this );
                }

                WP_ASSERT( getRenderTarget() );

                applicationManager->triggerEvent( EventType::Renderer, IEvent::renderTargetTextureLoaded,
                                                  Array<Parameter>(), this, this, nullptr );
            }
            else
            {
                WP_ASSERT( m_texture == nullptr );

                setObjectFlag( OBJECT_FLAG_TRIGGER_EVENTS, true );

                WP_ASSERT( textureManager );
                const auto textureName = getFilePath();

                WP_ASSERT( !StringUtil::isNullOrEmpty( textureName ) );
                if( StringUtil::isNullOrEmpty( textureName ) )
                {
                    setLoadingState( LoadingState::Unloaded );
                    return;
                }

                {
                    std::unique_ptr<Ogre::Image2, decltype( &destroyOgreImage )> imagePtr(
                        OGRE_NEW Ogre::Image2(), &destroyOgreImage );
                    WP_ASSERT( imagePtr );

                    const auto &resourceGrpName =
                        Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME;
                    imagePtr->load( textureName.c_str(), resourceGrpName );
                    WP_ASSERT( imagePtr->getWidth() > 0 );
                    WP_ASSERT( imagePtr->getHeight() > 0 );

                    auto textureFlags = Ogre::TextureFlags::AutomaticBatching |
                                        Ogre::TextureFlags::PrefersLoadingFromFileAsSRGB;

                    auto texture = textureManager->createOrRetrieveTexture(
                        textureName.c_str(), Ogre::GpuPageOutStrategy::Discard, textureFlags,
                        Ogre::TextureTypes::Type2D, resourceGrpName );
                    WP_ASSERT( texture );
                    if( !texture )
                    {
                        setLoadingState( LoadingState::Unloaded );
                        return;
                    }

                    // Schedule a transition for the texture to become resident in GPU memory
                    texture->scheduleTransitionTo( Ogre::GpuResidency::Resident, imagePtr.release(),
                                                   true, true );

                    setTexture( texture );
                    WP_ASSERT( getTexture() == texture );

                    void *textureHandle = nullptr;
                    getTextureFinal( &textureHandle );
                    WP_ASSERT( textureHandle ||
                               texture->getResidencyStatus() != Ogre::GpuResidency::Resident );

                    m_textureHandle = reinterpret_cast<size_t>( textureHandle );

                    auto size = Vector2I( texture->getWidth(), texture->getHeight() );
                    //WP_ASSERT( size.X() > 0 );
                    //WP_ASSERT( size.Y() > 0 );
                }
            }

            WP_ASSERT( m_texture );
            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
            setLoadingState( LoadingState::Unloaded );
        }
    }

    void CTextureOgreNext::reload( SmartPtr<ISharedObject> data )
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instance();
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void CTextureOgreNext::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Unloading );

            ScopedLock lock( this );

            removeObjectListeners();

            auto textureLoadingOptions = workphone::static_pointer_cast<TextureLoadingOptions>( data );

            auto destroyViewports = true;
            auto destroyTextureObject = true;

            if( textureLoadingOptions )
            {
                destroyViewports = textureLoadingOptions->unloadViewports;
                destroyTextureObject = textureLoadingOptions->reloadTextureObject;
            }

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            destroyStateObject();

            auto usageFlags = getUsageFlags();
            if( ( usageFlags & (u32)TextureUsage::TU_RENDERTARGET ) != 0 )
            {
                applicationManager->triggerEvent( EventType::Renderer,
                                                  IEvent::renderTargetTextureUnloaded,
                                                  Array<Parameter>(), this, this, nullptr );
            }

            if( destroyTextureObject )
            {
                if( auto texture = getTexture() )
                {
                    if( auto textureManager = getOgreTextureManager() )
                    {
                        textureManager->destroyTexture( texture );
                    }

                    setTexture( nullptr );
                }
            }

            if( destroyViewports )
            {
                Texture::unload( data );
            }

            destroyStateObject();

            m_textureHandle = 0;
            setLoadingState( LoadingState::Unloaded );
            WP_ASSERT( !destroyTextureObject || getTexture() == nullptr );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
            setLoadingState( LoadingState::Unloaded );
        }
    }

    void CTextureOgreNext::copyData( void *data, const Vector2I &size )
    {
        if( !isValidTextureSize( size ) )
        {
            return;
        }

        WP_ASSERT( data );
        if( !data )
        {
            return;
        }

        auto texture = getTexture();
        WP_ASSERT( texture );
        if( !texture )
        {
            return;
        }

        const auto width = static_cast<Ogre::uint32>( size.X() );
        const auto height = static_cast<Ogre::uint32>( size.Y() );
        const auto pixelFormat = texture->getPixelFormat();

        auto textureManager = texture->getTextureManager();
        WP_ASSERT( textureManager );
        if( !textureManager )
        {
            return;
        }

        if( texture->getWidth() != width || texture->getHeight() != height )
        {
            texture->scheduleTransitionTo( Ogre::GpuResidency::OnStorage );
            texture->setResolution( width, height );
            texture->scheduleTransitionTo( Ogre::GpuResidency::Resident );
            setSize( size );
        }

        Ogre::StagingTexture *stagingTexture =
            textureManager->getStagingTexture( width, height, 1u, 1u, pixelFormat );
        WP_ASSERT( stagingTexture );
        if( !stagingTexture )
        {
            return;
        }

        stagingTexture->startMapRegion();
        Ogre::TextureBox textureBox = stagingTexture->mapRegion( width, height, 1u, 1u, pixelFormat );
        if( !textureBox.data )
        {
            stagingTexture->stopMapRegion();
            textureManager->removeStagingTexture( stagingTexture );
            return;
        }

        const auto bytesPerPixel =
            static_cast<size_t>( Ogre::PixelFormatGpuUtils::getBytesPerPixel( pixelFormat ) );
        const auto srcBytesPerRow = static_cast<size_t>( width ) * bytesPerPixel;
        auto srcData = static_cast<const Ogre::uint8 *>( data );
        auto dstData = static_cast<Ogre::uint8 *>( textureBox.data );
        for( Ogre::uint32 y = 0u; y < height; ++y )
        {
            std::memcpy( dstData, srcData, srcBytesPerRow );
            srcData += srcBytesPerRow;
            dstData += textureBox.bytesPerRow;
        }

        stagingTexture->stopMapRegion();
        stagingTexture->upload( textureBox, texture, 0u );
        textureManager->removeStagingTexture( stagingTexture );
        texture->notifyDataIsReady();
    }

    void CTextureOgreNext::_getObject( void **ppObject ) const
    {
        WP_ASSERT( ppObject );
        if( !ppObject )
        {
            return;
        }

        *ppObject = m_texture;
        WP_ASSERT( *ppObject || !isLoaded() );
    }

    auto CTextureOgreNext::getActualSize() const -> Vector2I
    {
        ScopedLock lock( this );

        if( auto texture = getTexture() )
        {
            auto width = texture->getWidth();
            auto height = texture->getHeight();

            return Vector2I( width, height );
        }

        return Vector2I::zero();
    }

    auto CTextureOgreNext::getTexture() const -> Ogre::TextureGpu *
    {
        //WP_ASSERT( m_texture );
        return m_texture;
    }

    void CTextureOgreNext::createStateObject()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
        WP_ASSERT( graphicsSystem );

        auto textureManager = graphicsSystem->getTextureManager();
        WP_ASSERT( textureManager );

        auto factoryManager = applicationManager->getFactoryManagerPtr();
        WP_ASSERT( factoryManager );

        auto stateContext = textureManager->getStateContextPtr();
        WP_ASSERT( stateContext );

        auto state = factoryManager->make_ptr<State>();
        WP_ASSERT( state );
        state->setId( getId() );
        state->setOwner( this );
        stateContext->addState( state );

        auto textureState = factoryManager->make_ptr<TextureStateData>();
        WP_ASSERT( textureState );
        textureState->size = Vector2I( 512, 512 );
        state->setData( textureState );

        auto rttState = factoryManager->make_ptr<State>();
        WP_ASSERT( rttState );
        rttState->setId( getId() );
        rttState->setOwner( this );
        stateContext->addState( rttState );

        auto rttStateData = factoryManager->make_ptr<RenderTextureState>();
        WP_ASSERT( rttStateData );
        rttState->setData( rttStateData );
    }

    void CTextureOgreNext::setTexture( Ogre::TextureGpu *texture )
    {
        m_texture = texture;
        m_textureHandle = 0;
    }

    void CTextureOgreNext::getTextureGPU( void **ppTexture ) const
    {
        WP_ASSERT( ppTexture );
        if( !ppTexture )
        {
            return;
        }

        *ppTexture = m_texture;
        WP_ASSERT( *ppTexture || !isLoaded() );
    }

    void CTextureOgreNext::getTextureFinal( void **ppTexture ) const
    {
        WP_ASSERT( ppTexture );
        if( !ppTexture )
        {
            return;
        }

        *ppTexture = nullptr;

        if( auto ogreTexture = getTexture() )
        {
            if( ogreTexture->isDataReady() )
            {
                if( ogreTexture->getResidencyStatus() != Ogre::GpuResidency::Resident )
                {
                    *ppTexture = nullptr;
                    return;
                }
            }

#if defined WP_PLATFORM_WIN32
#    if WP_BUILD_RENDERER_DX11
            auto texture = static_cast<Ogre::D3D11TextureGpu *>( ogreTexture );
            WP_ASSERT( texture );
            auto tex = texture->getDefaultDisplaySrv();
            WP_ASSERT( tex );
            *ppTexture = tex;
#    elif WP_BUILD_RENDERER_GL3PLUS
            auto texture = (Ogre::GL3PlusTextureGpu *)ogreTexture;
            WP_ASSERT( texture );
            auto tex = texture->getDisplayTextureName();
            WP_ASSERT( tex != 0 );
            *ppTexture = reinterpret_cast<void *>( static_cast<uintptr_t>( tex ) );
#    elif WP_BUILD_RENDERER_OPENGL
            auto texture = (Ogre::GL3PlusTextureGpu *)ogreTexture;
            WP_ASSERT( texture );
            auto tex = texture->getFinalTextureName();
            WP_ASSERT( tex != 0 );
            *ppTexture = reinterpret_cast<void *>( static_cast<uintptr_t>( tex ) );
#    endif
#endif
        }

        WP_ASSERT( *ppTexture || !m_texture ||
                   getTexture()->getResidencyStatus() != Ogre::GpuResidency::Resident );
    }

    auto CTextureOgreNext::getTextureHandle() const -> size_t
    {
        if( m_textureHandle.load() == 0 )
        {
            void *iTexture = 0;
            getTextureFinal( (void **)&iTexture );
            m_textureHandle = reinterpret_cast<size_t>( iTexture );
        }

        WP_ASSERT(
            m_textureHandle.load() || !isLoaded() ||
            ( getTexture() && getTexture()->getResidencyStatus() != Ogre::GpuResidency::Resident ) );
        return m_textureHandle;
    }

    auto CTextureOgreNext::getUsageFlags() const -> u32
    {
        return m_usageFlags;
    }

    void CTextureOgreNext::setUsageFlags( u32 usageFlags )
    {
        m_usageFlags = usageFlags;
    }

    void CTextureOgreNext::setSizeCoroutine( ICoroutineData::PullType &coroutine )
    {
        WaitForSeconds( coroutine, 1.0f );

        if( auto texture = getTexture() )
        {
            texture->scheduleTransitionTo( Ogre::GpuResidency::OnStorage );
        }

        WaitForSeconds( coroutine, 1.0f );

        if( auto texture = getTexture() )
        {
            auto textureSize = getSize();
            texture->setResolution( textureSize.X(), textureSize.Y() );
        }

        //WaitForSeconds( coroutine, 1.0f );

        if( auto texture = getTexture() )
        {
            texture->scheduleTransitionTo( Ogre::GpuResidency::Resident );
        }
    }

    bool CTextureOgreNext::handleStateChanged( SmartPtr<IState> &state )
    {
        if( auto stateData = state->getData() )
        {
            if( stateData->isDerived<TextureStateData>() )
            {
                auto textureStateData = workphone::static_pointer_cast<TextureStateData>( stateData );

                if( !isValidTextureSize( textureStateData->size ) )
                {
                    return false;
                }

                auto renderTarget = getRenderTarget();
                setRenderTargetStateSize( renderTarget, textureStateData->size );

                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );
                if( !applicationManager )
                {
                    return false;
                }

                auto graphicsSystem = applicationManager->getGraphicsSystem();
                WP_ASSERT( graphicsSystem );
                if( !graphicsSystem )
                {
                    return false;
                }

                auto timer = applicationManager->getTimer();
                WP_ASSERT( timer );
                if( !timer )
                {
                    return false;
                }

                auto renderTask = graphicsSystem->getRenderTask();
                auto task = Thread::getCurrentTask();

                const auto &loadingState = getLoadingState();
                if( loadingState != LoadingState::Loaded )
                {
                    return false;
                }

                auto usageFlags = getUsageFlags();
                if( !isRenderTargetTexture( usageFlags, renderTarget ) )
                {
                    return false;
                }

                constexpr auto renderTextureResizeDelay = 1.0;
                constexpr auto renderTextureResizeCooldown = 3.0;
                const auto time = timer->getTime();

                auto textureSize = getSupportedRenderTextureSize( textureStateData->size );
                if( !isSupportedRenderTextureSize( textureSize ) )
                {
                    return false;
                }

                if( getActualSize() != textureStateData->size )
                {
                    if( m_nextResize < time )
                    {
                        auto jobQueue = applicationManager->getJobQueue();

                        auto func = std::bind( &CTextureOgreNext::setSizeCoroutine, this,
                                               std::placeholders::_1 );
                        jobQueue->startCoroutine( func );

                        m_nextResize = time + renderTextureResizeCooldown;

                        return true;
                    }
                }
            }
        }

        return false;
    }

    bool CTextureOgreNext::handleStateMessage( const SmartPtr<IStateMessage> &message )
    {
        return false;
    }

    CTextureOgreNext::TextureLoadingOptions::~TextureLoadingOptions() = default;

    CTextureOgreNext::TextureLoadingOptions::TextureLoadingOptions() = default;

    CTextureOgreNext::TextureGpuListener::TextureGpuListener() = default;
    CTextureOgreNext::TextureGpuListener::~TextureGpuListener() = default;

    void CTextureOgreNext::TextureGpuListener::notifyTextureChanged(
        Ogre::TextureGpu *texture, Ogre::TextureGpuListener::Reason reason, void *extraData )
    {
        WP_ASSERT( texture );

        if( auto owner = getOwner() )
        {
            auto currentTexture = owner->getTexture();
            if( texture == currentTexture )
            {
                if( reason == Deleted )
                {
                    owner->setTexture( nullptr );
                }
            }
        }
    }

    SmartPtr<CTextureOgreNext> CTextureOgreNext::TextureGpuListener::getOwner() const
    {
        auto p = m_owner.load();
        return p.lock();
    }

    void CTextureOgreNext::TextureGpuListener::setOwner( SmartPtr<CTextureOgreNext> owner )
    {
        WP_ASSERT( owner );
        m_owner = owner;
    }

}  // namespace workphone::render
