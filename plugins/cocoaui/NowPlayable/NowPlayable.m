//
//  NowPlayable.m
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 11/4/19.
//  Copyright © 2019 Alexey Yakovenko. All rights reserved.
//

#import "NowPlayable.h"
#import <MediaPlayer/MediaPlayer.h>
#include "deadbeef.h"

extern DB_functions_t *deadbeef;

@implementation NowPlayable

- (instancetype)init
{
    self = [super init];
    if (self) {
        MPNowPlayingInfoCenter *infoCenter = [MPNowPlayingInfoCenter defaultCenter];
        infoCenter.nowPlayingInfo = @{
            MPNowPlayingInfoPropertyPlaybackRate: @1.0,
        };

        MPRemoteCommandCenter *commandCenter = [MPRemoteCommandCenter sharedCommandCenter];

        [commandCenter.togglePlayPauseCommand addTarget:self action:@selector(togglePlayPauseCommand:)];
        commandCenter.togglePlayPauseCommand.enabled = YES;

        [commandCenter.nextTrackCommand addTarget:self action:@selector(nextTrackCommand:)];
        commandCenter.nextTrackCommand.enabled = YES;

        [commandCenter.previousTrackCommand addTarget:self action:@selector(previousTrackCommand:)];
        commandCenter.previousTrackCommand.enabled = YES;
    }
    return self;
}

- (void)dealloc
{
    MPRemoteCommandCenter *commandCenter = [MPRemoteCommandCenter sharedCommandCenter];
    [commandCenter.togglePlayPauseCommand removeTarget:self];
    [commandCenter.nextTrackCommand removeTarget:self];
    [commandCenter.previousTrackCommand removeTarget:self];
}

- (MPRemoteCommandHandlerStatus)togglePlayPauseCommand:(MPRemoteCommandEvent *)sender {
    int state = deadbeef->get_output ()->state ();
    if (state == OUTPUT_STATE_PLAYING) {
        deadbeef->sendmessage (DB_EV_PAUSE, 0, 0, 0);
    }
    else {
        deadbeef->sendmessage (DB_EV_PLAY_CURRENT, 0, 0, 0);
    }
    return MPRemoteCommandHandlerStatusSuccess;
}

- (MPRemoteCommandHandlerStatus)nextTrackCommand:(MPRemoteCommandEvent *)sender {
    deadbeef->sendmessage (DB_EV_NEXT, 0, 0, 0);
    return MPRemoteCommandHandlerStatusSuccess;
}

- (MPRemoteCommandHandlerStatus)previousTrackCommand:(MPRemoteCommandEvent *)sender {
    deadbeef->sendmessage (DB_EV_PREV, 0, 0, 0);
    return MPRemoteCommandHandlerStatusSuccess;
}

@end
