//
//  CustomToolbarItem.m
//  DeaDBeeF
//
//  Created by Oleksiy Yakovenko on 3/9/20.
//  Copyright © 2020 Oleksiy Yakovenko. All rights reserved.
//

#import "CustomToolbarItem.h"

@implementation CustomToolbarItem

- (void)awakeFromNib {
    self.view = self.customView;
}

@end
