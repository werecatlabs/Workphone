// Dear ImGui: standalone example application for OSX + Metal.
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs
#include <WPGraphicsOgreNext/Window/Apple/WindowMacOS.hpp>
#import <WPGraphicsOgreNext/Window/Apple/macOS/WindowView.hpp>
//#include "OgreOSXCocoaView.h"
#import <Foundation/Foundation.h>

#if TARGET_OS_OSX
#    import <Cocoa/Cocoa.h>
#else
#    import <UIKit/UIKit.h>
#endif

#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>

//#include "imgui.h"
//#include "imgui_impl_metal.h"
#if TARGET_OS_OSX
//#include "imgui_impl_osx.h"
@interface OgreViewController : NSViewController
@end
#else
@interface OgreViewController : UIViewController
@end
#endif

@interface OgreViewController ()
@property( nonatomic, readonly ) NSView *nsview;
@end

//-----------------------------------------------------------------------------------
// OgreViewController
//-----------------------------------------------------------------------------------

@implementation OgreViewController

- (void)viewDidLoad
{
}

- (void)loadView
{
    //self.view = [[WindowView alloc] initWithFrame:CGRectMake(0, 0, 1280*2, 720*2)];
}

//-----------------------------------------------------------------------------------
// Input processing
//-----------------------------------------------------------------------------------

#if TARGET_OS_OSX

// Forward Mouse events to Dear ImGui OSX backend.
- (void)mouseDown:(NSEvent *)event
{
    //ImGui_ImplOSX_HandleEvent(event, self.view);
}

- (void)rightMouseDown:(NSEvent *)event
{
    // ImGui_ImplOSX_HandleEvent(event, self.view);
}

- (void)otherMouseDown:(NSEvent *)event
{
    // ImGui_ImplOSX_HandleEvent(event, self.view);
}

- (void)mouseUp:(NSEvent *)event
{
    // ImGui_ImplOSX_HandleEvent(event, self.view);
}

- (void)rightMouseUp:(NSEvent *)event
{
    // ImGui_ImplOSX_HandleEvent(event, self.view);
}

- (void)otherMouseUp:(NSEvent *)event
{
    // ImGui_ImplOSX_HandleEvent(event, self.view);
}

- (void)mouseMoved:(NSEvent *)event
{
    int stop = 0;
    stop = 0;

    //ImGui_ImplOSX_HandleEvent(event, self.view);
}

- (void)mouseDragged:(NSEvent *)event
{
    // ImGui_ImplOSX_HandleEvent(event, self.view);
}

- (void)rightMouseMoved:(NSEvent *)event
{
    // ImGui_ImplOSX_HandleEvent(event, self.view);
}
- (void)rightMouseDragged:(NSEvent *)event
{
    // ImGui_ImplOSX_HandleEvent(event, self.view);
}

- (void)otherMouseMoved:(NSEvent *)event
{
    // ImGui_ImplOSX_HandleEvent(event, self.view);
}

- (void)otherMouseDragged:(NSEvent *)event
{
    // ImGui_ImplOSX_HandleEvent(event, self.view);
}

- (void)scrollWheel:(NSEvent *)event
{
    // ImGui_ImplOSX_HandleEvent(event, self.view);
}

#else

// This touch mapping is super cheesy/hacky. We treat any touch on the screen
// as if it were a depressed left mouse button, and we don't bother handling
// multitouch correctly at all. This causes the "cursor" to behave very erratically
// when there are multiple active touches. But for demo purposes, single-touch
// interaction actually works surprisingly well.
- (void)updateIOWithTouchEvent:(UIEvent *)event
{
    UITouch *anyTouch = event.allTouches.anyObject;
    CGPoint touchLocation = [anyTouch locationInView:self.view];
    ImGuiIO &io = ImGui::GetIO();
    io.AddMousePosEvent( touchLocation.x, touchLocation.y );

    BOOL hasActiveTouch = NO;
    for( UITouch *touch in event.allTouches )
    {
        if( touch.phase != UITouchPhaseEnded && touch.phase != UITouchPhaseCancelled )
        {
            hasActiveTouch = YES;
            break;
        }
    }
    io.AddMouseButtonEvent( 0, hasActiveTouch );
}

- (void)touchesBegan:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event
{
    [self updateIOWithTouchEvent:event];
}
- (void)touchesMoved:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event
{
    [self updateIOWithTouchEvent:event];
}
- (void)touchesCancelled:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event
{
    [self updateIOWithTouchEvent:event];
}
- (void)touchesEnded:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event
{
    [self updateIOWithTouchEvent:event];
}

#endif

@end

//-----------------------------------------------------------------------------------
// AppDelegate
//-----------------------------------------------------------------------------------

#if TARGET_OS_OSX

@interface OgreAppDelegate : NSObject <NSApplicationDelegate>
@property( nonatomic, strong ) NSWindow *window;
@end

@implementation OgreAppDelegate

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender
{
    return YES;
}

@end

#else

