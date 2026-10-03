#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/Wrapper/CWindowOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CViewportOgreNext.hpp>
#include <WPGraphicsOgreNext/OgreUtil.hpp>
#include <Workphone/Workphone.hpp>
#include <OgreWindow.h>
#include <Ogre.h>

#if defined WP_PLATFORM_WIN32
#    include <WPGraphicsOgreNext/Window/Windows/WindowWin32.hpp>
#    include <WPGraphicsOgreNext/Window/Windows/WindowWin32Alt.hpp>
#elif defined WP_PLATFORM_APPLE
#    include <WPGraphicsOgreNext/Window/Apple/WindowMacOS.hpp>
#endif

#ifdef WP_PLATFORM_WIN32
#    include <windows.h>
#endif

namespace workphone::render
{

    WP_CLASS_REGISTER_DERIVED( workphone::render, CWindowOgreNext,
                               CRenderTargetOgreNext<GraphicsWindow> );
    WP_CLASS_REGISTER_DERIVED( workphone::render, CWindowOgreNext::WindowTexture, Texture );

    CWindowOgreNext::CWindowOgreNext()
    {
        try
        {
            static const String name = "CWindowOgreNext";
            setName( name );

            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto stateManager = applicationManager->getStateManagerPtr();
            WP_ASSERT( stateManager );

            auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
            WP_ASSERT( graphicsSystem );

            auto factoryManager = applicationManager->getFactoryManagerPtr();
            WP_ASSERT( factoryManager );

            auto stateContext = stateManager->addStateContext();
            stateContext->setOwner( this );
            stateContext->setTaskId( TaskId::Render );
            setStateContext( stateContext );

            auto stateListener = factoryManager->make_ptr<RenderTargetListener>();
            stateListener->setOwner( this );
            setStateListener( stateListener );
            stateContext->addStateListener( stateListener );

            auto state = factoryManager->make_ptr<State>();
            state->setId( getId() );
            state->setOwner( this );
            stateContext->addState( state );

            auto stateData = factoryManager->make_ptr<WindowStateData>();
            state->setData( stateData );

            auto renderTargetState = factoryManager->make_ptr<State>();
            renderTargetState->setId( getId() );
            renderTargetState->setOwner( this );
            stateContext->addState( renderTargetState );

            auto renderTargetStateData = factoryManager->make_ptr<RenderTargetStateData>();
            renderTargetState->setData( renderTargetStateData );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    CWindowOgreNext::~CWindowOgreNext()
    {
    }

    void CWindowOgreNext::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

            auto root = Ogre::Root::getSingletonPtr();

            WP_ASSERT( root );
            WP_ASSERT( root->isInitialised() );

            Ogre::NameValuePairList params;

            auto handle = getWindowHandleAsString();

#if defined WP_PLATFORM_WIN32
#    if 1
            if( !StringUtil::isNullOrEmpty( handle ) )
            {
                params["externalWindowHandle"] = StringUtil::str( handle );
            }
            else
            {
                m_windowWin32 = new WindowWin32Alt;
                m_windowWin32->setRenderWindow( this );
                m_windowWin32->setName( getName() );
                m_windowWin32->load( nullptr );

                size_t windowHandle = 0;
                m_windowWin32->getWindowHandle( &windowHandle );

                params["externalWindowHandle"] = StringUtil::str( StringUtil::toString( windowHandle ) );
            }
#    else
            if( !StringUtil::isNullOrEmpty( handle ) )
            {
                params["externalWindowHandle"] = OgreUtil::toString( handle );
            }
#    endif
#elif defined WP_PLATFORM_APPLE
#    if 1
            if( !StringUtil::isNullOrEmpty( handle ) )
            {
                params["externalWindowHandle"] = OgreUtil::toString( handle );
            }
            else
            {
                m_osWindow = new WindowMacOS;

                m_osWindow->setWindow( this );
                m_osWindow->load( nullptr );

                size_t windowHandle = 0;
                m_osWindow->getWindowHandle( &windowHandle );

                params["externalWindowHandle"] = StringUtil::toString( windowHandle );
            }

            params["contentScalingFactor"] = StringUtil::toString( 1.0f );
#    else
            if( !StringUtil::isNullOrEmpty( handle ) )
            {
                params["externalWindowHandle"] = OgreUtil::toString( handle );
            }
#    endif
#else
            if( !StringUtil::isNullOrEmpty( handle ) )
            {
                params["externalWindowHandle"] = OgreUtil::toString( handle );
            }
#endif

            auto title = getTitle();
            auto size = getSize();
            auto fullscreen = isFullScreen();

            m_window = root->createRenderWindow( title.c_str(), static_cast<u32>( size.X() ),
                                                 static_cast<u32>( size.Y() ), fullscreen, &params );

            m_windowEventListener = new WindowListener();
            m_windowEventListener->setOwner( this );
            Ogre::WindowEventUtilities::addWindowEventListener( m_window, m_windowEventListener );

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void CWindowOgreNext::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            const auto &loadingState = getLoadingState();
            if( loadingState != LoadingState::Unloaded )
            {
                setLoadingState( LoadingState::Unloading );

                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                if( m_windowEventListener )
                {
                    Ogre::WindowEventUtilities::removeWindowEventListener( m_window,
                                                                           m_windowEventListener );
                    delete m_windowEventListener;
                    m_windowEventListener = nullptr;
                }

                m_window = nullptr;

#if defined WP_PLATFORM_WIN32
                if( m_windowWin32 )
                {
                    m_windowWin32->unload( nullptr );
                    delete m_windowWin32;
                    m_windowWin32 = nullptr;
                }
#elif defined WP_PLATFORM_APPLE
                if( m_osWindow )
                {
                    m_osWindow->unload( nullptr );
                    delete m_osWindow;
                    m_osWindow = nullptr;
                }
#endif

                CRenderTargetOgreNext<GraphicsWindow>::unload( data );

                setLoadingState( LoadingState::Unloaded );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void CWindowOgreNext::setupWindow( Ogre::Window *window )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

            m_window = window;

            m_windowEventListener = new WindowListener();
            m_windowEventListener->setOwner( this );
            Ogre::WindowEventUtilities::addWindowEventListener( m_window, m_windowEventListener );

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void CWindowOgreNext::update()
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        WP_ASSERT( graphicsSystem );

        ScopedLock lock( this );

        if( m_window )
        {
            auto t = 0;
            auto l = 0;
            auto w = 0u;
            auto h = 0u;

            m_window->getMetrics( w, h, l, t );

            if( auto stateContext = getStateContext() )
            {
                if( auto data = stateContext->invalidateStateData<WindowStateData>( false ) )
                {
                    data->position = Vector2I( l, t );

                    if( m_window->isVisible() )
                    {
                        data->flags |= WINDOW_FLAG_VISIBLE;
                    }
                    else
                    {
                        data->flags &= ~WINDOW_FLAG_VISIBLE;
                    }

                    if( m_window->isClosed() )
                    {
                        data->flags |= WINDOW_FLAG_IS_CLOSED;
                    }
                    else
                    {
                        data->flags &= ~WINDOW_FLAG_IS_CLOSED;
                    }

                    if( m_window->isPrimary() )
                    {
                        data->flags |= WINDOW_FLAG_IS_PRIMARY;
                    }
                    else
                    {
                        data->flags &= ~WINDOW_FLAG_IS_PRIMARY;
                    }

                    //if ( m_window->isDeactivatedOnFocusChange() )
                    //{
                    //    data->flags |= WINDOW_FLAG_DEACTIVATE_ON_FOCUS_CHANGE;
                    //}
                    //else
                    //{
                    //    data->flags &= ~WINDOW_FLAG_DEACTIVATE_ON_FOCUS_CHANGE;
                    //}

                    if( m_window->isFullscreen() )
                    {
                        data->flags |= WINDOW_FLAG_FULLSCREEN;
                    }
                    else
                    {
                        data->flags &= ~WINDOW_FLAG_FULLSCREEN;
                    }
                }

                if( auto data = stateContext->invalidateStateData<RenderTargetStateData>( false ) )
                {
                    data->size = Vector2I( w, h );
                }
            }
        }

        if( !m_eventQueue.empty() )
        {
            SmartPtr<IGraphicsWindowEvent> event;
            while( m_eventQueue.try_pop( event ) )
            {
                auto listeners = getListeners();
                for( auto listener : listeners )
                {
                    listener->handleEvent( event );
                }

                handleQueuedEvent( event );
            }
        }
    }

    auto CWindowOgreNext::addViewport( hash_type id, SmartPtr<IGraphicsCamera> camera, s32 ZOrder,
                                       f32 left, f32 top, f32 width, f32 height ) -> SmartPtr<IViewport>
    {
        try
        {
            ScopedLock lock( this );

            auto applicationManager = core::IApplicationManager::instance();
            auto factoryManager = applicationManager->getFactoryManager();

            Ogre::Camera *ogreCamera = nullptr;
            if( camera )
            {
                camera->_getObject( reinterpret_cast<void **>( &ogreCamera ) );
            }

            auto viewport = factoryManager->make_ptr<CViewportOgreNext>();

            viewport->setRenderTarget( this );
            viewport->setCamera( camera );

            viewport->setViewportId( (u32)id );

            if( auto stateContext = getStateContext() )
            {
                if( auto data = stateContext->getStateData<RenderTargetStateData>() )
                {
                    data->viewports.push_back( viewport );
                }
            }

            return viewport;
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return nullptr;
    }

    void CWindowOgreNext::handleEvent( SmartPtr<IGraphicsWindowEvent> event )
    {
        m_eventQueue.push( event );
    }

    //void CWindowOgreNext::setFullscreen( bool fullScreen, unsigned int width, unsigned int height )
    //{
    //    if( m_window )
    //    {
    //        m_window->requestFullscreenSwitch( fullScreen, false, 0, width, height, 1, 60 );
    //    }
    //}

    void CWindowOgreNext::destroy()
    {
        if( m_window )
        {
            m_window->destroy();
        }
    }

    void CWindowOgreNext::resize( unsigned int width, unsigned int height )
    {
        try
        {
            if( isThreadSafe() )
            {
                if( m_window )
                {
                    m_window->requestResolution( width, height );
                }
            }
            else
            {
                auto applicationManager = core::IApplicationManager::instance();
                auto factoryManager = applicationManager->getFactoryManager();

                auto message = factoryManager->make_ptr<StateMessageVector2I>();
                message->setType( RESIZE_HASH );
                message->setValue( Vector2I( width, height ) );

                if( auto stateContext = getStateContext() )
                {
                    stateContext->addMessage( TaskId::Render, message );
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void CWindowOgreNext::windowMovedOrResized()
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto graphicsSystem = applicationManager->getGraphicsSystem();
            WP_ASSERT( graphicsSystem );

            auto materialManager = graphicsSystem->getMaterialManager();
            WP_ASSERT( materialManager );

            auto factoryManager = applicationManager->getFactoryManager();
            WP_ASSERT( factoryManager );

            auto renderTask = graphicsSystem->getRenderTask();
            auto stateTask = graphicsSystem->getStateTask();
            auto task = Thread::getCurrentTask();

            const auto &loadingState = getLoadingState();
            if( loadingState == LoadingState::Loaded && task == renderTask )
            {
                ScopedLock lock( this );

                if( m_window )
                {
                    m_window->windowMovedOrResized();
                }

                if( m_window )
                {
                    auto t = 0;
                    auto l = 0;
                    auto w = 0u;
                    auto h = 0u;

                    m_window->getMetrics( w, h, l, t );

                    if( auto stateContext = getStateContext() )
                    {
                        if( auto data = stateContext->getStateData<WindowStateData>() )
                        {
                            data->position = Vector2I( l, t );
                        }

                        if( auto data = stateContext->getStateData<RenderTargetStateData>() )
                        {
                            data->size = Vector2I( w, h );
                        }
                    }
                }

                applicationManager->triggerEvent( EventType::Window, IEvent::windowMovedOrResized, {},
                                                  this, this, nullptr );
            }
            else
            {
                auto message = factoryManager->make_ptr<StateMessage>();
                message->setType( MOVED_OR_RESIZED_HASH );

                if( auto stateContext = getStateContext() )
                {
                    stateContext->addMessage( stateTask, message );
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void CWindowOgreNext::maximize()
    {
        ScopedLock lock( this );

#ifdef WP_PLATFORM_WIN32
        if( auto window = getWindow() )
        {
            HWND hwnd;

            window->getCustomAttribute( "WINDOW", &hwnd );
            ShowWindow( hwnd, SW_SHOWMAXIMIZED );
        }
#endif

        if( m_window )
        {
            m_window->windowMovedOrResized();

            auto t = 0;
            auto l = 0;
            auto w = 0u;
            auto h = 0u;

            m_window->getMetrics( w, h, l, t );

            if( auto stateContext = getStateContext() )
            {
                if( auto data = stateContext->invalidateStateData<WindowStateData>( false ) )
                {
                    data->position = Vector2I( l, t );
                }

                if( auto data = stateContext->invalidateStateData<RenderTargetStateData>( false ) )
                {
                    data->size = Vector2I( w, h );
                }
            }
        }
    }

    void CWindowOgreNext::reposition( int left, int top )
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto graphicsSystem = applicationManager->getGraphicsSystem();
            WP_ASSERT( graphicsSystem );

            auto materialManager = graphicsSystem->getMaterialManager();
            WP_ASSERT( materialManager );

            auto factoryManager = applicationManager->getFactoryManager();
            WP_ASSERT( factoryManager );

            auto renderTask = graphicsSystem->getRenderTask();
            auto stateTask = graphicsSystem->getStateTask();
            auto task = Thread::getCurrentTask();

            const auto &loadingState = getLoadingState();
            if( loadingState == LoadingState::Loaded && task == renderTask )
            {
                if( m_window )
                {
                    m_window->reposition( left, top );
                }
            }
            else
            {
                auto message = factoryManager->make_ptr<StateMessageVector2I>();
                message->setType( REPOSITION_HASH );
                message->setValue( Vector2I( left, top ) );

                if( auto stateContext = getStateContext() )
                {
                    stateContext->addMessage( stateTask, message );
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void CWindowOgreNext::getCustomAttribute( const String &name, void *pData )
    {
        m_window->getCustomAttribute( name.c_str(), pData );
    }

    void CWindowOgreNext::getWindowHandle( void *pData )
    {
#if defined WP_PLATFORM_WIN32
        getCustomAttribute( "WINDOW", pData );
#elif defined WP_PLATFORM_APPLE
#    if defined WP_BUILD_RENDERER_METAL
        if( m_osWindow )
        {
            m_osWindow->getWindowHandle( pData );
            return;
        }

        getCustomAttribute( "WINDOW", (void *)pData );
        if( !pData )
        {
            getCustomAttribute( "MetalDevice", (void *)&pData );
        }
#    else
        getCustomAttribute( "WINDOW", pData );
        if( windowHnd == 0 )
        {
            getCustomAttribute( "RENDERDOC", pData );
        }
#    endif
#else
        getCustomAttribute( "WINDOW", pData );
#endif
    }

    void CWindowOgreNext::getDeviceHandle( void *pData )
    {
    }

    void CWindowOgreNext::handleQueuedEvent( SmartPtr<IGraphicsWindowEvent> event )
    {
        auto windowEvent = workphone::dynamic_pointer_cast<WindowMessageData>( event );
        auto uMsg = windowEvent->getMessage();
        auto renderWindow = this;

#ifdef WP_PLATFORM_WIN32
        switch( uMsg )
        {
        case WM_CLOSE:
        {
            bool close = true;

            if( renderWindow )
            {
                auto listeners = renderWindow->getListeners();
                for( auto listener : listeners )
                {
                    auto retValue = listener->handleEvent(
                        EventType::Window, IGraphicsWindowListener::windowClosingHash,
                        Array<Parameter>(), nullptr, nullptr, nullptr );

                    if( !retValue.getBool() )
                    {
                        close = false;
                    }
                }
            }

            if( !close )
            {
                return;
            }

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            applicationManager->setQuit( true );
        }
        break;
        case WM_ENTERSIZEMOVE:
            // log->logMessage("WM_ENTERSIZEMOVE");
            break;
        case WM_EXITSIZEMOVE:
            // log->logMessage("WM_EXITSIZEMOVE");
            break;
        case WM_MOVE:
        {
            // log->logMessage("WM_MOVE");
            windowMovedOrResized();

            auto listeners = renderWindow->getListeners();
            for( auto listener : listeners )
            {
                //listener->windowMoved( win );

                auto retValue = listener->handleEvent( EventType::Window,
                                                       IGraphicsWindowListener::windowMovedHash,
                                                       Array<Parameter>(), nullptr, nullptr, nullptr );
            }
        }
        break;
        case WM_DISPLAYCHANGE:
        {
            windowMovedOrResized();

            auto listeners = renderWindow->getListeners();
            for( auto listener : listeners )
            {
                //listener->windowResized( win );

                auto retValue = listener->handleEvent( EventType::Window,
                                                       IGraphicsWindowListener::windowResizedHash,
                                                       Array<Parameter>(), nullptr, nullptr, nullptr );
            }
        }
        break;
        case WM_SIZE:
        {
            // log->logMessage("WM_SIZE");
            windowMovedOrResized();

            auto listeners = renderWindow->getListeners();
            for( auto listener : listeners )
            {
                //listener->windowResized( win );

                auto retValue = listener->handleEvent( EventType::Window,
                                                       IGraphicsWindowListener::windowResizedHash,
                                                       Array<Parameter>(), nullptr, nullptr, nullptr );
            }
        }
        break;

        default:
        {
        }
        };
#endif
    }

    void CWindowOgreNext::_getObject( void **ppObject ) const
    {
        *ppObject = m_window;
    }

    auto CWindowOgreNext::getWindow() const -> Ogre::Window *
    {
        return m_window;
    }

    void CWindowOgreNext::setWindow( Ogre::Window *window )
    {
        m_window = window;
    }

    bool CWindowOgreNext::handleStateMessage( const SmartPtr<IStateMessage> &message )
    {
        if( message->isExactly<StateMessage>() )
        {
            WP_ASSERT( workphone::dynamic_pointer_cast<StateMessage>( message ) );
            auto stateMessage = workphone::static_pointer_cast<StateMessage>( message );
            auto type = stateMessage->getType();

            if( type == MOVED_OR_RESIZED_HASH )
            {
                windowMovedOrResized();
            }
        }
        else if( message->isExactly<StateMessageVector2I>() )
        {
            WP_ASSERT( workphone::dynamic_pointer_cast<StateMessageVector2I>( message ) );
            auto stateMessage = workphone::static_pointer_cast<StateMessageVector2I>( message );
            auto type = stateMessage->getType();
            auto value = stateMessage->getValue();

            if( type == RESIZE_HASH )
            {
                resize( static_cast<u32>( value.X() ), static_cast<u32>( value.Y() ) );
            }
        }

        return false;
    }

    bool CWindowOgreNext::handleStateChanged( SmartPtr<IState> &state )
    {
        auto viewports = getViewports();
        for( auto &viewport : viewports )
        {
            if( viewport )
            {
                if( viewport->handleStateChanged( state ) )
                {
                    return true;
                }
            }
        }

        if( state && state->getOwnerPtr() == this )
        {
            auto stateData = state->getData();
            if( stateData->isDerived<WindowStateData>() )
            {
                auto renderWindowState = workphone::static_pointer_cast<WindowStateData>( stateData );

                const auto &flags = renderWindowState->flags;
                auto visible = BitUtil::getFlagValue( flags, IGraphicsWindow::WINDOW_FLAG_VISIBLE );

                if( auto window = getWindow() )
                {
                    window->_setVisible( visible );

                    return true;
                }
            }
            else if( stateData->isDerived<RenderTargetStateData>() )
            {
                auto renderTargetState =
                    workphone::static_pointer_cast<RenderTargetStateData>( stateData );

                if( auto window = getWindow() )
                {
                    auto w = window->getWidth();
                    auto h = window->getHeight();

                    auto actualSize = Vector2I( w, h );
                    renderTargetState->size = actualSize;

                    return true;
                }
            }
        }

        return false;
    }

    SmartPtr<ITexture> CWindowOgreNext::getTexture() const
    {
        if( m_window )
        {
            auto texture = workphone::make_ptr<WindowTexture>();
            texture->setRenderTexture( m_window->getTexture() );
            texture->load( nullptr );
            return texture;
        }

        return nullptr;
    }

    auto CWindowOgreNext::WindowListener::windowClosing( Ogre::Window *rw ) -> bool
    {
        (void)rw;

        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        applicationManager->setQuit( true );

        return true;
    }

    auto CWindowOgreNext::WindowListener::getOwner() const -> SmartPtr<CWindowOgreNext>
    {
        auto p = m_owner.load();
        return p.lock();
    }

    void CWindowOgreNext::WindowListener::setOwner( SmartPtr<CWindowOgreNext> owner )
    {
        m_owner = owner;
    }

    CWindowOgreNext::WindowListener::WindowListener() = default;

    CWindowOgreNext::WindowListener::~WindowListener() = default;

    CWindowOgreNext::WindowTexture::WindowTexture() = default;
    CWindowOgreNext::WindowTexture::~WindowTexture() = default;

    void CWindowOgreNext::WindowTexture::load( SmartPtr<ISharedObject> data )
    {
        (void)data;
        setLoadingState( LoadingState::Loaded );
    }

    void CWindowOgreNext::WindowTexture::unload( SmartPtr<ISharedObject> data )
    {
        (void)data;
        setLoadingState( LoadingState::Unloaded );
    }

    void CWindowOgreNext::WindowTexture::setRenderTexture( Ogre::TextureGpu *texture )
    {
        m_texture = texture;
    }

    Ogre::TextureGpu *CWindowOgreNext::WindowTexture::getRenderTexture() const
    {
        return m_texture;
    }

    void CWindowOgreNext::WindowTexture::_getObject( void **ppObject ) const
    {
        *ppObject = m_texture;
    }

}  // namespace workphone::render
