//
//  ScopeSettings.h
//  deadbeef
//
//  Created by Oleksiy Yakovenko on 30/10/2021.
//  Copyright © 2021 Oleksiy Yakovenko. All rights reserved.
//

#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>
#import "scope.h"

typedef NS_ENUM(NSInteger, ScopeScaleMode) {
    ScopeScaleModeAuto,
    ScopeScaleMode1x,
    ScopeScaleMode2x,
    ScopeScaleMode3x,
    ScopeScaleMode4x,
};

typedef NS_ENUM(NSInteger, ScopeFragmentDuration) {
    ScopeFragmentDuration50,
    ScopeFragmentDuration100,
    ScopeFragmentDuration200,
    ScopeFragmentDuration300,
    ScopeFragmentDuration500,
};

NS_ASSUME_NONNULL_BEGIN

@interface ScopeSettings : NSObject

@property (nonatomic) ddb_scope_mode_t renderMode;
@property (nonatomic) ScopeScaleMode scaleMode;
@property (nonatomic) ScopeFragmentDuration fragmentDuration;
@property (nonatomic) BOOL useCustomColor;
@property (nonatomic) BOOL useCustomBackgroundColor;
@property (nonatomic) NSColor *customColor;
@property (nonatomic) NSColor *customBackgroundColor;

@end

NS_ASSUME_NONNULL_END