@interface AppDelegate : UIResponder <UIApplicationDelegate>
@property( strong, nonatomic ) UIWindow *window;
@end

@implementation AppDelegate

- (BOOL)application:(UIApplication *)application
    didFinishLaunchingWithOptions:(NSDictionary<UIApplicationLaunchOptionsKey, id> *)launchOptions
{
    UIViewController *rootViewController = [[OgreViewController alloc] init];
    self.window = [[UIWindow alloc] initWithFrame:UIScreen.mainScreen.bounds];
    self.window.rootViewController = rootViewController;
    [self.window makeKeyAndVisible];
    return YES;
}

@end

#endif

//-----------------------------------------------------------------------------------
// Application main() function
//-----------------------------------------------------------------------------------
namespace workphone
{
    namespace render
    {
        WindowMacOS::WindowMacOS()
        {
        }
        WindowMacOS::~WindowMacOS()
        {
        }

        void WindowMacOS::load( SmartPtr<ISharedObject> data )
        {
            NSRect frame = NSMakeRect( 0.0, 0.0, mRequestedWidth, mRequestedHeight );
            if( mRequestedWidth == 1 )
            {
                // just make it big enough to see
                frame.size.width = frame.size.height = 500.0;
            }
            NSWindowStyleMask style =
                NSWindowStyleMaskResizable | NSWindowStyleMaskTitled | NSWindowStyleMaskClosable;
            if( mRequestedFullscreenMode )
            {
                frame.size = NSScreen.mainScreen.visibleFrame.size;
                style = NSWindowStyleMaskBorderless;
            }

            //auto viewController = [[OgreViewController alloc] initWithNibName:nil bundle:nil];
            NSWindow *window = [[NSWindow alloc] initWithContentRect:frame
                                                           styleMask:style
                                                             backing:NSBackingStoreBuffered
                                                               defer:NO];
            //window.contentViewController = viewController;
            window.title = @( mTitle.c_str() );

            auto v = [[WindowView alloc] initWithFrame:frame];
            [v setOgreWindow:getWindow()];
            window.contentView = v;
            externalWindowHandle = (void *)CFBridgingRetain( window );

            m_windowView = (void *)CFBridgingRetain( v );

            // SetupMetalWindowListeners( this, window );
            if( !mHidden )
            {
                [window orderFront:nil];
            }

            [window makeFirstResponder:v];

            [window center];
            [window becomeKeyWindow];
            //m_nsWindow = (NSWindow *)CFBridgingRetain( window );
        }

        void WindowMacOS::unload( SmartPtr<ISharedObject> data )
        {
        }

        void WindowMacOS::getWindowHandle( void *pData )
        {
            *static_cast<void **>( pData ) = externalWindowHandle;
        }

        s32 WindowMacOS::getWidth() const
        {
            auto w = (NSWindow *)CFBridgingRelease(externalWindowHandle);
            auto frame = [w frame];
            return frame.size.width;
        }

        s32 WindowMacOS::getHeight() const
        {
            auto w = (NSWindow *)CFBridgingRelease(externalWindowHandle);
            auto frame = [w frame];
            return frame.size.height;
        }

        IWindow *WindowMacOS::getWindow() const
        {
            return m_window;
        }

        void WindowMacOS::setWindow( IWindow *window )
        {
            m_window = window;
        }

        void WindowMacOS::setSize( s32 w, s32 h )
        {
            if( externalWindowHandle )
            {
                auto window = (NSWindow *)CFBridgingRelease(externalWindowHandle);

                auto frame = [window frame];
                auto s = NSMakeRect( frame.origin.x, frame.origin.y, w, h );
                if( frame.size.width != w || frame.size.height != h )
                {
                    [window setFrame:s display:true animate:true];
                }
            }
        }

        Vector2I WindowMacOS::getFrameSize() const
        {
            auto w = (NSWindow *)CFBridgingRelease(externalWindowHandle);
            auto frame = [w frame];
            return Vector2I( frame.size.width, frame.size.height );
        }

        void WindowMacOS::setFrameSize( s32 w, s32 h )
        {
            if( externalWindowHandle )
            {
                auto v = (WindowView *)CFBridgingRelease(m_windowView);

                auto frame = [v frame];
                auto s = NSMakeSize( w, h );
                if( frame.size.width != w || frame.size.height != h )
                {
                    //window.contentLayoutRect = s;
                    //[window setFrame:s];
                    //[window setFrame:NSScreen.mainScreen.visibleFrame display: true animate: true];

                    [v setFrameSize:s];
                }
            }
        }

        void WindowMacOS::updateTrackingSize()
        {
            auto v = (WindowView *)CFBridgingRelease(m_windowView);
            [v updateTrackingAreas];
        }

        void WindowMacOS::maximize()
        {
            if( externalWindowHandle )
            {
                auto window = (NSWindow *)CFBridgingRelease(externalWindowHandle);

                [window setFrame:NSScreen.mainScreen.visibleFrame display:true animate:true];
            }
        }

    }  // end namespace render
}  // end namespace fb
