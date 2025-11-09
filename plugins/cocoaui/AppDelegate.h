/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2014 Oleksiy Yakovenko and other contributors

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

#import <Cocoa/Cocoa.h>
#import "MainWindowController.h"
#import "MediaLibraryManager.h"
#import "SearchWindowController.h"
#import "PreferencesWindowController.h"
#include <deadbeef/deadbeef.h>

#define MAX_COLUMNS 20

@interface AppDelegate : NSObject <NSApplicationDelegate>

- (int)ddb_message:(int)_id ctx:(uint64_t)ctx p1:(uint32_t)p1 p2:(uint32_t)p2;

@property (nonatomic) MainWindowController *mainWindow;

@property (nonatomic,readonly) MediaLibraryManager *mediaLibraryManager;

@end
