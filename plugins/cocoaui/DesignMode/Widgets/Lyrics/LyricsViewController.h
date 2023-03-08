//
//  LyricsViewController.h
//  deadbeef
//
//  Created by Oleksiy Yakovenko on 21/11/2021.
//  Copyright © 2021 Oleksiy Yakovenko. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#include <deadbeef/deadbeef.h>

NS_ASSUME_NONNULL_BEGIN

@interface LyricsViewController : NSViewController

@property (nonatomic,nullable) ddb_playItem_t *track;


@end

NS_ASSUME_NONNULL_END
