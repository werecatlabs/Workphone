#pragma once

//#include "OgreMetalPrerequisites.hpp"
#include <WPGraphicsOgre/WPGraphicsOgrePrerequisites.hpp>
#include <Workphone/Interface/Graphics/IInstanceManager.hpp>

#include <Cocoa/Cocoa.hpp>
#import <AppKit/AppKit.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
//#include "OgreMetalPrerequisites.hpp"

@interface WindowView : NSView
{
    fb::render::IWindow* renderWindow;
}

// view has a handle to the metal device when created
@property( nonatomic, readonly ) id<MTLDevice> device;

@property( nonatomic, readwrite ) BOOL layerSizeDidUpdate;

@property( nonatomic, readwrite ) NSTrackingArea* trackingArea;

/// When true (default), we will try to set the contentScaleFactor to the native's.
/// You can use 'nativeScaleFactor' for further control.
/// Note: Changing contentScaleFactor directly will force this value to false.
@property( nonatomic ) bool scaleToNative;

/// When scaleToNative = true, instead of setting self.contentScaleFactor, you
/// should change this setting. It's expressed in fractions of the nativeScale.
///
/// For example on an iPad Mini 3 the native factor is 2.0; thus if you set
/// nativeScaleFactor = 1; then contentScaleFactor = 2.0
/// If you set nativeScaleFactor = 0.5; we'll set contentScaleFactor = 1.0
@property( nonatomic ) CGFloat nativeScaleFactor;

@property( nonatomic ) CGFloat contentScaleFactor;

/// The value of presentationTime will be passed to
/// MTLCommandBuffer::presentDrawable atTime:presentationTime
/// When negative, it means to present immediately (Ogre will call presentDrawable overload)
/// This value is very important if you want to render at e.g. 30 fps:
/// Calling CADisplayLink.frameInterval = 2 is not enough; as the CPU timer will be fired
/// once every two VSync intervals, but if the GPU completes its job too soon, it will
/// present immediately (i.e. in the next 16ms instead of waiting 33ms).
/// In that case you'll want to set presentationTime = displayLink.timestamp+(1.0/fps)
/// See
/// https://developer.apple.com/library/prerelease/content/documentation/3DDrawing/Conceptual/MTLBestPracticesGuide/FrameRate.html
@property( nonatomic ) CFTimeInterval presentationTime;

- (void)setOgreWindow:(fb::render::IWindow*)w;
- (fb::render::IWindow*)ogreWindow;

- (void)setFrameSize:(NSSize)s;

- (void)updateTrackingAreas;

@end