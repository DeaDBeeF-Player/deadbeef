//
//  MediaLibrarySelectorCellView.m
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 8/2/20.
//  Copyright © 2020 Alexey Yakovenko. All rights reserved.
//

#import "MediaLibrarySelectorCellView.h"

@interface MediaLibrarySelectorCellView()

@property (nonatomic,readwrite) NSPopUpButton *popupButton;

@end

@implementation MediaLibrarySelectorCellView

- (instancetype)initWithCoder:(NSCoder *)coder
{
    self = [super initWithCoder:coder];
    if (!self) {
        return nil;
    }

    self.popupButton = [[NSPopUpButton alloc] initWithFrame:self.bounds];
    [self.popupButton addItemWithTitle:@"Genres"];
    [self.popupButton addItemWithTitle:@"Albums"];
    [self.popupButton addItemWithTitle:@"Artists"];
    [self.popupButton addItemWithTitle:@"Folders"];

    self.popupButton.autoresizingMask = NSViewMinXMargin | NSViewWidthSizable | NSViewMaxXMargin | NSViewMinYMargin | NSViewHeightSizable | NSViewMaxYMargin;

    self.popupButton.action = @selector(popupButtonAction:);
    self.popupButton.target = self;

    [self addSubview:self.popupButton];

    return self;
}

- (void)popupButtonAction:(NSPopUpButton *)sender {
    [self.delegate filterSelectorChanged:self];
}

@end
