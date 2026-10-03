// Dear ImGui: standalone example application for OSX + Metal.
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs
#include <EditorPrerequisites.hpp>
#include <EditorApplication.hpp>
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
//#    include "imgui_impl_osx.h"
@interface AppViewController : NSViewController
@end
#else
@interface AppViewController : UIViewController
@end
#endif

@interface AppViewController () <MTKViewDelegate>

@property( nonatomic, readonly ) workphone::editor::EditorApplication *application;
@property( nonatomic, readonly ) MTKView *mtkView;
@property( nonatomic, readonly ) NSView *view;
@property( nonatomic, strong ) id<MTLDevice> device;
@property( nonatomic, strong ) NSWindow *window;
@property( nonatomic, strong ) id<MTLCommandQueue> commandQueue;
@end

//-----------------------------------------------------------------------------------
// AppViewController
//-----------------------------------------------------------------------------------

@implementation AppViewController

- (instancetype)initWithNibName:(nullable NSString *)nibNameOrNil
                         bundle:(nullable NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];

    //_device = MTLCreateSystemDefaultDevice();
    //_commandQueue = [_device newCommandQueue];

    /*
    if( !self.device )
    {
        NSLog( @"Metal is not supported" );
        abort();
    }
     */

    return self;
}

- (MTKView *)mtkView
{
    return (MTKView *)self.view;
}

- (void)loadView
{
    //self.view = [[MTKView alloc] initWithFrame:CGRectMake(0, 0, 1200, 720)];
    self.view = [[NSView alloc] initWithFrame:CGRectMake( 0, 0, 1200, 720 )];
}

- (void)viewDidLoad
{
    [super viewDidLoad];

    // self.mtkView.device = self.device;
    // self.mtkView.delegate = self;

#if TARGET_OS_OSX
    // Add a tracking area in order to receive mouse events whenever the mouse is within the bounds of
    // our view
    NSTrackingArea *trackingArea = [[NSTrackingArea alloc]
        initWithRect:NSZeroRect
             options:NSTrackingMouseMoved | NSTrackingInVisibleRect | NSTrackingActiveAlways
               owner:self
            userInfo:nil];
    [self.view addTrackingArea:trackingArea];

#endif

    _application = new workphone::editor::EditorApplication;

    size_t windowHandle = 0;
    void *pData = (void *)&windowHandle;
    *static_cast<void **>( pData ) = (void *)CFBridgingRetain( self.window );

    //self.window.contentView = self.view;

    _application->setWindowHandle( windowHandle );
    _application->load( nullptr );
}

- (void)drawInMTKView:(MTKView *)view
{
    _application->iterate();
}

- (void)mtkView:(MTKView *)view drawableSizeWillChange:(CGSize)size
{
}

//-----------------------------------------------------------------------------------
// Input processing
//-----------------------------------------------------------------------------------

#if TARGET_OS_OSX

// Forward Mouse events to Dear ImGui OSX backend.
/*
-(void)mouseDown:(NSEvent *)event           { ImGui_ImplOSX_HandleEvent(event, self.view); }
-(void)rightMouseDown:(NSEvent *)event      { ImGui_ImplOSX_HandleEvent(event, self.view); }
-(void)otherMouseDown:(NSEvent *)event      { ImGui_ImplOSX_HandleEvent(event, self.view); }
-(void)mouseUp:(NSEvent *)event             { ImGui_ImplOSX_HandleEvent(event, self.view); }
-(void)rightMouseUp:(NSEvent *)event        { ImGui_ImplOSX_HandleEvent(event, self.view); }
-(void)otherMouseUp:(NSEvent *)event        { ImGui_ImplOSX_HandleEvent(event, self.view); }
-(void)mouseMoved:(NSEvent *)event          { ImGui_ImplOSX_HandleEvent(event, self.view); }
-(void)mouseDragged:(NSEvent *)event        { ImGui_ImplOSX_HandleEvent(event, self.view); }
-(void)rightMouseMoved:(NSEvent *)event     { ImGui_ImplOSX_HandleEvent(event, self.view); }
-(void)rightMouseDragged:(NSEvent *)event   { ImGui_ImplOSX_HandleEvent(event, self.view); }
-(void)otherMouseMoved:(NSEvent *)event     { ImGui_ImplOSX_HandleEvent(event, self.view); }
-(void)otherMouseDragged:(NSEvent *)event   { ImGui_ImplOSX_HandleEvent(event, self.view); }
-(void)scrollWheel:(NSEvent *)event         { ImGui_ImplOSX_HandleEvent(event, self.view); }
*/
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

@interface AppDelegate : NSObject <NSApplicationDelegate>
@property( nonatomic, strong ) NSWindow *window;
@property( nonatomic, strong ) AppViewController *viewController;
@end

@implementation AppDelegate

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender
{
    return YES;
}

- (instancetype)init
{
    if( self = [super init] )
    {
        /*
        auto rootViewController = [[AppViewController alloc] initWithNibName:nil bundle:nil];
        self.window = [[NSWindow alloc]
            initWithContentRect:NSZeroRect
                      styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
                                NSWindowStyleMaskResizable | NSWindowStyleMaskMiniaturizable
                        backing:NSBackingStoreBuffered
                          defer:NO];
        rootViewController.window = self.window;

        self.window.contentViewController = rootViewController;
        [self.window orderFront:self];
        [self.window center];
        [self.window becomeKeyWindow];
        */
    }

    return self;
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
    UIViewController *rootViewController = [[AppViewController alloc] init];
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

#if TARGET_OS_OSX

#if 0
int main( int argc, const char *argv[] )
{
    using namespace lioncat;
    // return NSApplicationMain(argc, argv);

    @autoreleasepool
    {
        AppDelegate *delegate = [[AppDelegate alloc] init];

        NSApplication *application = [NSApplication sharedApplication];
        [application setDelegate:delegate];
        //[application run];  // Note this never returns until the app finishes, so you need to decide where to
                      // hook your code into

        auto typeManager = TypeManager::instance();
        if( !typeManager )
        {
            typeManager = new TypeManager;
            typeManager->load();
            TypeManager::setInstance( typeManager );
        }

        lioncat::editor::EditorApplication app;
        app.load( nullptr );
        app.run();

        //auto viewController = (AppViewController*)delegate.window.contentViewController;
        //viewController.application->run();
    }
}
#else
#if !FB_EDITOR_TESTS
int main()
{
    using namespace workphone;

    auto typeManager = TypeManager::instance();
    if( !typeManager )
    {
        typeManager = new TypeManager;
        typeManager->load();
        TypeManager::setInstance( typeManager );
    }

    workphone::editor::EditorApplication app;
    app.setActiveThreads(4);
    app.load( nullptr );
    app.run();
    app.unload( nullptr );

    if( typeManager )
    {
        typeManager->unload();
        delete typeManager;
        TypeManager::setInstance( nullptr );
        typeManager = nullptr;
    }

    return 0;
}
#endif
#endif


#else

#if !FB_EDITOR_TESTS
int main( int argc, char *argv[] )
{
    @autoreleasepool
    {
        return UIApplicationMain( argc, argv, nil, NSStringFromClass( [AppDelegate class] ) );
    }
}
#endif

#endif
