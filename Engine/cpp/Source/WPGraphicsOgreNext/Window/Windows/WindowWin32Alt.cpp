#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/Window/Windows/WindowWin32Alt.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone
{
    namespace render
    {
        namespace
        {
            const char *WINDOW_CLASS_NAME = "OgreGLWindow";
        }

        LRESULT CALLBACK _WndProcAlt( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
        {
            if( uMsg == WM_CREATE )
            {
                if( lParam == 0 )
                {
                    WP_LOG_ERROR( "WindowWin32Alt::_WndProcAlt received WM_CREATE without create data." );
                    return -1;
                }

                const auto createParams = reinterpret_cast<LONG_PTR>(
                    reinterpret_cast<LPCREATESTRUCT>( lParam )->lpCreateParams );
                SetLastError( ERROR_SUCCESS );
                SetWindowLongPtr( hWnd, GWLP_USERDATA, createParams );
                if( GetLastError() != ERROR_SUCCESS )
                {
                    const auto error = GetLastError();
                    WP_LOG_ERROR( "WindowWin32Alt::_WndProcAlt failed to store window data. Win32 error " +
                                  StringUtil::toString( error ) );
                    return -1;
                }
                return 0;
            }

            auto win = reinterpret_cast<WindowWin32Alt *>( GetWindowLongPtr( hWnd, GWLP_USERDATA ) );
            if( !win )
            {
                return DefWindowProc( hWnd, uMsg, wParam, lParam );
            }

            if( auto renderWindow = win->getRenderWindow() )
            {
                auto event = workphone::make_ptr<workphone::WindowMessageData>();
                event->setWindowHandle( hWnd );
                event->setMessage( uMsg );
                event->setWParam( wParam );
                event->setLParam( lParam );
                renderWindow->handleEvent( event );
            }

            return DefWindowProc( hWnd, uMsg, wParam, lParam );
        }

        WindowWin32Alt::WindowWin32Alt() = default;

        WindowWin32Alt::~WindowWin32Alt()
        {
            unload( nullptr );
        }

        void WindowWin32Alt::adjustWindow( s32 clientWidth, s32 clientHeight, s32 *outDrawableWidth,
                                           s32 *outDrawableHeight )
        {
            if( !outDrawableWidth || !outDrawableHeight )
            {
                WP_LOG_ERROR( "WindowWin32Alt::adjustWindow requires valid output pointers." );
                return;
            }

            RECT rect;
            SetRect( &rect, 0, 0, clientWidth, clientHeight );
            if( !AdjustWindowRect( &rect, getWindowStyle( mRequestedFullscreenMode ), FALSE ) )
            {
                const auto error = GetLastError();
                WP_LOG_ERROR( "WindowWin32Alt::adjustWindow failed. Win32 error " +
                              StringUtil::toString( error ) );
                *outDrawableWidth = clientWidth;
                *outDrawableHeight = clientHeight;
                return;
            }

            *outDrawableWidth = rect.right - rect.left;
            *outDrawableHeight = rect.bottom - rect.top;
        }

        void WindowWin32Alt::load( SmartPtr<ISharedObject> data )
        {
            (void)data;
            WP_ASSERT( Thread::getCurrentTask() == TaskId::Primary );

            if( mHWnd )
            {
                WP_LOG_WARNING( "WindowWin32Alt::load called while a window already exists." );
                return;
            }

            constexpr int defaultWidth = 1280;
            constexpr int defaultHeight = 720;
            int left = -1;
            int top = -1;
            HMONITOR monitor = MonitorFromPoint( POINT{ left, top }, MONITOR_DEFAULTTOPRIMARY );
            if( !monitor )
            {
                const auto error = GetLastError();
                WP_LOG_ERROR( "WindowWin32Alt::load could not locate a display monitor. Win32 error " +
                              StringUtil::toString( error ) );
                return;
            }

            MONITORINFOEX monitorInfo = {};
            monitorInfo.cbSize = sizeof( monitorInfo );
            if( !GetMonitorInfo( monitor, &monitorInfo ) )
            {
                const auto error = GetLastError();
                WP_LOG_ERROR( "WindowWin32Alt::load could not read display monitor information. Win32 error " +
                              StringUtil::toString( error ) );
                return;
            }

            delete[] mDeviceName;
            const auto deviceNameLength = strlen( monitorInfo.szDevice );
            mDeviceName = new char[deviceNameLength + 1];
            memcpy( mDeviceName, monitorInfo.szDevice, deviceNameLength + 1 );

            mFullscreenWinStyle = WS_VISIBLE | WS_CLIPCHILDREN | WS_POPUP;
            mWindowedWinStyle = WS_VISIBLE | WS_CLIPCHILDREN | WS_OVERLAPPEDWINDOW;

            const int workWidth = monitorInfo.rcWork.right - monitorInfo.rcWork.left;
            const int workHeight = monitorInfo.rcWork.bottom - monitorInfo.rcWork.top;
            if( left == -1 || top == -1 )
            {
                int windowWidth = 0;
                int windowHeight = 0;
                adjustWindow( defaultWidth, defaultHeight, &windowWidth, &windowHeight );
                const int clampedWidth = ( windowWidth < workWidth ) ? windowWidth : workWidth;
                const int clampedHeight = ( windowHeight < workHeight ) ? windowHeight : workHeight;

                if( left == -1 )
                    left = monitorInfo.rcWork.left + ( workWidth - clampedWidth ) / 2;

                if( top == -1 )
                    top = monitorInfo.rcWork.top + ( workHeight - clampedHeight ) / 2;
            }

            mWidth = defaultWidth;
            mHeight = defaultHeight;
            mTop = top;
            mLeft = left;

            if( mIsFullScreen )
            {
                mTop = monitorInfo.rcMonitor.top;
                mLeft = monitorInfo.rcMonitor.left;
            }
            else
            {
                RECT rect;
                SetRect( &rect, 0, 0, mWidth, mHeight );
                if( !AdjustWindowRect( &rect, getWindowStyle( mRequestedFullscreenMode ), FALSE ) )
                {
                    const auto error = GetLastError();
                    WP_LOG_ERROR( "WindowWin32Alt::load could not calculate outer window dimensions. Win32 error " +
                                  StringUtil::toString( error ) );
                    unload( nullptr );
                    return;
                }

                mWidth = rect.right - rect.left;
                mHeight = rect.bottom - rect.top;

                if( mLeft < monitorInfo.rcWork.left )
                    mLeft = monitorInfo.rcWork.left;
                if( mTop < monitorInfo.rcWork.top )
                    mTop = monitorInfo.rcWork.top;
                if( mWidth > monitorInfo.rcWork.right - mLeft )
                    mWidth = monitorInfo.rcWork.right - mLeft;
                if( mHeight > monitorInfo.rcWork.bottom - mTop )
                    mHeight = monitorInfo.rcWork.bottom - mTop;
            }

            HINSTANCE instance = GetModuleHandle( nullptr );
            if( !instance )
            {
                const auto error = GetLastError();
                WP_LOG_ERROR( "WindowWin32Alt::load could not get the process module handle. Win32 error " +
                              StringUtil::toString( error ) );
                unload( nullptr );
                return;
            }

            WNDCLASS windowClass = {};
            windowClass.style = CS_OWNDC;
            windowClass.lpfnWndProc = _WndProcAlt;
            windowClass.hInstance = instance;
            windowClass.hIcon = LoadIcon( nullptr, IDI_APPLICATION );
            windowClass.hCursor = LoadCursor( nullptr, IDC_ARROW );
            windowClass.hbrBackground = static_cast<HBRUSH>( GetStockObject( BLACK_BRUSH ) );
            windowClass.lpszClassName = WINDOW_CLASS_NAME;

            if( !RegisterClass( &windowClass ) )
            {
                const auto error = GetLastError();
                WNDCLASS existingClass = {};
                if( error != ERROR_CLASS_ALREADY_EXISTS ||
                    !GetClassInfo( instance, WINDOW_CLASS_NAME, &existingClass ) ||
                    existingClass.lpfnWndProc != _WndProcAlt )
                {
                    WP_LOG_ERROR( "WindowWin32Alt::load could not register its window class. Win32 error " +
                                  StringUtil::toString( error ) );
                    unload( nullptr );
                    return;
                }
                WP_LOG_INFO( "WindowWin32Alt::load reusing the existing compatible window class." );
            }

            if( mIsFullScreen )
            {
                switchMode( mWidth, mHeight, mDisplayFrequency );
            }

            const auto title = getName();
            mHWnd = CreateWindowEx( 0, WINDOW_CLASS_NAME, title.c_str(), getWindowStyle( mIsFullScreen ),
                                    mLeft, mTop, mWidth, mHeight, nullptr, nullptr, instance, this );
            if( !mHWnd )
            {
                const auto error = GetLastError();
                WP_LOG_ERROR( "WindowWin32Alt::load failed to create window '" + title +
                              "'. Win32 error " + StringUtil::toString( error ) );
                unload( nullptr );
                return;
            }

            WP_LOG_INFO( "WindowWin32Alt::load created window '" + title + "' at " +
                         StringUtil::toString( mLeft ) + "," + StringUtil::toString( mTop ) + " size " +
                         StringUtil::toString( mWidth ) + "x" + StringUtil::toString( mHeight ) + "." );
        }

        void WindowWin32Alt::unload( SmartPtr<ISharedObject> data )
        {
            (void)data;
            WP_ASSERT( Thread::getCurrentTask() == TaskId::Primary );

            setRenderWindow( nullptr );
            if( mHWnd )
            {
                const auto window = mHWnd;
                if( !DestroyWindow( window ) )
                {
                    const auto error = GetLastError();
                    WP_LOG_ERROR( "WindowWin32Alt::unload failed to destroy the window. Win32 error " +
                                  StringUtil::toString( error ) );
                    SetWindowLongPtr( window, GWLP_USERDATA, 0 );
                }
                else
                {
                    WP_LOG_INFO( "WindowWin32Alt::unload destroyed the window." );
                }
                mHWnd = nullptr;
            }

            delete[] mDeviceName;
            mDeviceName = nullptr;
        }

        void WindowWin32Alt::getWindowHandle( void *pData )
        {
            if( !pData )
            {
                WP_LOG_ERROR( "WindowWin32Alt::getWindowHandle received a null output pointer." );
                return;
            }
            *static_cast<void **>( pData ) = mHWnd;
        }

        DWORD WindowWin32Alt::getWindowStyle( bool fullScreen ) const
        {
            return fullScreen ? mFullscreenWinStyle : mWindowedWinStyle;
        }

        void WindowWin32Alt::switchMode( u32 width, u32 height, u32 frequency )
        {
            if( !mDeviceName )
            {
                WP_LOG_ERROR( "WindowWin32Alt::switchMode called without a display device name." );
                return;
            }

            DEVMODE displayDeviceMode = {};
            displayDeviceMode.dmSize = sizeof( displayDeviceMode );
            displayDeviceMode.dmBitsPerPel = mColourDepth;
            displayDeviceMode.dmPelsWidth = width;
            displayDeviceMode.dmPelsHeight = height;
            displayDeviceMode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

            if( frequency )
            {
                displayDeviceMode.dmDisplayFrequency = frequency;
                displayDeviceMode.dmFields |= DM_DISPLAYFREQUENCY;
                const auto testResult = ChangeDisplaySettingsEx( mDeviceName, &displayDeviceMode, nullptr,
                                                                  CDS_FULLSCREEN | CDS_TEST, nullptr );
                if( testResult != DISP_CHANGE_SUCCESSFUL )
                {
                    WP_LOG_WARNING( "WindowWin32Alt::switchMode rejected the requested display frequency. Result " +
                                    StringUtil::toString( testResult ) + "; retrying without it." );
                    displayDeviceMode.dmFields &= ~DM_DISPLAYFREQUENCY;
                }
            }

            const auto result = ChangeDisplaySettingsEx( mDeviceName, &displayDeviceMode, nullptr,
                                                          CDS_FULLSCREEN, nullptr );
            if( result != DISP_CHANGE_SUCCESSFUL )
            {
                WP_LOG_ERROR( "WindowWin32Alt::switchMode failed to change display mode. Result " +
                              StringUtil::toString( result ) );
            }
        }

        SmartPtr<IGraphicsWindow> WindowWin32Alt::getRenderWindow() const
        {
            return m_renderWindow;
        }

        void WindowWin32Alt::setRenderWindow( SmartPtr<IGraphicsWindow> window )
        {
            m_renderWindow = window;
        }

        void WindowWin32Alt::windowMovedOrResized()
        {
            if( auto renderWindow = getRenderWindow() )
            {
                renderWindow->windowMovedOrResized();
            }
        }

        String WindowWin32Alt::getName() const
        {
            return mName;
        }

        void WindowWin32Alt::setName( const String &name )
        {
            mName = name;
        }

    }  // namespace render
}  // namespace workphone
