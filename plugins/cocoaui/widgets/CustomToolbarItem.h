//
//  CustomToolbarItem.h
//  DeaDBeeF
//
//  Created by Oleksiy Yakovenko on 3/9/20.
//  Copyright © 2020 Oleksiy Yakovenko. All rights reserved.
//

#import <Cocoa/Cocoa.h>

NS_ASSUME_NONNULL_BEGIN

@interface CustomToolbarItem : NSToolbarItem

@property IBOutlet NSView *customView;

@end

NS_ASSUME_NONNULL_END
