//
//  TrackPositionFormatter.m
//  deadbeef
//
//  Created by Oleksiy Yakovenko on 11/07/2021.
//  Copyright © 2021 Oleksiy Yakovenko. All rights reserved.
//

#import "TrackPositionFormatter.h"
#include <deadbeef/deadbeef.h>

extern DB_functions_t *deadbeef;

@implementation TrackPositionFormatter

- (NSString *)stringForObjectValue:(NSControl *)obj {

    DB_playItem_t *track = deadbeef->streamer_get_playing_track_safe ();
    if (!track) {
        return @"--:--:--";
    }

    double pos = obj.doubleValue / 100;
    float duration = 0;
    duration = deadbeef->pl_get_item_duration (track);
    double time = duration * pos;
    int hr = time/3600;
    int mn = (time-hr*3600)/60;
    int sc = round(time-hr*3600-mn*60);

    NSString *res = [NSString stringWithFormat:@"%02d:%02d:%02d", hr, mn, sc];

    deadbeef->pl_item_unref (track);

    return res;
}

- (BOOL)getObjectValue:(out id  _Nullable __autoreleasing *)obj forString:(NSString *)string errorDescription:(out NSString * _Nullable __autoreleasing *)error {
    *error = @"error";
    return NO;
}

@end
