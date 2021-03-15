//
//  PinnedGroupTitleView.h
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 2/28/20.
//  Copyright © 2020 Alexey Yakovenko. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "DdbListviewDelegate.h"

NS_ASSUME_NONNULL_BEGIN

@interface PinnedGroupTitleView : NSView

@property (nonatomic) PlaylistGroup *group;
@property (nonatomic, weak) id<DdbListviewDelegate> delegate;

@end

NS_ASSUME_NONNULL_END
