/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2016 Alexey Yakovenko and other contributors

    This software is provided 'as-is', without any express or implied
    warranty.  In no event will the authors be held liable for any damages
    arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.

    3. This notice may not be removed or altered from any source distribution.
*/

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
    int _abort_flag;
}

- (void)windowDidLoad {
    [super windowDidLoad];
}

- (void)dismissController:(id)sender {
    if (_rg_settings.tracks) {
        for (int i = 0; i < _rg_settings.num_tracks; i++) {
            deadbeef->pl_item_unref (_rg_settings.tracks[i]);
        }
        free (_rg_settings.tracks);
    }
    if (_rg_settings.results) {
        free (_rg_settings.results);
    }
    memset (&_rg_settings, 0, sizeof (_rg_settings));
    
    if (g_rgControllers) {
        [g_rgControllers removeObject:self];
    }
}

- (IBAction)progressCancelAction:(id)sender {
    _abort_flag = 1;
}

+ (ReplayGainScannerController *)runScanner:(int)mode forTracks:(DB_playItem_t **)tracks count:(int)count {
    deadbeef->background_job_increment ();

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
    [[self window] setIsVisible:YES];
    [[self window] makeKeyWindow];

    memset (&_rg_settings, 0, sizeof (ddb_rg_scanner_settings_t));
    _rg_settings._size = sizeof (ddb_rg_scanner_settings_t);
    _rg_settings.mode = mode;
    _rg_settings.tracks = tracks;
    _rg_settings.num_tracks = count;
    _rg_settings.targetdb = deadbeef->conf_get_float ("rg_scanner.target_db", DDB_RG_SCAN_DEFAULT_LOUDNESS);
    _rg_settings.results = calloc (count, sizeof (ddb_rg_scanner_result_t));
    _rg_settings.pabort = &_abort_flag;
    _rg_settings.progress_callback = _scan_progress;
    _rg_settings.progress_cb_user_data = (__bridge void *)self;

    dispatch_queue_t aQueue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
    dispatch_async(aQueue, ^{
        _rg->scan (&_rg_settings);
        deadbeef->background_job_decrement ();
        dispatch_async(dispatch_get_main_queue(), ^{
            if (_abort_flag) {
                [self dismissController:self];
                return;
            }
            [self scanFinished];
        });
    });
}

- (void)progress:(int)current {
    deadbeef->pl_lock ();
    const char *uri = deadbeef->pl_find_meta (_rg_settings.tracks[current], ":URI");

    [_progressText setStringValue:[NSString stringWithUTF8String:uri]];
    [_progressIndicator setDoubleValue:(double)current/_rg_settings.num_tracks*100];
    deadbeef->pl_unlock ();
}

- (void)scanFinished {
    [[self window] setIsVisible:NO];
    [_resultsWindow setIsVisible:YES];
    [_resultsWindow makeKeyWindow];
    [_resultsTableView setDataSource:(id<NSTableViewDataSource>)self];
    [_resultsTableView reloadData];
}

- (IBAction)updateFileTagsAction:(id)sender {
}

- (IBAction)resultsCancelAction:(id)sender {
}

// NSTableViewDataSource
- (int)numberOfRowsInTableView:(NSTableView *)aTableView
{
    return _rg_settings.num_tracks;
}

- (id)tableView:(NSTableView *)aTableView objectValueForTableColumn:(NSTableColumn *)aTableColumn row:(int)rowIndex {
    DB_playItem_t *it = _rg_settings.tracks[rowIndex];
    NSUInteger colIdx = [[aTableView tableColumns] indexOfObject:aTableColumn];

    NSString *status_str[] = {
        @"Success",
        @"File not found",
    };

    switch (colIdx) {
    case 0: {
            ddb_tf_context_t ctx;
            memset (&ctx, 0, sizeof(ctx));
            ctx._size = sizeof (ddb_tf_context_t);
            ctx.it = it;

            char buffer[100];
            deadbeef->tf_eval (&ctx, _title_tf, buffer, sizeof (buffer));
            return [NSString stringWithUTF8String:buffer];
        }
        break;
    case 1:
        return _rg_settings.results[rowIndex].scan_result < 2 ? status_str[_rg_settings.results[rowIndex].scan_result] : @"Unknown error";
    case 2:
        return _rg_settings.mode == DDB_RG_SCAN_MODE_TRACK ? @"" : [NSString stringWithFormat:@"%0.2f dB", _rg_settings.results[rowIndex].album_gain];
    case 3:
        return [NSString stringWithFormat:@"%0.2f dB", _rg_settings.results[rowIndex].track_gain];
    case 4:
        return _rg_settings.mode == DDB_RG_SCAN_MODE_TRACK ? @"" : [NSString stringWithFormat:@"%0.6f", _rg_settings.results[rowIndex].album_peak];
    case 5:
        return [NSString stringWithFormat:@"%0.6f", _rg_settings.results[rowIndex].track_peak];
    }

    return @"Placeholder";
}


@end

static void
_scan_progress (int current, void *user_data) {
    dispatch_async(dispatch_get_main_queue(), ^{
        ReplayGainScannerController *ctl = (__bridge ReplayGainScannerController *)user_data;
        [ctl progress:current];
    });
}
