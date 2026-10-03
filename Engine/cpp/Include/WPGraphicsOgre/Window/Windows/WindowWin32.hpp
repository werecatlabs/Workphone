#ifndef WindowWin32_h__
#define WindowWin32_h__

#include <WPGraphicsOgre/WPGraphicsOgrePrerequisites.hpp>
#include <windef.h>

namespace workphone
{
    namespace render
    {

        class WindowWin32
        {
        public:
            WindowWin32();
            ~WindowWin32();

            void load( SmartPtr<ISharedObject> data );
            void unload( SmartPtr<ISharedObject> data );

            void getWindowHandle( void *pData );

            DWORD getWindowStyle( bool fullScreen ) const;

            void switchMode( u32 width, u32 height, u32 frequency );

            SmartPtr<IGraphicsWindow> getRenderWindow() const;
            void setRenderWindow( SmartPtr<IGraphicsWindow> window );

            void windowMovedOrResized();

            u32 getWidth() const;

            void setWidth( u32 width );

            u32 getHeight() const;

            void setHeight( u32 height );

        protected:
            void adjustWindow( u32 clientWidth, u32 clientHeight, u32 *outDrawableWidth,
                               u32 *outDrawableHeight );

            SmartPtr<render::IGraphicsWindow> m_renderWindow;

            c8 *m_deviceName = nullptr;

            s32 m_colourDepth = 0;

            s32 m_top = -1;
            s32 m_left = -1;
            s32 m_width = 1280;
            s32 m_height = 720;

            s32 m_displayFrequency = 0;  // fullscreen only, to restore display

            HWND m_hwnd;  // Win32 Window handle
            HDC m_hdc;
            HGLRC m_glrc;

            /// The priority of the render target.
            u8 m_priority = 0;

            bool m_ownsGLContext = false;
            bool m_sizing = false;

            bool m_isFullScreen = false;
            bool m_requestedFullscreenMode = false;

            DWORD m_windowedWinStyle = 0;    // Windowed mode window style flags.
            DWORD m_fullscreenWinStyle = 0;  // Fullscreen mode window style flags.

            /// The name of this target.
            String m_name;
        };

    }  // namespace render
}  // namespace workphone

#endif  // WindowWin32_h__
