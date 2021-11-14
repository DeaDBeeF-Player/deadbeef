//
//  ScopePreferencesViewController.h
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 14/11/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@class ScopeSettings;

NS_ASSUME_NONNULL_BEGIN

@interface ScopePreferencesViewController : NSViewController

@property (nullable, nonatomic) ScopeSettings *settings;
@property (nullable, nonatomic, weak) NSPopover *popover;

@end

NS_ASSUME_NONNULL_END
