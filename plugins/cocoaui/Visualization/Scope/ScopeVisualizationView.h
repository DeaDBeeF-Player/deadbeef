//
//  ScopeVisualizationView.h
//  deadbeef
//
//  Created by Alexey Yakovenko on 18/10/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import <MetalKit/MTKView.h>
#import "ScopeSettings.h"

NS_ASSUME_NONNULL_BEGIN

@interface ScopeVisualizationView : MTKView

- (void)updateScopeSettings:(ScopeSettings *)settings;

@end

NS_ASSUME_NONNULL_END
