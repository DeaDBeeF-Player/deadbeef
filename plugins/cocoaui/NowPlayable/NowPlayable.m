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

@interface NowPlayable()

@property (nonatomic) NSTimer *updateTimer;

@end

@implementation NowPlayable

- (instancetype)init
{
    self = [super init];
    if (!self) {
        return nil;
    }

    MPNowPlayingInfoCenter *infoCenter = [MPNowPlayingInfoCenter defaultCenter];
    infoCenter.nowPlayingInfo = @{
        MPNowPlayingInfoPropertyPlaybackRate: @1.0,
    };

    MPRemoteCommandCenter *commandCenter = [MPRemoteCommandCenter sharedCommandCenter];

    [commandCenter.togglePlayPauseCommand addTarget:self action:@selector(togglePlayPauseCommand:)];
    commandCenter.togglePlayPauseCommand.enabled = YES;

    [commandCenter.playCommand addTarget:self action:@selector(playCommand:)];
    commandCenter.playCommand.enabled = YES;

    [commandCenter.pauseCommand addTarget:self action:@selector(pauseCommand:)];
    commandCenter.pauseCommand.enabled = YES;

    [commandCenter.nextTrackCommand addTarget:self action:@selector(nextTrackCommand:)];
    commandCenter.nextTrackCommand.enabled = YES;

    [commandCenter.previousTrackCommand addTarget:self action:@selector(previousTrackCommand:)];
    commandCenter.previousTrackCommand.enabled = YES;

    [commandCenter.changePlaybackPositionCommand addTarget:self action:@selector(changePlaybackPositionCommand:)];
    commandCenter.changePlaybackPositionCommand.enabled = YES;

    __weak NowPlayable *weakSelf = self;

    self.updateTimer = [NSTimer scheduledTimerWithTimeInterval:0.5 repeats:YES block:^(NSTimer * _Nonnull timer) {
        NowPlayable *self = weakSelf;
        if (!self) {
            return;
        }

        NSMutableDictionary *info = [NSMutableDictionary new];
        info[MPNowPlayingInfoPropertyPlaybackRate] = @1.0;

        DB_playItem_t *it = deadbeef->streamer_get_playing_track ();

        if (it) {
            float duration = deadbeef->pl_get_item_duration (it);
            if (duration >= 0) {
                info[MPMediaItemPropertyPlaybackDuration] = @((NSTimeInterval)duration);
                info[MPNowPlayingInfoPropertyElapsedPlaybackTime] = @((NSTimeInterval)deadbeef->streamer_get_playpos ());
                info[MPNowPlayingInfoPropertyIsLiveStream] = @NO;
            }
            else {
                info[MPNowPlayingInfoPropertyIsLiveStream] = @YES;
            }
            deadbeef->pl_item_unref (it);
        }

        info[MPNowPlayingInfoPropertyMediaType] = @(MPNowPlayingInfoMediaTypeAudio);

        MPNowPlayingInfoCenter *infoCenter = [MPNowPlayingInfoCenter defaultCenter];

        infoCenter.nowPlayingInfo = info;

        ddb_playback_state_t state = deadbeef->get_output ()->state ();
        switch (state) {
        case DDB_PLAYBACK_STATE_PLAYING:
            infoCenter.playbackState = MPNowPlayingPlaybackStatePlaying;
            break;
        case DDB_PLAYBACK_STATE_PAUSED:
            infoCenter.playbackState = MPNowPlayingPlaybackStatePaused;
            break;
        case DDB_PLAYBACK_STATE_STOPPED:
            infoCenter.playbackState = MPNowPlayingPlaybackStateStopped;
            break;
        }
    }];

    return self;
}

- (void)dealloc
{
    [self.updateTimer invalidate];
    self.updateTimer = nil;
    MPRemoteCommandCenter *commandCenter = [MPRemoteCommandCenter sharedCommandCenter];
    [commandCenter.togglePlayPauseCommand removeTarget:self];
    [commandCenter.playCommand removeTarget:self];
    [commandCenter.pauseCommand removeTarget:self];
    [commandCenter.nextTrackCommand removeTarget:self];
    [commandCenter.previousTrackCommand removeTarget:self];
    [commandCenter.changePlaybackPositionCommand removeTarget:self];
}

- (MPRemoteCommandHandlerStatus)togglePlayPauseCommand:(MPRemoteCommandEvent *)sender {
    ddb_playback_state_t state = deadbeef->get_output ()->state ();
    if (state == DDB_PLAYBACK_STATE_PLAYING) {
        deadbeef->sendmessage (DB_EV_PAUSE, 0, 0, 0);
    }
    else {
        deadbeef->sendmessage (DB_EV_PLAY_CURRENT, 0, 0, 0);
    }
    return MPRemoteCommandHandlerStatusSuccess;
}

- (MPRemoteCommandHandlerStatus)playCommand:(MPRemoteCommandEvent *)sender {
    deadbeef->sendmessage (DB_EV_PLAY_CURRENT, 0, 0, 0);
    return MPRemoteCommandHandlerStatusSuccess;
}

- (MPRemoteCommandHandlerStatus)pauseCommand:(MPRemoteCommandEvent *)sender {
    deadbeef->sendmessage (DB_EV_PAUSE, 0, 0, 0);
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

- (MPRemoteCommandHandlerStatus)changePlaybackPositionCommand:(MPChangePlaybackPositionCommandEvent *)sender {
    deadbeef->sendmessage (DB_EV_SEEK, 0, (uint32_t)(sender.positionTime * 1000), 0);
    return MPRemoteCommandHandlerStatusSuccess;
}

@end
