/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2016 Oleksiy Yakovenko and other contributors

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
#import "PlaylistView.h"
#import "ConverterWindowController.h"

typedef NS_ENUM(NSInteger, PlaylistColumnAlignment) {
    ColumnAlignmentLeft = 0,
    ColumnAlignmentCenter = 1,
    ColumnAlignmentRight = 2,
};

typedef struct {
    char *title;
    int type; // predefined col type
    char *format;
    char *sortFormat;
    int size;
    PlaylistColumnAlignment alignment;
    int set_text_color;
    uint8_t text_color[4];
    char *bytecode;
    int sort_order;
} plt_col_info_t;

@interface PlaylistViewController : NSViewController

@property (nonatomic,readonly) plt_col_info_t *columns;
@property (nonatomic,readonly) int ncolumns;

- (void)cleanup;
- (int)sendMessage:(uint32_t)_id ctx:(uintptr_t)ctx p1:(uint32_t)p1 p2:(uint32_t)p2;

@end
