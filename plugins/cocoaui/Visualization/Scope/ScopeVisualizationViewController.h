//
//  ScopeVisualizationViewController.h
//  deadbeef
//
//  Created by Alexey Yakovenko on 30/10/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//

#import "VisualizationViewController.h"
#import "ScopeSettings.h"

@interface ScopeVisualizationViewController : VisualizationViewController

@property (nonatomic,nullable) ScopeSettings *settings;

- (void)updateScopeSettings:(nonnull ScopeSettings *)settings;
- (void)message:(uint32_t)_id ctx:(uintptr_t)ctx p1:(uint32_t)p1 p2:(uint32_t)p2;

@end
