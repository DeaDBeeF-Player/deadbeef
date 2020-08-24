//
//  MediaLibrarySelectorCellView.m
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 8/2/20.
//  Copyright © 2020 Alexey Yakovenko. All rights reserved.
//

#import "MediaLibrarySelectorCellView.h"

@implementation MediaLibrarySelectorCellView

- (void)awakeFromNib {
    [self.popupButton removeAllItems];

    [self.popupButton addItemWithTitle:@"Genres"];
    [self.popupButton addItemWithTitle:@"Albums"];
    [self.popupButton addItemWithTitle:@"Artists"];
    [self.popupButton addItemWithTitle:@"Folders"];

    self.popupButton.action = @selector(popupButtonAction:);
    self.popupButton.target = self;
}

- (void)popupButtonAction:(NSPopUpButton *)sender {
    [self.delegate filterSelectorChanged:self];
}

@end
