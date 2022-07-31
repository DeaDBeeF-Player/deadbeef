//
//  HeaderView.h
//  DeaDBeeF
//
//  Created by Oleksiy Yakovenko on 2/1/20.
//  Copyright © 2020 Oleksiy Yakovenko. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@class PlaylistView;

NS_ASSUME_NONNULL_BEGIN

@interface PlaylistHeaderView : NSView

@property (nonatomic,weak) PlaylistView *listview;

@end

NS_ASSUME_NONNULL_END
