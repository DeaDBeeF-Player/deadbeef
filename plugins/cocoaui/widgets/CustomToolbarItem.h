//
//  CustomToolbarItem.h
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 3/9/20.
//  Copyright © 2020 Alexey Yakovenko. All rights reserved.
//

#import <Cocoa/Cocoa.h>

NS_ASSUME_NONNULL_BEGIN

@interface CustomToolbarItem : NSToolbarItem

@property IBOutlet NSView *customView;

@end

NS_ASSUME_NONNULL_END
