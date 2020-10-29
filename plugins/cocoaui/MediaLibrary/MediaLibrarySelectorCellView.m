//
//  MediaLibrarySelectorCellView.m
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 8/2/20.
//  Copyright © 2020 Alexey Yakovenko. All rights reserved.
//

#include "deadbeef.h"
#import "MediaLibrarySelectorCellView.h"

@implementation MediaLibrarySelectorCellView

- (void)awakeFromNib {
    self.popupButton.action = @selector(popupButtonAction:);
    self.popupButton.target = self;
}

- (void)popupButtonAction:(NSPopUpButton *)sender {
    [self.delegate filterSelectorChanged:self];
}

@end
