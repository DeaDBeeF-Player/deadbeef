//
//  PropertySheetContentView.m
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 2/10/20.
//  Copyright © 2020 Alexey Yakovenko. All rights reserved.
//

#import "PropertySheetContentView.h"

@implementation PropertySheetContentView

- (NSSize)intrinsicContentSize {
    return self.contentSize;
}

- (void)setContentSize:(NSSize)contentSize {
    _contentSize = contentSize;
    [self invalidateIntrinsicContentSize];
}

@end

