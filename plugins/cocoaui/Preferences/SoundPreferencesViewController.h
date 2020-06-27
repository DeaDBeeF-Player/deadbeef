//
//  PlaybackPreferencesViewController.h
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 2/25/20.
//  Copyright © 2020 Alexey Yakovenko. All rights reserved.
//

#import <Cocoa/Cocoa.h>

NS_ASSUME_NONNULL_BEGIN

@interface SoundPreferencesViewController : NSViewController

- (void)outputDeviceChanged;

@end

NS_ASSUME_NONNULL_END
