#pragma once

#include <WPGraphicsOgre/WPGraphicsOgrePrerequisites.hpp>
#include <Workphone/Interface/Graphics/IInstanceManager.hpp>
#include <Workphone/Math/Vector2.hpp>
//#include <Cocoa/Cocoa.h>
//#include <Foundation/NSObject.h>

namespace workphone
{
    namespace render
    {

        class WindowMacOS
        {
        public:
            WindowMacOS();
            ~WindowMacOS();

            void load( SmartPtr<ISharedObject> data );
            void unload( SmartPtr<ISharedObject> data );

            void getWindowHandle( void *pData );
            
            s32 getWidth() const;
            s32 getHeight() const;

            IWindow* getWindow() const;
            void setWindow(IWindow* window);

            void setSize(s32 w, s32 h);

            Vector2I getFrameSize() const;
            void setFrameSize(s32 w, s32 h);

            void updateTrackingSize();

            void maximize();

        protected:
            //NSTrackingArea* trackingArea;

            void *externalWindowHandle = nullptr;

            String mTitle = "Window";

            /** 0/0 is legal and will be interpreted as 0/1.
                0/anything is interpreted as zero.
                If you are representing a whole number, the denominator should be 1.
            */
            u32 mFrequencyNumerator = 1;
            u32 mFrequencyDenominator = 1;

            u32 mRequestedWidth = 1280;   // in view points
            u32 mRequestedHeight = 720;  // in view points

            bool mFullscreenMode = false;
            bool mRequestedFullscreenMode = false;
            bool mBorderless = false;

            bool mFocused = true;
            bool mIsPrimary = true;

            bool mVSync = false;
            bool mHidden = false;

            u32 mVSyncInterval = 0;

            s32 mLeft = 0;  // in pixels
            s32 mTop = 0;   // in pixels
            
            IWindow* m_window = nullptr;
            void* m_windowView = nullptr;
        };

    }  // end namespace render
}  // end namespace fb
