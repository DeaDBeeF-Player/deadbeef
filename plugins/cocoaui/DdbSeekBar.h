//
//  DdbSeekBar.h
//  deadbeef
//
//  Created by waker on 27/08/14.
//  Copyright (c) 2014 Alexey Yakovenko. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface DdbSeekBar : NSSliderCell

@property NSImage *knobImg;

@property NSGradient* gradBtmOuter;
@property NSGradient* gradBtmInner;
@property NSGradient* gradTopOuter;
@property NSGradient* gradTopInner;

@end
