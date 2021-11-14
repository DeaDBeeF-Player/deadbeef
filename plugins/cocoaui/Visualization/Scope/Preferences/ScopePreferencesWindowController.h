//
//  ScopePreferencesWindowController.h
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 14/11/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@class ScopeSettings;

NS_ASSUME_NONNULL_BEGIN

@interface ScopePreferencesWindowController : NSWindowController

@property (nonatomic) ScopeSettings *settings;

@end

NS_ASSUME_NONNULL_END
