//
//  CustomToolbarItem.m
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 3/9/20.
//  Copyright © 2020 Alexey Yakovenko. All rights reserved.
//

#import "CustomToolbarItem.h"
#import "DdbSeekBar.h"

@implementation CustomToolbarItem

- (void)awakeFromNib {
    self.view = self.customView;
}

@end
