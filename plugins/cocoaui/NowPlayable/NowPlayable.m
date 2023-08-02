//
//  NowPlayable.m
//  DeaDBeeF
//
//  Created by Oleksiy Yakovenko on 11/4/19.
//  Copyright © 2019 Oleksiy Yakovenko. All rights reserved.
//

#import "NowPlayable.h"
#import <MediaPlayer/MediaPlayer.h>
#include <deadbeef/deadbeef.h>

extern DB_functions_t *deadbeef;

@interface NowPlayable()

@property (nonatomic) NSTimer *updateTimer;
@property (nonatomic) char *artist_tf;
@property (nonatomic) char *album_tf;

@end

@implementation NowPlayable

- (instancetype)init
{
    self = [super init];
    if (!self) {
        return nil;
    }

    _artist_tf = deadbeef->tf_compile ("$if2(%artist%,Unknown Artist)");
    _album_tf = deadbeef->tf_compile ("$if2(%album%,Unknown Album)");


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
        NowPlayable *nowPlayable = weakSelf;
        if (!nowPlayable) {
            return;
        }

        NSMutableDictionary *info = [NSMutableDictionary new];
        info[MPNowPlayingInfoPropertyPlaybackRate] = @1.0;

        DB_playItem_t *it = deadbeef->streamer_get_playing_track_safe ();

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

            // song info
            char text[1000];
            deadbeef->pl_get_meta (it, "title", text, sizeof (text));
            info[MPMediaItemPropertyTitle] = @(text);

            ddb_tf_context_t ctx = {
                ._size = sizeof (ddb_tf_context_t),
                .flags = DDB_TF_CONTEXT_NO_DYNAMIC|DDB_TF_CONTEXT_MULTILINE,
                .it = it
            };

            deadbeef->tf_eval (&ctx, nowPlayable.artist_tf, text, sizeof (text));
            info[MPMediaItemPropertyArtist] = @(text);
            deadbeef->tf_eval (&ctx, nowPlayable.album_tf, text, sizeof (text));
            info[MPMediaItemPropertyAlbumTitle] = @(text);

            deadbeef->pl_item_unref (it);
        }

        info[MPNowPlayingInfoPropertyMediaType] = @(MPNowPlayingInfoMediaTypeAudio);

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

    deadbeef->tf_free (_artist_tf);
    deadbeef->tf_free (_album_tf);
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
