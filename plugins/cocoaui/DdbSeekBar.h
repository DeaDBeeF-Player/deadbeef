//
//  DdbSeekBar.h
//  deadbeef
//
//  Created by waker on 27/08/14.
//  Copyright (c) 2014 Alexey Yakovenko. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface DdbSeekBar : NSSlider
@end

@interface DdbSeekBarCell : NSSliderCell {
#if 0
    NSImage *_backCapLeft;
    NSImage *_backCapRight;
    NSImage *_backFiller;
#endif
    NSImage *_frontCapLeft;
    NSImage *_frontCapRight;
    NSImage *_frontFiller;
}
@end
