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

#import <WPGraphicsOgre/Window/Apple/macOS/WindowView.h>
#include <Workphone/Workphone.h>
//#import "OgreMetalView.h"
#import <QuartzCore/QuartzCore.h>
#import <SpriteKit/SpriteKit.h>
#import <AppKit/AppKit.h>

@implementation WindowView

- (void)viewDidMoveToWindow
{
    // if(self.scaleToNative)
    //    [setContentScaleFactor:self.window.screen.backingScaleFactor * self.nativeScaleFactor];
    _layerSizeDidUpdate = YES;
}

- (id)initWithFrame:(NSRect)frame
{
    self = [super initWithFrame:frame];

    if( self )
    {
        NSApplicationLoad();
        renderWindow = 0;
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
    [super setContentScaleFactor:contentScaleFactor];
    _layerSizeDidUpdate = YES;
}

- (void)setOgreWindow:(fb::render::IWindow*)w
{
    renderWindow = w;
}

- (fb::render::IWindow*)ogreWindow
{
    return renderWindow;
}

- (void)setFrameSize:(NSSize)s
{
    [super setFrameSize:s];
    if (renderWindow)
        renderWindow->windowMovedOrResized();
}

- (void)drawRect:(NSRect)r
{
    //if(renderWindow)
    //    renderWindow->update();
}

- (BOOL)acceptsFirstResponder
{
    return NO;
}

- (BOOL)canBecomeKeyView
{
    return NO;
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
    auto applicationManager = fb::core::IApplicationManager::instance();
    auto graphicsSystem = applicationManager->getGraphicsSystem();
    
    auto windows = graphicsSystem->getWindows();
    for(auto window : windows)
    {
        auto listeners = window->getListeners();
        for(auto listener : listeners)
        {
            auto e = fb::make_ptr<fb::WindowMessageData>();

            auto pEvent = (void*)CFBridgingRetain(event);
            e->setEvent(pEvent);

            auto pSelf = (void*)CFBridgingRetain(self);
            e->setSelf(pSelf);

            listener->handleEvent(e);
        }
    }
}

-(void)mouseDown:(NSEvent *)event
{
    [self handleEvent:event];
}

- (void)mouseUp:(NSEvent *)event
{
    [self handleEvent:event];
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

- (void)updateTrackingAreas
{
    NSRect clipRect = NSMakeRect(0.0f, 0.0f, 0.0f, 0.0f);
    clipRect		= [[[self window] contentView] frame];

    NSRect rect = NSMakeRect(0.0f, 0.0f, clipRect.size.width * 2.0f, clipRect.size.height * 2.0f);

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
