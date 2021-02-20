//
//  PlaylistWidget.h
//  deadbeef
//
//  Created by Alexey Yakovenko on 20/02/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "WidgetBase.h"

// HACK: we still require access to the view controller from main window, because ARC cleanup is bugged, and we need to call cleanup method directly
@class PlaylistViewController;

NS_ASSUME_NONNULL_BEGIN

@interface PlaylistWidget : WidgetBase<WidgetProtocol>

@property (nonatomic,readonly) PlaylistViewController *viewController;

@end

NS_ASSUME_NONNULL_END
