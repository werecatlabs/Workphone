/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2016 Torus Knot Software Ltd

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
                                                              copies of the Software, and to permit
persons to whom the Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
        THE SOFTWARE.
        -----------------------------------------------------------------------------
        */

#import <WPGraphicsOgreNext/Window/Apple/macOS/WindowView.hpp>
#include <Workphone/WPCore.hpp>
//#import "OgreMetalView.h"
#import <QuartzCore/QuartzCore.h>
#import <SpriteKit/SpriteKit.h>

@implementation WindowView

- (void)initCommon
{
    self.layer = [CAMetalLayer layer];
    self.wantsLayer = YES;

    _layerSizeDidUpdate = YES;
}

- (void)viewDidMoveToWindow
{
    // if(self.scaleToNative)
    //    [setContentScaleFactor:self.window.screen.backingScaleFactor * self.nativeScaleFactor];
    _layerSizeDidUpdate = YES;
}

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
- (id)initWithFrame:(CGRect)frame
#else
- (id)initWithFrame:(NSRect)frame
#endif
{
    self = [super initWithFrame:frame];

    if( self )
    {
        [self initCommon];
    }

    return self;
}

- (instancetype)initWithCoder:(NSCoder *)coder
{
    self = [super initWithCoder:coder];

    if( self )
    {
        [self initCommon];
    }
    return self;
}

- (BOOL)acceptsFirstMouse:(NSEvent *)theEvent
{
    if( self.superview == nil )
    {
        return NO;
    }
    else
    {
        return [self.superview acceptsFirstMouse:theEvent];
    }
}

- (void)setContentScaleFactor:(CGFloat)contentScaleFactor
{
    self.scaleToNative = false;
    //[super setContentScaleFactor:contentScaleFactor];
    _layerSizeDidUpdate = YES;
}

- (void)setOgreWindow:(workphone::render::IWindow*)w
{
    renderWindow = w;
}

- (workphone::render::IWindow*)ogreWindow
{
    return renderWindow;
}

- (void)setMousePosition:(workphone::Vector2F)p
{
    mousePosition_ = p;
}

- (workphone::Vector2F)mousePosition
{
    return mousePosition_;
}

- (void)setFrameSize:(NSSize)s
{
    [super setFrameSize:s];
    //if (renderWindow)
    //    renderWindow->windowMovedOrResized();
}

- (void)drawRect:(NSRect)r
{
    //if(renderWindow)
    //    renderWindow->update();
}

- (void)setBoundsSize:(NSSize)newSize
{
    [super setBoundsSize:newSize];
    _layerSizeDidUpdate = YES;
}

- (void)viewDidChangeBackingProperties
{
    [super viewDidChangeBackingProperties];
    _layerSizeDidUpdate = YES;
}

-(void)handleEvent:(NSEvent *)event
{
    auto applicationManager = workphone::core::ApplicationManager::instance();
    auto graphicsSystem = applicationManager->getGraphicsSystem();
    
    auto windows = graphicsSystem->getWindows();
    for(auto window : windows)
    {
        auto listeners = window->getListeners();
        for(auto listener : listeners)
        {
            auto e = workphone::make_ptr<workphone::WindowMessageData>();

            auto pEvent = (void*)CFBridgingRetain(event);
            e->setEvent(pEvent);

            auto pSelf = (void*)CFBridgingRetain(self);
            e->setSelf(pSelf);

            listener->handleEvent(e);
        }
    }
}

- (void)capture
{
    auto applicationManager = workphone::core::ApplicationManager::instance();
    auto graphicsSystem = applicationManager->getGraphicsSystem();
    auto inputManager = applicationManager->getInputDeviceManager();
    auto window = graphicsSystem->getDefaultWindow();

    auto mouseState = inputManager->getCurrentMouseState();
}

-(void)mouseDown:(NSEvent *)event
{
    [self handleEvent:event];

    using namespace workphone;

    auto applicationManager = workphone::core::ApplicationManager::instance();
    auto graphicsSystem = applicationManager->getGraphicsSystem();
    auto inputManager = applicationManager->getInputDeviceManager();
    auto window = graphicsSystem->getDefaultWindow();

    auto inputEvent = inputManager->createInputEvent();
    inputEvent->setEventType( IInputEvent::EventType::Mouse );

    auto mouseState = inputManager->createMouseState();
    inputEvent->setMouseState( mouseState );

    mouseState->setEventType( IMouseState::Event::LeftPressed );

    //if (event.type == NSEventTypeMouseMoved || event.type == NSEventTypeLeftMouseDragged || event.type == NSEventTypeRightMouseDragged || event.type == NSEventTypeOtherMouseDragged)
    {
        NSPoint mousePoint = event.locationInWindow;
        mousePoint = [self convertPoint:mousePoint fromView:nil];
        mousePoint = NSMakePoint( mousePoint.x, self.bounds.size.height - mousePoint.y );
        //io.AddMousePosEvent((float)mousePoint.x, (float)mousePoint.y);

        auto size = window->getSize();
        auto currentMousePosition = Vector2F( mousePoint.x, mousePoint.y );
        auto relativePosition = self.mousePosition - currentMousePosition;

        mouseState->setDelta( relativePosition );
        mouseState->setRelativePosition( currentMousePosition / Vector2F( size.x, size.y ) );
        mouseState->setAbsolutePosition( currentMousePosition );
        //mouseState->setWheelPosition( static_cast<f32>( arg.state.Z.rel ) );
    }

    inputManager->postEvent(inputEvent);
}

