#include <WPGraphicsOgre/WPGraphicsOgrePCH.hpp>
#include <WPGraphicsOgre/Window/Windows/WindowWin32.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone
{
    namespace render
    {
        LRESULT CALLBACK _WndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto factoryManager = applicationManager->getFactoryManagerPtr();

            if( uMsg == WM_CREATE )
            {  // Store pointer to Win32Window in user data area
                auto pLParam = (LPCREATESTRUCT)lParam;
                auto pCreateParams = (LONG_PTR)( pLParam->lpCreateParams );
                SetWindowLongPtr( hWnd, GWLP_USERDATA, pCreateParams );
                return 0;
            }

            auto win = (WindowWin32 *)GetWindowLongPtr( hWnd, GWLP_USERDATA );
            if( !win )
            {
                return DefWindowProc( hWnd, uMsg, wParam, lParam );
            }

            if( applicationManager->isRunning() )
            {
                auto e = factoryManager->make_ptr<WindowMessageData>();
                e->setMessage( uMsg );
                e->setWParam( wParam );
                e->setLParam( lParam );

                auto renderWindow = win->getRenderWindow();
                if( renderWindow )
                {
                    auto listeners = renderWindow->getListeners();
                    for( auto listener : listeners )
                    {
                        listener->handleEvent( e );
                    }
                }
            }

            switch( uMsg )
            {
            case WM_CLOSE:
            {
                bool close = true;

                if( auto renderWindow = win->getRenderWindow() )
                {
                    auto listeners = renderWindow->getListeners();
                    for( auto listener : listeners )
                    {
                        auto retValue = listener->handleEvent(
                            EventType::Window, IGraphicsWindowListener::windowClosingHash, Array<Parameter>(),
                            nullptr, nullptr, nullptr );

                        if( !retValue.getBool() )
                        {
                            close = false;
                        }
                    }
                }

                if( !close )
                {
                    return 0;
                }

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
                win->windowMovedOrResized();

                auto renderWindow = win->getRenderWindow();
                auto listeners = renderWindow->getListeners();
                for( auto listener : listeners )
                {
                    //listener->windowMoved( win );

                    auto retValue =
                        listener->handleEvent( EventType::Window, IGraphicsWindowListener::windowMovedHash,
                                               Array<Parameter>(), nullptr, nullptr, nullptr );
                }
            }
            break;
            case WM_DISPLAYCHANGE:
            {
                win->windowMovedOrResized();

                auto renderWindow = win->getRenderWindow();
                auto listeners = renderWindow->getListeners();
                for( auto listener : listeners )
                {
                    //listener->windowResized( win );

                    auto retValue =
                        listener->handleEvent( EventType::Window, IGraphicsWindowListener::windowResizedHash,
                                               Array<Parameter>(), nullptr, nullptr, nullptr );
                }
            }
            break;
            case WM_SIZE:
            {
                // log->logMessage("WM_SIZE");
                win->windowMovedOrResized();

                auto renderWindow = win->getRenderWindow();
                auto listeners = renderWindow->getListeners();
                for( auto listener : listeners )
                {
                    //listener->windowResized( win );

                    auto retValue =
                        listener->handleEvent( EventType::Window, IGraphicsWindowListener::windowResizedHash,
                                               Array<Parameter>(), nullptr, nullptr, nullptr );
                }
            }
            break;

            default:
            {
            }
            };

            return DefWindowProc( hWnd, uMsg, wParam, lParam );
        }

        WindowWin32::WindowWin32()
        {
        }

        WindowWin32::~WindowWin32()
        {
            unload( nullptr );
        }

        void WindowWin32::adjustWindow( u32 clientWidth, u32 clientHeight, u32 *outDrawableWidth,
                                        u32 *outDrawableHeight )
        {
            RECT rc;
            SetRect( &rc, 0, 0, clientWidth, clientHeight );
            AdjustWindowRect( &rc, getWindowStyle( m_requestedFullscreenMode ), false );
            *outDrawableWidth = rc.right - rc.left;
            *outDrawableHeight = rc.bottom - rc.top;
        }

        void WindowWin32::load( SmartPtr<ISharedObject> data )
        {
            unsigned int width = 1280;
            unsigned int height = 720;
            int left = -1;  // Defaults to screen center
            int top = -1;   // Defaults to screen center
            WNDPROC windowProc = _WndProc;
            String title = "Test";
            bool hidden = false;
            String border;
            bool outerSize = false;
            bool hwGamma = false;
            bool enableDoubleClick = false;
            int monitorIndex = -1;
            HMONITOR hMonitor = nullptr;
            HINSTANCE hInst = nullptr;
            bool fullScreen = false;

            // WNDCLASS dummyClass;
            // memset( &dummyClass, 0, sizeof( WNDCLASS ) );
            // dummyClass.style = CS_OWNDC;
            // dummyClass.hInstance = hinst;
            // dummyClass.lpfnWndProc = dummyWndProc;
            // dummyClass.lpszClassName = dummyText;
            // RegisterClass( &dummyClass );

            // HWND hwnd = CreateWindow( dummyText, dummyText, WS_POPUP | WS_CLIPCHILDREN, 0, 0, 32, 32,
            // 0,
            //                           0, hinst, 0 );

            auto mIsExternal = false;
            if( !mIsExternal )
            {
                DWORD dwStyleEx = 0;
                MONITORINFOEX monitorInfoEx;
                RECT rc;

                // If we didn't specified the adapter index, or if it didn't find it
                if( hMonitor == nullptr )
                {
                    POINT windowAnchorPoint;

                    // Fill in anchor point.
                    windowAnchorPoint.x = left;
                    windowAnchorPoint.y = top;

                    // Get the nearest monitor to this window.
                    hMonitor = MonitorFromPoint( windowAnchorPoint, MONITOR_DEFAULTTOPRIMARY );
                }

                // Get the target monitor info
                memset( &monitorInfoEx, 0, sizeof( MONITORINFOEX ) );
                monitorInfoEx.cbSize = sizeof( MONITORINFOEX );
                GetMonitorInfo( hMonitor, &monitorInfoEx );

                size_t devNameLen = strlen( monitorInfoEx.szDevice );
                m_deviceName = new char[devNameLen + 1];

                strcpy( m_deviceName, monitorInfoEx.szDevice );

                // Update window style flags.
                m_fullscreenWinStyle = ( hidden ? 0 : WS_VISIBLE ) | WS_CLIPCHILDREN | WS_POPUP;
                m_windowedWinStyle = ( hidden ? 0 : WS_VISIBLE ) | WS_CLIPCHILDREN;

                if( border == "none" )
                    m_windowedWinStyle |= WS_POPUP;
                else if( border == "fixed" )
                    m_windowedWinStyle |=
                        WS_OVERLAPPED | WS_BORDER | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
                else
                    m_windowedWinStyle |= WS_OVERLAPPEDWINDOW;

                // No specified top left -> Center the window in the middle of the monitor
                if( left == -1 || top == -1 )
                {
                    auto screenw = monitorInfoEx.rcWork.right - monitorInfoEx.rcWork.left;
                    auto screenh = monitorInfoEx.rcWork.bottom - monitorInfoEx.rcWork.top;

                    long winWidth = 0;
                    long winHeight = 0;
                    //adjustWindow( width, height, &winWidth, &winHeight );

                    // clamp window dimensions to screen size
                    auto outerw = ( winWidth < screenw ) ? winWidth : screenw;
                    auto outerh = ( winHeight < screenh ) ? winHeight : screenh;

                    if( left == -1 )
                        left = monitorInfoEx.rcWork.left + ( screenw - outerw ) / 2;
                    else if( monitorIndex != -1 )
                        left += monitorInfoEx.rcWork.left;

                    if( top == -1 )
                        top = monitorInfoEx.rcWork.top + ( screenh - outerh ) / 2;
                    else if( monitorIndex != -1 )
                        top += monitorInfoEx.rcWork.top;
                }
                else if( monitorIndex != -1 )
                {
                    left += monitorInfoEx.rcWork.left;
                    top += monitorInfoEx.rcWork.top;
                }

                m_width = width;
                m_height = height;
                m_top = top;
                m_left = left;

                if( m_isFullScreen )
                {
                    dwStyleEx |= WS_EX_TOPMOST;
                    m_top = monitorInfoEx.rcMonitor.top;
                    m_left = monitorInfoEx.rcMonitor.left;
                }
                else
                {
                    int screenw = GetSystemMetrics( SM_CXSCREEN );
                    int screenh = GetSystemMetrics( SM_CYSCREEN );

                    if( !outerSize )
                    {
                        // Calculate window dimensions required
                        // to get the requested client area
                        SetRect( &rc, 0, 0, m_width, m_height );
                        AdjustWindowRect( &rc, getWindowStyle( fullScreen ), false );
                        m_width = rc.right - rc.left;
                        m_height = rc.bottom - rc.top;

                        // Clamp window rect to the nearest display monitor.
                        if( m_left < (u32)monitorInfoEx.rcWork.left )
                            m_left = (u32)monitorInfoEx.rcWork.left;

                        if( m_top < (u32)monitorInfoEx.rcWork.top )
                            m_top = (u32)monitorInfoEx.rcWork.top;

                        if( static_cast<int>( m_width ) > monitorInfoEx.rcWork.right - m_left )
                            m_width = monitorInfoEx.rcWork.right - m_left;

                        if( static_cast<int>( m_height ) > monitorInfoEx.rcWork.bottom - m_top )
                            m_height = monitorInfoEx.rcWork.bottom - m_top;
                    }
                }

                UINT classStyle = CS_OWNDC;
                if( enableDoubleClick )
                    classStyle |= CS_DBLCLKS;

                // register class and create window
                WNDCLASS wc = { classStyle,
                                windowProc,
                                0,
                                0,
                                hInst,
                                LoadIcon( nullptr, IDI_APPLICATION ),
                                LoadCursor( nullptr, IDC_ARROW ),
                                static_cast<HBRUSH>( GetStockObject( BLACK_BRUSH ) ),
                                nullptr,
                                "OgreGLWindow" };
                RegisterClass( &wc );

                if( m_isFullScreen )
                {
                    switchMode( m_width, m_height, m_displayFrequency );
                }

                // Pass pointer to self as WM_CREATE parameter
                auto windowStyle = getWindowStyle( fullScreen );
                auto name = String( "OgreGLWindow" );
                m_hwnd = CreateWindowEx( dwStyleEx, name.c_str(), title.c_str(), windowStyle, m_left,
                                         m_top, m_width, m_height, nullptr, nullptr, hInst, this );

                // LogManager::getSingleton().stream()
                //     << "Created Win32Window '" << mName << "' : " << mWidth << "x" << mHeight << ", "
                //     << mColourDepth << "bpp";
            }
        }

        void WindowWin32::unload( SmartPtr<ISharedObject> data )
        {
            if( m_hwnd )
            {
                DestroyWindow( m_hwnd );
                m_hwnd = nullptr;
            }
        }

        void WindowWin32::getWindowHandle( void *pData )
        {
            *static_cast<void **>( pData ) = m_hwnd;
        }

        DWORD WindowWin32::getWindowStyle( bool fullScreen ) const
        {
            if( fullScreen )
            {
                return m_fullscreenWinStyle;
            }

            return m_windowedWinStyle;
        }

        void WindowWin32::switchMode( u32 width, u32 height, u32 frequency )
        {
            DEVMODE displayDeviceMode = {};

            displayDeviceMode.dmSize = sizeof( DEVMODE );
            displayDeviceMode.dmBitsPerPel = m_colourDepth;
            displayDeviceMode.dmPelsWidth = width;
            displayDeviceMode.dmPelsHeight = height;
            displayDeviceMode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

            if( frequency )
            {
                displayDeviceMode.dmDisplayFrequency = frequency;
                displayDeviceMode.dmFields |= DM_DISPLAYFREQUENCY;
                if( ChangeDisplaySettingsEx( m_deviceName, &displayDeviceMode, nullptr,
                                             CDS_FULLSCREEN | CDS_TEST,
                                             nullptr ) != DISP_CHANGE_SUCCESSFUL )
                {
                    WP_LOG( "ChangeDisplaySettings with user display frequency failed" );
                    displayDeviceMode.dmFields ^= DM_DISPLAYFREQUENCY;
                }
            }

            if( ChangeDisplaySettingsEx( m_deviceName, &displayDeviceMode, nullptr, CDS_FULLSCREEN,
                                         nullptr ) != DISP_CHANGE_SUCCESSFUL )
            {
                WP_LOG_ERROR( "ChangeDisplaySettings failed" );
            }
        }

        SmartPtr<IGraphicsWindow> WindowWin32::getRenderWindow() const
        {
            return m_renderWindow;
        }

        void WindowWin32::setRenderWindow( SmartPtr<IGraphicsWindow> window )
        {
            m_renderWindow = window;
        }

        void WindowWin32::windowMovedOrResized()
        {
            if( auto renderWindow = getRenderWindow() )
            {
                renderWindow->windowMovedOrResized();
            }
        }

        u32 WindowWin32::getWidth() const
        {
            return m_width;
        }

        void WindowWin32::setWidth( u32 width )
        {
            m_width = width;
        }

        u32 WindowWin32::getHeight() const
        {
            return m_height;
        }

        void WindowWin32::setHeight( u32 height )
        {
            m_height = height;
        }

    }  // namespace render
}  // namespace workphone
