//
//  Based on AppleMediaKeyController.m
//  https://gist.github.com/gauravk92/546311
//
//  This version has been modified for use in DeaDBeeF Player
//
//  The original copyright notice:
//
//  AppleMediaKeyController.m
//
//  Modified by Gaurav Khanna on 8/17/10.
//  SOURCE: http://github.com/sweetfm/SweetFM/blob/master/Source/HMediaKeys.m
//  SOURCE: http://stackoverflow.com/questions/2969110/cgeventtapcreate-breaks-down-mysteriously-with-key-down-events//
//
//  Permission is hereby granted, free of charge, to any person
//  obtaining a copy of this software and associated documentation
//  files (the "Software"), to deal in the Software without restriction,
//  including without limitation the rights to use, copy, modify,
//  merge, publish, distribute, sublicense, and/or sell copies of
//  the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be
//  included in all copies or substantial portions of the Software.
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
//  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
//  OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
//  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
//  ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
//  CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
//  WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//

#import "MediaKeyController.h"
#import <IOKit/hidsystem/ev_keymap.h>
#include "../../deadbeef.h"

extern DB_functions_t *deadbeef;

#define NX_KEYSTATE_UP      0x0A
#define NX_KEYSTATE_DOWN    0x0B

static CFMachPortRef _eventPort;
static CFRunLoopSourceRef _runLoopSource;

CGEventRef tapEventCallback (CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *refcon) {
    if(type == kCGEventTapDisabledByTimeout) {
        ;
        CGEventTapEnable(_eventPort, TRUE);
    }

    if(type != NX_SYSDEFINED)
        return event;

    NSEvent *nsEvent = [NSEvent eventWithCGEvent:event];

    if([nsEvent subtype] != 8)
        return event;

    long data = [nsEvent data1];
    int keyCode = (data & 0xFFFF0000) >> 16;
    int keyFlags = (data & 0xFFFF);
    int keyState = (keyFlags & 0xFF00) >> 8;
    BOOL keyIsRepeat = (keyFlags & 0x1) > 0;

    if(keyIsRepeat)
        return event;

    switch (keyCode) {
        case NX_KEYTYPE_PLAY:
            if(keyState == NX_KEYSTATE_DOWN) {
                int state = deadbeef->get_output ()->state ();
                if (state == OUTPUT_STATE_PLAYING) {
                    deadbeef->sendmessage (DB_EV_PAUSE, 0, 0, 0);
                }
                else {
                    deadbeef->sendmessage (DB_EV_PLAY_CURRENT, 0, 0, 0);
                }
            }
            if(keyState == NX_KEYSTATE_UP || keyState == NX_KEYSTATE_DOWN)
                return NULL;
            break;
        case NX_KEYTYPE_FAST:
            if(keyState == NX_KEYSTATE_DOWN)
                deadbeef->sendmessage (DB_EV_NEXT, 0, 0, 0);
            if(keyState == NX_KEYSTATE_UP || keyState == NX_KEYSTATE_DOWN)
                return NULL;
            break;
        case NX_KEYTYPE_REWIND:
            if(keyState == NX_KEYSTATE_DOWN)
                deadbeef->sendmessage (DB_EV_PREV, 0, 0, 0);
            if(keyState == NX_KEYSTATE_UP || keyState == NX_KEYSTATE_DOWN)
                return NULL;
            break;
    }
    return event;
}

void grabMediaKeys (void) {
    CFRunLoopRef runLoop;

    _eventPort = CGEventTapCreate(kCGSessionEventTap,
                                  kCGHeadInsertEventTap,
                                  kCGEventTapOptionDefault,
                                  CGEventMaskBit(NX_SYSDEFINED),
                                  tapEventCallback,
                                  NULL);

    if(_eventPort == NULL) {
        NSLog(@"Fatal Error: Event Tap could not be created");
        return;
    }

    _runLoopSource = CFMachPortCreateRunLoopSource(kCFAllocatorSystemDefault, _eventPort, 0);

    if(_runLoopSource == NULL) {
        NSLog(@"Fatal Error: Run Loop Source could not be created");
        CFRelease(_eventPort);
        return;
    }

    runLoop = CFRunLoopGetCurrent();

    if(runLoop == NULL) {
        NSLog(@"Fatal Error: Couldn't get current threads Run Loop");
        CFRelease(_eventPort);
        CFRelease(_runLoopSource);
        return;
    }

    CFRunLoopAddSource(runLoop, _runLoopSource, kCFRunLoopCommonModes);
}

void ungrabMediaKeys (void) {
    CFRelease(_eventPort);
    CFRelease(_runLoopSource);
}
