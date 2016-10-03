//
//  ReplayGainScannerController.m
//  deadbeef
//
//  Created by Oleksiy Yakovenko on 01/10/16.
//  Copyright © 2016 Alexey Yakovenko. All rights reserved.
//

#import "ReplayGainScannerController.h"
#include "rg_scanner.h"

extern DB_functions_t *deadbeef;

static void
_scan_progress (int current, void *user_data);

static char *_title_tf;

static NSMutableArray *g_rgControllers;

@interface ReplayGainScannerController ()
- (void)progress:(int)current;
@end

@implementation ReplayGainScannerController {
    ddb_rg_scanner_settings_t _rg_settings;
    ddb_rg_scanner_t *_rg;
}

- (void)windowDidLoad {
    [super windowDidLoad];
}

- (IBAction)progressCancelAction:(id)sender {
}

+ (ReplayGainScannerController *)runScanner:(int)mode forTracks:(DB_playItem_t **)tracks count:(int)count {
    ReplayGainScannerController *ctl = [[ReplayGainScannerController alloc] initWithWindowNibName:@"ReplayGain"];

    if (!_title_tf) {
        _title_tf = deadbeef->tf_compile ("%title%");
    }

    [ctl runScanner:mode forTracks:tracks count:count];
    if (!g_rgControllers) {
        g_rgControllers = [[NSMutableArray alloc] init];
    }
    [g_rgControllers addObject:ctl];
    return ctl;
}

- (void)runScanner:(int)mode forTracks:(DB_playItem_t **)tracks count:(int)count {
    _rg = (ddb_rg_scanner_t *)deadbeef->plug_get_for_id ("rg_scanner");
    if (_rg && _rg->misc.plugin.version_major != 1) {
        _rg = NULL;
        deadbeef->log ("Invalid version of rg_scanner plugin");
        return;
    }

    if (!_rg) {
        deadbeef->log ("ReplayGain plugin is not found");
    }

    //    [[_rgScannerWindowController window] setIsVisible:YES];
    //    [[_rgScannerWindowController window] makeKeyWindow];
    [_progressPanel setIsVisible:YES];
    [_progressPanel makeKeyWindow];

    memset (&_rg_settings, 0, sizeof (ddb_replaygain_settings_t));
    _rg_settings._size = sizeof (ddb_replaygain_settings_t);
    _rg_settings.mode = mode;
    _rg_settings.tracks = tracks;
    _rg_settings.num_tracks = count;
    _rg_settings.out_track_peak = calloc (count, sizeof (float));
    _rg_settings.out_track_gain = calloc (count, sizeof (float));
    _rg_settings.progress_callback = _scan_progress;
    _rg_settings.progress_cb_user_data = (__bridge void *)self;

    dispatch_queue_t aQueue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
    dispatch_async(aQueue, ^{
        _rg->scan (&_rg_settings);
        dispatch_async(dispatch_get_main_queue(), ^{
            [self scanFinished];
        });
    });
}

- (void)progress:(int)current {
    deadbeef->pl_lock ();
    ddb_tf_context_t ctx;
    memset (&ctx, 0, sizeof(ctx));
    ctx._size = sizeof (ddb_tf_context_t);
    ctx.it = _rg_settings.tracks[current];

    char buffer[100];
    deadbeef->tf_eval (&ctx, _title_tf, buffer, sizeof (buffer));

    [_progressText setStringValue:[NSString stringWithUTF8String:buffer]];
    deadbeef->pl_unlock ();
}

- (void)scanFinished {
    _rg->clear_settings (&_rg_settings);
    if (g_rgControllers) {
        [g_rgControllers removeObject:self];
    }
}
@end

static void
_scan_progress (int current, void *user_data) {
    dispatch_async(dispatch_get_main_queue(), ^{
        ReplayGainScannerController *ctl = (__bridge ReplayGainScannerController *)user_data;
        [ctl progress:current];
    });
}
