//
//  SeekbarOverlay.m
//  DeaDBeeF
//
//  Created by Oleksiy Yakovenko on 3/13/20.
//  Copyright © 2020 Oleksiy Yakovenko. All rights reserved.
//

#import <QuartzCore/CATextLayer.h>
#import "SeekbarOverlay.h"


@interface SeekbarOverlay() <CALayerDelegate>

@property (nonatomic) CATextLayer *textLayer;
@property (nonatomic) NSFont *font;

@end

@implementation SeekbarOverlay

- (instancetype)initWithFrame:(CGRect)frame
{
    self = [super initWithFrame:frame];
    if (!self) {
        return nil;
    }
    [self setup];
    return self;
}

- (instancetype)initWithCoder:(NSCoder *)coder
{
    self = [super initWithCoder:coder];
    if (!self) {
        return nil;
    }
    [self setup];
    return self;
}

- (void)setup {
    self.wantsLayer = YES;

    self.layer = [CALayer new];
    self.layer.delegate = self;
    self.layer.backgroundColor = [[NSColor.windowBackgroundColor shadowWithLevel:0.7] colorWithAlphaComponent:0.5].CGColor;
    self.layer.cornerRadius = 4;

    self.textLayer = [CATextLayer new];
    self.font = [NSFont systemFontOfSize:20 weight:NSFontWeightBold];
    self.textLayer.foregroundColor = [NSColor.whiteColor shadowWithLevel:0.1].CGColor;
    self.textLayer.font = (__bridge CFTypeRef _Nullable)self.font;
    self.textLayer.fontSize = self.font.pointSize;
    self.textLayer.alignmentMode = kCAAlignmentCenter;
    self.textLayer.contentsScale = NSScreen.mainScreen.backingScaleFactor;

    [self.layer addSublayer:self.textLayer];
}

- (void)layoutSublayersOfLayer:(CALayer *)layer {
    if (layer == self.layer) {
        NSAttributedString *string = [[NSAttributedString alloc] initWithString:@"Sj" attributes:@{
            NSFontAttributeName: self.font
        }];
        NSSize size = string.size;
        NSRect frame = self.layer.bounds;
        frame.origin.y = (self.layer.bounds.size.height - size.height)/2;
        frame.size.height = size.height;
        self.textLayer.frame = frame;
    }
}

- (NSSize)intrinsicContentSize {
    return NSMakeSize(120, 26);
}

- (void)setText:(NSString *)text {
    self.textLayer.string = text;
}

@end
