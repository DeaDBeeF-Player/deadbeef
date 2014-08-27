//
//  DdbSeekBar.m
//  deadbeef
//
//  Created by waker on 27/08/14.
//  Copyright (c) 2014 Alexey Yakovenko. All rights reserved.
//

#import "DdbSeekBar.h"

@implementation DdbSeekBar

@synthesize knobImg;

- (id)initWithCoder:(NSCoder *)decoder
{
    self = [super initWithCoder:decoder];
    if (self) {
        knobImg = [NSImage imageNamed:@"knobTemplate.pdf"];
    }
    return self;
}

- (void)drawBarInside:(NSRect)aRect flipped:(BOOL)flipped
{
    NSRect innerRect = aRect;
    innerRect.size.width -= 2;
    innerRect.origin.x += 1;

    aRect.size.height += 2;
    aRect.origin.y -= 1;
    
    NSColor *color = [NSColor selectedControlTextColor];
    
    // outer 1
    NSBezierPath*    clipShape = [NSBezierPath bezierPath];
    [clipShape appendBezierPathWithRoundedRect:aRect xRadius:3 yRadius:3];
    
    NSGradient* aGradient = [[NSGradient alloc]
                             initWithColorsAndLocations:[color highlightWithLevel:0.4], (CGFloat)0.0,
                             [color highlightWithLevel:0.6], (CGFloat)0.5,
                             [color highlightWithLevel:0.7], (CGFloat)1.0,
                              nil];
    
    [aGradient drawInBezierPath:clipShape angle:90];
    
    // inner 1
    aGradient = [[NSGradient alloc]
                 initWithColorsAndLocations:[color highlightWithLevel:0.5], (CGFloat)0.0,
                 [color highlightWithLevel:0.75], (CGFloat)0.5,
                 [color highlightWithLevel:0.8], (CGFloat)1.0,
                 nil];

    clipShape = [NSBezierPath bezierPath];
    [clipShape appendBezierPathWithRoundedRect:innerRect xRadius:3 yRadius:3];
    
    [aGradient drawInBezierPath:clipShape angle:90];

    innerRect.size.width *= [self floatValue] / ([self maxValue] - [self minValue]);
    if (innerRect.size.width > 0) {
        aRect.size.width = innerRect.size.width+2;
        // outer 2
        clipShape = [NSBezierPath bezierPath];
        [clipShape appendBezierPathWithRoundedRect:aRect xRadius:3 yRadius:3];
        
        aGradient = [[NSGradient alloc]
                                 initWithColorsAndLocations:[color highlightWithLevel:0.1], (CGFloat)0.0,
                                 [color highlightWithLevel:0.2], (CGFloat)0.5,
                                 [color highlightWithLevel:0.3], (CGFloat)1.0,
                                 nil];
        
        [aGradient drawInBezierPath:clipShape angle:90];

        
        // inner 2
        aGradient = [[NSGradient alloc]
                     initWithColorsAndLocations:[color highlightWithLevel:0.2], (CGFloat)0.0,
                     [color highlightWithLevel:0.45], (CGFloat)0.6,
                     [color highlightWithLevel:0.5], (CGFloat)1.0,
                     nil];
        

        clipShape = [NSBezierPath bezierPath];
        [clipShape appendBezierPathWithRoundedRect:innerRect xRadius:3 yRadius:3];
        
        [aGradient drawInBezierPath:clipShape angle:90];
    }
}

-(void)drawKnob:(NSRect)knobRect
{
    /*if ([self isEnabled]) {
        knobRect.origin.x += (knobRect.size.width-8)/2;
        knobRect.origin.y += (knobRect.size.height-8)/2;
        knobRect.size.width = knobRect.size.height = 8;
        [knobImg drawInRect:knobRect];
    }*/
}

@end
