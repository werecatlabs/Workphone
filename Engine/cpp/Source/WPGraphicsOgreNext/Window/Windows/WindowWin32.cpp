#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/Window/Windows/WindowWin32.hpp>
#include <Workphone/Workphone.hpp>
#include <Ogre.h>
#include <OgreWindow.h>

namespace workphone
{
    namespace render
    {

        LRESULT CALLBACK _WndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
        {
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

            auto renderWindow = win->getRenderWindow();
            if( renderWindow )
            {
                auto e = workphone::make_ptr<workphone::WindowMessageData>();
                e->setWindowHandle( hWnd );
                e->setMessage( uMsg );
                e->setWParam( wParam );
                e->setLParam( lParam );
                renderWindow->handleEvent( e );
            }

            return DefWindowProc( hWnd, uMsg, wParam, lParam );
        }

        WindowWin32::WindowWin32() : hwnd_( nullptr ), windowStyle_( WS_OVERLAPPEDWINDOW )
        {
        }

        WindowWin32::~WindowWin32()
        {
            if( hwnd_ )
            {
                DestroyWindow( hwnd_ );
            }
        }

        void WindowWin32::load( SmartPtr<ISharedObject> data )
        {
            WNDPROC windowProc = _WndProc;

            // Implement window creation based on provided data
            // Sample window creation code
            WNDCLASS wc = { 0 };
            wc.lpfnWndProc = DefWindowProc;
            wc.hInstance = GetModuleHandle( nullptr );
            wc.lpszClassName = "WindowClass";
            wc.lpfnWndProc = windowProc;

            RegisterClass( &wc );

            hwnd_ = CreateWindowEx( 0,                             // Optional window styles
                                    wc.lpszClassName,              // Window class
                                    name_.c_str(),                 // Window name
                                    windowStyle_,                  // Window style
                                    CW_USEDEFAULT, CW_USEDEFAULT,  // Size and position
                                    800, 600,                      // Width and height
                                    nullptr,                       // Parent window
                                    nullptr,                       // Menu
                                    wc.hInstance,                  // Instance handle
                                    this                           // Additional application data
            );

            if( hwnd_ == nullptr )
            {
                // Handle window creation failure
                return;
            }

            //ShowWindow( hwnd_, SW_SHOW );
        }

        void WindowWin32::unload( SmartPtr<ISharedObject> data )
        {
            // Cleanup resources here
            if( hwnd_ )
            {
                DestroyWindow( hwnd_ );
                hwnd_ = nullptr;
            }
        }

        void WindowWin32::getWindowHandle( void *pData )
        {
            // Assuming pData is expected to be HWND
            *static_cast<HWND *>( pData ) = hwnd_;
        }

        DWORD WindowWin32::getWindowStyle( bool fullScreen ) const
        {
            return fullScreen ? ( WS_POPUP ) : windowStyle_;
        }

        void WindowWin32::switchMode( u32 width, u32 height, u32 frequency )
        {
            // Adjust window or screen mode based on new width, height, and frequency
            SetWindowPos( hwnd_, nullptr, 0, 0, width, height, SWP_NOZORDER | SWP_NOMOVE );
        }

        SmartPtr<IGraphicsWindow> WindowWin32::getRenderWindow() const
        {
            auto p = renderWindow_.lock();
            return p;
        }

        void WindowWin32::setRenderWindow( SmartPtr<IGraphicsWindow> window )
        {
            renderWindow_ = window;
        }

        void WindowWin32::windowMovedOrResized()
        {
            // Handle logic when window moves or resizes
        }

        String WindowWin32::getName() const
        {
            return name_;
        }

        void WindowWin32::setName( const String &name )
        {
            name_ = name;
        }

        void WindowWin32::adjustWindow( s32 clientWidth, s32 clientHeight, s32 *outDrawableWidth,
                                        s32 *outDrawableHeight )
        {
            // Adjust window size calculations based on the client area
            RECT rect = { 0, 0, clientWidth, clientHeight };
            AdjustWindowRect( &rect, windowStyle_, false );
            *outDrawableWidth = rect.right - rect.left;
            *outDrawableHeight = rect.bottom - rect.top;
        }

    }  // namespace render
}  // namespace workphone
