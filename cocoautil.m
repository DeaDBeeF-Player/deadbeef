/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2014 Alexey Yakovenko and other contributors

    This software is provided 'as-is', without any express or implied
    warranty.  In no event will the authors be held liable for any damages
    arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.

    3. This notice may not be removed or altered from any source distribution.
*/

#import "cocoautil.h"
#import <Foundation/Foundation.h>

int
cocoautil_get_resources_path (char *s, int size) {
    strcpy (s, [[[NSBundle mainBundle] resourcePath] UTF8String]);
    return 0;
}

void
cocoautil_backtrace (void) {
    NSLog(@"%@",[NSThread callStackSymbols]);
}

int
cocoautil_get_library_path (char *s, int size) {
    *s = 0;

    NSArray* paths = NSSearchPathForDirectoriesInDomains( NSLibraryDirectory, NSUserDomainMask, YES );
    if (![paths count]) {
        return -1;
    }
    strncat (s, [paths[0] UTF8String], size);
    return 0;
}
