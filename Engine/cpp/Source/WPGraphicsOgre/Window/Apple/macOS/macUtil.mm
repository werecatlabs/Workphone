#import "WPGraphicsOgre/Window/Apple/macOS/macUtil.h"
#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>
#import <dlfcn.h>

namespace fb {


    void mac_dispatchOneEvent()
    {
#if 1
        NSApplication *app = NSApplication.sharedApplication;
        NSEvent *event = [app nextEventMatchingMask:NSEventMaskAny
                                          untilDate:[NSDate distantPast]
                                             inMode:NSDefaultRunLoopMode
                                            dequeue:YES];

        if( event != nil )
        {
            [app sendEvent:event];
        }
#else
        NSApplication *app = NSApplication.sharedApplication;
        //NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
        for ( ; ; )
        {

            NSEvent *event = [app nextEventMatchingMask:NSAnyEventMask
                                              untilDate:[NSDate distantPast]
                                                 inMode:NSDefaultRunLoopMode
                                                dequeue:YES];

            /* NSEvent *event = [app nextEventMatchingMask:NSAnyEventMask
                                               untilDate:[NSDate distantPast]
                                                  inMode:NSDefaultRunLoopMode
                                                 dequeue:YES];*/

            /*NSEvent *event = [app nextEventMatchingMask:NSAnyEventMask
                                              untilDate:[NSDate distantFuture]
                                                 inMode:NSDefaultRunLoopMode
                                                dequeue:YES];*/
            if( event != nil )
            {
                [app sendEvent:event];
            }
            else
            {
                break;
            }
        }

        // [pool release];

        //if( event != nil )
        //{
        //    [app sendEvent:event];
        //}
#endif
    }
}