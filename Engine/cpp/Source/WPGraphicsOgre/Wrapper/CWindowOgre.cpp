#include <WPGraphicsOgre/WPGraphicsOgrePCH.hpp>
#include <WPGraphicsOgre/Wrapper/CWindowOgre.hpp>
#include <WPGraphicsOgre/Wrapper/CViewportOgre.hpp>
#include <WPGraphicsOgre/Addons/OgreUtil.hpp>
#include <Workphone/Workphone.hpp>
#include <Ogre.h>
#include <OgreOverlayManager.h>

#if defined WP_PLATFORM_WIN32
#    include <windows.h>
#    include <WPGraphicsOgre/Window/Windows/WindowWin32.hpp>
#elif defined WP_PLATFORM_APPLE
#    include <WPGraphicsOgre/Window/Apple/WindowMacOS.hpp>
#endif

namespace workphone
{
    namespace render
    {

        WP_CLASS_REGISTER_DERIVED( workphone, CWindowOgre, CRenderTargetOgre<GraphicsWindow> );

        CWindowOgre::CWindowOgre() : CRenderTargetOgre<GraphicsWindow>()
        {
            setupStateObject();
        }

        CWindowOgre::~CWindowOgre()
        {
            unload( nullptr );
        }

        void CWindowOgre::load( SmartPtr<ISharedObject> data )
        {
            try
            {
                WP_ASSERT( Thread::getTaskFlag( Thread::Render_Flag ) );
                setLoadingState( LoadingState::Loading );

                setSize( Vector2I( 1280, 720 ) );

                auto applicationManager = core::IApplicationManager::instance();
                auto graphicsSystem = applicationManager->getGraphicsSystem();

#if _DEBUG
                auto taskFlags = Thread::getTaskFlags();
                WP_ASSERT( ( taskFlags & Thread::Render_Flag ) != 0 );
#endif

                // Confirm Ogre::Root created
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
                    m_windowWin32 = new WindowWin32;
                    m_windowWin32->setRenderWindow( this );
                    m_windowWin32->load( nullptr );

                    size_t windowHandle = 0;
                    m_windowWin32->getWindowHandle( &windowHandle );

                    auto sWindowHandle = StringUtil::toString( windowHandle );
                    params["externalWindowHandle"] = StringUtil::str( sWindowHandle );
                }
#    else
                if( !StringUtil::isNullOrEmpty( handle ) )
                {
                    params["externalWindowHandle"] = OgreUtil::toString( handle );
                }
#    endif
#elif defined WP_PLATFORM_APPLE
#    if 1
                if( !StringUtil::isNullOrEmpty( m_windowHandle ) )
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

                //params["contentScalingFactor"] = StringUtil::toString( 1.0f );
#    else
                if( !StringUtil::isNullOrEmpty( m_windowHandle ) )
                {
                    params["externalWindowHandle"] = OgreUtil::toString( handle );
                }
#    endif
#elif defined WP_PLATFORM_LINUX
                if( !StringUtil::isNullOrEmpty( m_windowHandle ) )
                {
                    params["externalWindowHandle"] = OgreUtil::toString( handle );
                }
#endif

                auto name = getTitle();
                auto size = getSize();
                auto fullScreen = isFullScreen();

                auto w = static_cast<u32>( size.X() );
                auto h = static_cast<u32>( size.Y() );

                if( auto renderer = root->getRenderSystem() )
                {
                    auto desc = renderer->getRenderWindowDescription();
                    desc.name = Ogre::String( name.c_str(), name.length() );
                    desc.useFullScreen = fullScreen;
                    desc.width = w;
                    desc.height = h;

#if defined WP_PLATFORM_APPLE
                    desc.width /= 4;
                    desc.height /= 2;
#endif

                    desc.miscParams = params;
                    m_window = root->createRenderWindow( desc );
                }

                auto windowListener = new WindowListener;
                windowListener->setOwner( this );
                m_windowListener = windowListener;

                Ogre::WindowEventUtilities::addWindowEventListener( m_window, windowListener );

                setRenderTarget( m_window );

                //m_window->resize( w, h );
                //windowMovedOrResized();

#if defined WP_PLATFORM_APPLE
                if( m_osWindow )
                {
                    m_osWindow->updateTrackingSize();
                }
#endif

                setLoadingState( LoadingState::Loaded );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void CWindowOgre::unload( SmartPtr<ISharedObject> data )
        {
            try
            {
                if( const auto &loadingState = getLoadingState(); loadingState == LoadingState::Loaded )
                {
                    setLoadingState( LoadingState::Unloading );

                    auto root = Ogre::Root::getSingletonPtr();

                    if( m_windowListener )
                    {
                        Ogre::WindowEventUtilities::removeWindowEventListener( m_window,
                                                                               m_windowListener );
                        m_windowListener = nullptr;
                    }

                    if( m_window )
                    {
                        root->destroyRenderTarget( m_window );
                        m_window = nullptr;
                        m_renderTarget = nullptr;
                    }

#if defined WP_PLATFORM_WIN32
                    if( m_windowWin32 )
                    {
                        delete m_windowWin32;
                        m_windowWin32 = nullptr;
                    }
#elif defined WP_PLATFORM_APPLE
                    if( m_osWindow )
                    {
                        delete m_osWindow;
                        m_osWindow = nullptr;
                    }
#endif

                    CRenderTargetOgre<GraphicsWindow>::unload( nullptr );

                    setLoadingState( LoadingState::Unloaded );
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void CWindowOgre::handleEvent( SmartPtr<IGraphicsWindowEvent> event )
        {
        }

        void CWindowOgre::update()
        {
            if( m_window )
            {
                auto &oMgr = Ogre::OverlayManager::getSingleton();
                auto width = oMgr.getViewportWidth();
                auto height = oMgr.getViewportHeight();

#if defined WP_PLATFORM_APPLE
                if( m_osWindow && m_window )
                {
                    auto requestedSize = Vector2I( getSize().x, getSize().y );
                    auto osWindowSize = Vector2I( m_osWindow->getWidth(), m_osWindow->getHeight() );
                    auto windowSize = Vector2I( m_window->getWidth(), m_window->getHeight() );

                    //m_osWindow->setFrameSize(w, h);

                    if( requestedSize != osWindowSize )
                    {
                        //m_window->resize( width, height );
                        //windowMovedOrResized();

                        auto w = static_cast<u32>( requestedSize.X() );
                        auto h = static_cast<u32>( requestedSize.Y() );

                        //m_osWindow->setSize( w, h );
                        //m_osWindow->setFrameSize(w, h);
                        //m_window->windowMovedOrResized();
                    }

                    if( requestedSize != windowSize )
                    {
                        //m_window->resize( width, height );
                        //windowMovedOrResized();

                        auto w = static_cast<u32>( requestedSize.X() );
                        auto h = static_cast<u32>( requestedSize.Y() );

                        //m_osWindow->setSize( w, h );

                        //m_window->resize(w/2, h/2);
                        //m_window->resize(w, h);
                        //m_window->windowMovedOrResized();
                    }
                    //m_osWindow->setSize( requestedSize.X(), requestedSize.Y());
                    //m_osWindow->setFrameSize(windowSize.X(), windowSize.Y());
                }
#endif

                auto w = m_window->getWidth();
                auto h = m_window->getHeight();

                if( w != width || h != height )
                {
                    //m_window->resize( width, height );
                    //windowMovedOrResized();
                }

                if( width <= 0 && height <= 0 )
                {
                    auto size = m_window->getNumViewports();
                    for( size_t i = 0; i < size; ++i )
                    {
                        auto vp = m_window->getViewport( (u16)i );
                        vp->_updateDimensions();
                    }
                }

                CRenderTargetOgre<GraphicsWindow>::update();
            }
        }

        void CWindowOgre::initialise( Ogre::RenderWindow *window )
        {
            try
            {
                setLoadingState( LoadingState::Loading );

                m_window = window;
                setRenderTarget( m_window );

                setLoadingState( LoadingState::Loaded );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void CWindowOgre::setFullscreen( bool fullScreen, unsigned int width, unsigned int height )
        {
            ScopedLock lock( this );
            m_isFullscreen = fullScreen;
            m_window->setFullscreen( fullScreen, width, height );
        }

        void CWindowOgre::setFullscreen( bool fullscreen )
        {
            m_isFullscreen = fullscreen;
        }

        void CWindowOgre::destroy()
        {
            if( m_window )
            {
                m_window->destroy();
            }
        }

        void CWindowOgre::resize( u32 width, u32 height )
        {
            if( m_window )
            {
                m_window->resize( width, height );
            }
        }

        void CWindowOgre::windowMovedOrResized()
        {
            if( m_window )
            {
                m_window->windowMovedOrResized();
            }
        }

        void CWindowOgre::reposition( s32 left, s32 top )
        {
            ScopedLock lock( core::IApplicationManager::instance()->getGraphicsSystem() );
            m_window->reposition( left, top );
        }

        void CWindowOgre::maximize()
        {
            if( !m_window )
            {
                return;
            }

#ifdef WP_PLATFORM_WIN32
            HWND hwnd;

            m_window->getCustomAttribute( "WINDOW", &hwnd );
            ShowWindow( hwnd, SW_SHOWMAXIMIZED );
#elif defined WP_PLATFORM_APPLE
            if( m_osWindow && m_window )
            {
                m_osWindow->maximize();
            }
#endif

            m_window->windowMovedOrResized();

            auto numViewports = m_window->getNumViewports();
            for( u16 i = 0; i < numViewports; i++ )
            {
                auto vp = m_window->getViewport( i );
                vp->_updateDimensions();
            }
        }

        bool CWindowOgre::isVisible() const
        {
            ScopedLock lock( core::IApplicationManager::instance()->getGraphicsSystem() );
            return m_window->isVisible();
        }

        void CWindowOgre::setVisible( bool visible )
        {
            if( m_window )
            {
                m_window->setVisible( visible );
            }
        }

        bool CWindowOgre::isClosed() const
        {
            if( m_window )
            {
                return m_window->isClosed();
            }

            return false;
        }

        bool CWindowOgre::isPrimary() const
        {
            if( m_window )
            {
                return m_window->isPrimary();
            }

            return false;
        }

        bool CWindowOgre::isFullScreen() const
        {
            return m_isFullscreen;
        }

        void CWindowOgre::getMetrics( unsigned int &width, unsigned int &height,
                                      unsigned int &colourDepth, int &left, int &top )
        {
            ScopedLock lock( core::IApplicationManager::instance()->getGraphicsSystem() );
            return m_window->getMetrics( width, height, left, top );
        }

        u8 CWindowOgre::suggestPixelFormat() const
        {
            ScopedLock lock( core::IApplicationManager::instance()->getGraphicsSystem() );
            return m_window->suggestPixelFormat();
        }

        bool CWindowOgre::isDeactivatedOnFocusChange() const
        {
            ScopedLock lock( core::IApplicationManager::instance()->getGraphicsSystem() );
            return m_window->isDeactivatedOnFocusChange();
        }

        void CWindowOgre::setDeactivateOnFocusChange( bool deactivate )
        {
            ScopedLock lock( core::IApplicationManager::instance()->getGraphicsSystem() );
            m_window->setDeactivateOnFocusChange( deactivate );
        }

        void CWindowOgre::getCustomAttribute( const String &name, void *pData )
        {
            try
            {
                auto applicationManager = core::IApplicationManager::instance();
                auto graphicsSystem = applicationManager->getGraphicsSystem();

                ScopedLock lock( graphicsSystem );

                if( m_window )
                {
                    m_window->getCustomAttribute( name.c_str(), pData );
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void CWindowOgre::getWindowHandle( void *pData )
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto graphicsSystem = applicationManager->getGraphicsSystem();

            ScopedLock lock( graphicsSystem );

#if defined WP_PLATFORM_WIN32
            getCustomAttribute( "WINDOW", pData );
#elif defined WP_PLATFORM_APPLE
#    if defined WP_BUILD_RENDERER_METAL
            getCustomAttribute( "WINDOW", (void *)pData );
            if( !pData )
            {
                // getCustomAttribute("MetalDevice", &windowHnd);
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

        void CWindowOgre::getDeviceHandle( void *pData )
        {
        }

        void CWindowOgre::addListener( SmartPtr<IGraphicsWindowListener> listener )
        {
            m_listeners.push_back( listener );
        }

        void CWindowOgre::removeListener( SmartPtr<IGraphicsWindowListener> listener )
        {
            auto it = std::find( m_listeners.begin(), m_listeners.end(), listener );
            if( it != m_listeners.end() )
            {
                m_listeners.erase( it );
            }
        }

        Array<SmartPtr<IGraphicsWindowListener>> CWindowOgre::getListeners() const
        {
            return m_listeners.snapshot();
        }

        void CWindowOgre::_getObject( void **ppObject ) const
        {
            *ppObject = m_window;
        }

        Array<SmartPtr<IViewport>> CWindowOgre::getViewports() const
        {
            return m_viewports.snapshot();
        }

        Ogre::RenderWindow *CWindowOgre::getWindow() const
        {
            return m_window;
        }

        void CWindowOgre::setWindow( Ogre::RenderWindow *window )
        {
            m_window = window;
        }

        Vector2I CWindowOgre::getSize() const
        {
#if defined _DEBUG
#    if defined WP_PLATFORM_APPLE
            /*if( m_osWindow && m_window )
            {
                auto osWindowSize = Vector2I( m_osWindow->getWidth(), m_osWindow->getHeight() );
                auto windowSize = Vector2I( m_window->getWidth(), m_window->getHeight() );
                WP_ASSERT( osWindowSize == windowSize );
            }
             */
#    endif
#endif

#if defined WP_PLATFORM_APPLE
            /*if( m_osWindow )
            {
                auto w = m_osWindow->getWidth();
                auto h = m_osWindow->getHeight();
                return Vector2F( w, h );
            }
            */
#endif
            if( m_window )
            {
                auto w = m_window->getWidth();
                auto h = m_window->getHeight();
                return Vector2I( w, h );
            }

            return m_size;
        }

        void CWindowOgre::setSize( const Vector2I &size )
        {
            m_size = size;
        }

        void CWindowOgre::setColourDepth( u32 colourDepth )
        {
        }

        String CWindowOgre::getWindowHandleAsString() const
        {
            return m_windowHandle;
        }

        void CWindowOgre::setWindowHandleAsString( const String &handle )
        {
            m_windowHandle = handle;
        }

        CWindowOgre::WindowStateListener::WindowStateListener() = default;

        CWindowOgre::WindowStateListener::~WindowStateListener() = default;

        bool CWindowOgre::WindowStateListener::handleStateChanged( SmartPtr<IState> &state )
        {
            return false;
        }

        bool CWindowOgre::WindowStateListener::handleStateMessage(
            const SmartPtr<IStateMessage> &message )
        {
            if( auto owner = getOwner() )
            {
                if( message->isExactly<StateMessage>() )
                {
                    WP_ASSERT( workphone::dynamic_pointer_cast<StateMessage>( message ) );
                    auto stateMessage = workphone::static_pointer_cast<StateMessage>( message );
                    auto type = stateMessage->getType();

                    if( type == MOVED_OR_RESIZED_HASH )
                    {
                        owner->windowMovedOrResized();
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
                        owner->resize( static_cast<u32>( value.X() ), static_cast<u32>( value.Y() ) );
                    }
                }
            }

            return false;
        }

        SmartPtr<CWindowOgre> CWindowOgre::WindowStateListener::getOwner() const
        {
            auto p = m_owner.load();
            return p.lock();
        }

        void CWindowOgre::WindowStateListener::setOwner( SmartPtr<CWindowOgre> owner )
        {
            m_owner = owner;
        }

        CWindowOgre::WindowListener::WindowListener() = default;

        CWindowOgre::WindowListener::~WindowListener() = default;

        void CWindowOgre::WindowListener::windowMoved( Ogre::RenderWindow *rw )
        {
            (void)rw;
        }

        void CWindowOgre::WindowListener::windowResized( Ogre::RenderWindow *rw )
        {
            (void)rw;
        }

        bool CWindowOgre::WindowListener::windowClosing( Ogre::RenderWindow *rw )
        {
            (void)rw;

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            applicationManager->setQuit( true );

            return true;
        }

        void CWindowOgre::WindowListener::windowClosed( Ogre::RenderWindow *rw )
        {
            (void)rw;
        }

        void CWindowOgre::WindowListener::windowFocusChange( Ogre::RenderWindow *rw )
        {
            (void)rw;
        }

        CWindowOgre *CWindowOgre::WindowListener::getOwner() const
        {
            return m_owner;
        }

        void CWindowOgre::WindowListener::setOwner( CWindowOgre *owner )
        {
            m_owner = owner;
        }
    }  // namespace render
}  // namespace workphone
