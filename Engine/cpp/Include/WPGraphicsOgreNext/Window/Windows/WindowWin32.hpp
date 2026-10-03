#ifndef WindowWin32_h__
#define WindowWin32_h__

#include <WPGraphicsOgreNext/WPGraphicsOgreNextPrerequisites.hpp>
#include <Workphone/Memory/SmartPtr.hpp>
#include "Workphone/Memory/WeakPtr.hpp"
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

            String getName() const;

            void setName( const String &name );

        protected:
            void adjustWindow( s32 clientWidth, s32 clientHeight, s32 *outDrawableWidth,
                               s32 *outDrawableHeight );

            HWND hwnd_;  // Handle to the window
            WeakPtr<IGraphicsWindow> renderWindow_;
            String name_;
            DWORD windowStyle_;
        };

    }  // namespace render
}  // namespace workphone

#endif  // WindowWin32_h__
