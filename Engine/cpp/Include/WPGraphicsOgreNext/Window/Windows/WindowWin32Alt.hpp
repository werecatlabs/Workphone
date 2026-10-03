#ifndef __WindowWin32Alt_h__
#define __WindowWin32Alt_h__

#include <WPGraphicsOgreNext/WPGraphicsOgreNextPrerequisites.hpp>
#include <Workphone/Memory/SmartPtr.hpp>
#include <windef.h>

namespace workphone
{
    namespace render
    {

        class WindowWin32Alt
        {
        public:
            WindowWin32Alt();
            ~WindowWin32Alt();

            void load( SmartPtr<ISharedObject> data );
            void unload( SmartPtr<ISharedObject> data );

            void getWindowHandle( void *pData );

            DWORD getWindowStyle( bool fullScreen ) const;

            void switchMode( u32 width, u32 height, u32 frequency );

            SmartPtr<IGraphicsWindow> getRenderWindow() const;
            void setRenderWindow( SmartPtr<IGraphicsWindow> window );

            void windowMovedOrResized();

            String getName() const;

            void setName( const String &name );

        protected:
            void adjustWindow( s32 clientWidth, s32 clientHeight, s32 *outDrawableWidth,
                               s32 *outDrawableHeight );

            SmartPtr<render::IGraphicsWindow> m_renderWindow;

            // Win32 Window handle
            HWND mHWnd = nullptr;
            HDC mHDC = nullptr;
            HGLRC mGlrc = nullptr;
            char *mDeviceName = nullptr;

            s32 mTop = -1;
            s32 mLeft = -1;
            s32 mWidth = 1280;
            s32 mHeight = 720;

            s32 mColourDepth = 0;
            DWORD mWindowedWinStyle = 0;    // Windowed mode window style flags.
            DWORD mFullscreenWinStyle = 0;  // Fullscreen mode window style flags.

            /// The priority of the render target.
            u8 mPriority = 0;

            bool mOwnsGLContext = false;
            bool mSizing = false;
            bool mIsFullScreen = false;
            bool mRequestedFullscreenMode = false;
            int mDisplayFrequency = 0;  // fullscreen only, to restore display

            /// The name of this target.
            String mName;
        };

    }  // namespace render
}  // namespace workphone

#endif  // WindowWin32_h__