- (void)mouseUp:(NSEvent *)event
{
    [self handleEvent:event];

    using namespace workphone;

    auto applicationManager = workphone::core::ApplicationManager::instance();
    auto graphicsSystem = applicationManager->getGraphicsSystem();
    auto inputManager = applicationManager->getInputDeviceManager();
    auto window = graphicsSystem->getDefaultWindow();

    auto inputEvent = inputManager->createInputEvent();
    inputEvent->setEventType( IInputEvent::EventType::Mouse );

    auto mouseState = inputManager->createMouseState();
    inputEvent->setMouseState( mouseState );

    mouseState->setEventType( IMouseState::Event::LeftReleased );

    //if (event.type == NSEventTypeMouseMoved || event.type == NSEventTypeLeftMouseDragged || event.type == NSEventTypeRightMouseDragged || event.type == NSEventTypeOtherMouseDragged)
    {
        NSPoint mousePoint = event.locationInWindow;
        mousePoint = [self convertPoint:mousePoint fromView:nil];
        mousePoint = NSMakePoint( mousePoint.x, self.bounds.size.height - mousePoint.y );
        //io.AddMousePosEvent((float)mousePoint.x, (float)mousePoint.y);

        auto size = window->getSize();
        auto currentMousePosition = Vector2F( mousePoint.x, mousePoint.y );
        auto relativePosition = self.mousePosition - currentMousePosition;

        mouseState->setDelta( relativePosition );
        mouseState->setRelativePosition( currentMousePosition / Vector2F( size.x, size.y ) );
        mouseState->setAbsolutePosition( currentMousePosition );
        //mouseState->setWheelPosition( static_cast<f32>( arg.state.Z.rel ) );
    }

    inputManager->postEvent(inputEvent);
}


-(void)mouseMoved:(NSEvent *)event
{
    [self handleEvent:event];
}

- (void)rightMouseDown:(NSEvent *)event
{
    [self handleEvent:event];
}

- (void)otherMouseDown:(NSEvent *)event
{
    [self handleEvent:event];
}

- (void)rightMouseUp:(NSEvent *)event
{
    [self handleEvent:event];
}

- (void)otherMouseUp:(NSEvent *)event
{
    [self handleEvent:event];
}

- (void)mouseDragged:(NSEvent *)event
{
    [self handleEvent:event];
}

- (void)rightMouseMoved:(NSEvent *)event
{
    [self handleEvent:event];
}

- (void)rightMouseDragged:(NSEvent *)event
{
    [self handleEvent:event];
}

- (void)otherMouseMoved:(NSEvent *)event
{
    [self handleEvent:event];
}

- (void)otherMouseDragged:(NSEvent *)event
{
    [self handleEvent:event];
}

- (void)scrollWheel:(NSEvent *)event
{
    [self handleEvent:event];
}

/*
- (void)keyDown:(NSEvent*)event
{
    [self handleEvent:event];
}

- (void)keyUp:(NSEvent *)event
{
    [self handleEvent:event];
}

- (void)flagsChanged:(NSEvent *)event
{
    [self handleEvent:event];
}
*/

- (BOOL)becomeFirstResponder {
    return YES;
}

- (BOOL)resignFirstResponder {
    return YES;
}

- (void)updateTrackingAreas
{
    NSRect clipRect = NSMakeRect(0.0f, 0.0f, 0.0f, 0.0f);
    clipRect		= [[[self window] contentView] frame];

    NSRect rect = NSMakeRect(0.0f, 0.0f, clipRect.size.width*2, clipRect.size.height*2);

    if ([self trackingArea])
    {
        [self removeTrackingArea:[self trackingArea]];
    }

    NSTrackingAreaOptions trackingOptions = NSTrackingMouseMoved | NSTrackingEnabledDuringMouseDrag | NSTrackingMouseEnteredAndExited | NSTrackingActiveInActiveApp;

    NSDictionary* trackerData	= [NSDictionary dictionaryWithObjectsAndKeys:
                                                  [NSNumber numberWithInt:0], @"OISMouseTrackingKey", nil];


    NSTrackingArea* newTrackingArea = [[NSTrackingArea alloc]
        initWithRect:[self frame] // in our case track the entire view
             options:trackingOptions
               owner:self
            userInfo:trackerData];
    [self addTrackingArea:newTrackingArea];

    [[self window] setAcceptsMouseMovedEvents:YES];
}



@end
