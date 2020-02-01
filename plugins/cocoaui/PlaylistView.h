/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2015 Alexey Yakovenko and other contributors

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

#import "DdbWidget.h"
#import "PlaylistHeaderView.h"
#import "PlaylistContentView.h"

@interface PlaylistView : NSView

@property (nonatomic,readonly) PlaylistHeaderView *headerView;
@property (nonatomic,readonly) PlaylistContentView *contentView;
//@property (readonly) DdbListviewGroup_t *groups;
//@property (readonly) int grouptitle_height;
//@property (readonly) int fullheight;
//@property (readwrite) NSPoint lastpos;
//@property (readwrite) int shift_sel_anchor;
@property (weak,nonatomic) id<DdbListviewDelegate> delegate;

//- (void)reloadData;
//- (void)groupCheck;
//- (int)pickPoint:(int)y group:(DdbListviewGroup_t **)group groupIndex:(int *)group_idx index:(int *)global_idx;
//- (void)drawRow:(int)idx;
//- (void)drawGroup:(int)idx;
//- (void)listMouseUp:(NSEvent *)event;
//- (void)listMouseDragged:(NSEvent *)event;
//- (void)setCursor:(int)cursor noscroll:(BOOL)noscroll;
//- (void)scrollToRowWithIndex:(int)idx;
//- (void)setVScroll:(int)scroll;
- (void)updateContentFrame;
//- (void)cleanup;
@end
