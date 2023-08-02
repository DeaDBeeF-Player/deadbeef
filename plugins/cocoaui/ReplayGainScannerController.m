/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2016 Oleksiy Yakovenko and other contributors

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
#include <sys/time.h>

extern DB_functions_t *deadbeef;

static void
_scan_progress (int current, void *user_data);

static char *_title_tf;
static ddb_rg_scanner_t *_rg;

static NSMutableArray *g_rgControllers;

@interface ReplayGainScannerController () {
    ddb_rg_scanner_settings_t _rg_settings;
    struct timeval _rg_start_tv;
}
@property (nonatomic) int abort_flag;
@property (nonatomic) BOOL abortTagWriting;

- (void)progress:(int)current;
@end

@implementation ReplayGainScannerController

+ (BOOL)initPlugin {
    if (_rg) {
        return YES;
    }

    _rg = (ddb_rg_scanner_t *)deadbeef->plug_get_for_id ("rg_scanner");

    if (!_rg) {
        deadbeef->log ("rg_scanner plugin is not found");
        return NO;
    }

    if (_rg && _rg->misc.plugin.version_major != 1) {
        _rg = NULL;
        deadbeef->log ("Invalid version of rg_scanner plugin");
        return NO;
    }

    return YES;
}

+ (ReplayGainScannerController *)runScanner:(int)mode forTracks:(DB_playItem_t **)tracks count:(int)count {
    deadbeef->background_job_increment ();

    ReplayGainScannerController *ctl = [[ReplayGainScannerController alloc] initWithWindowNibName:@"ReplayGain"];

    if (!_title_tf) {
        _title_tf = deadbeef->tf_compile ("%title%");
    }

    [ctl runScanner:mode forTracks:tracks count:count];
    if (!g_rgControllers) {
        g_rgControllers = [NSMutableArray new];
    }
    [g_rgControllers addObject:ctl];
    return ctl;
}

+ (ReplayGainScannerController *)removeRgTagsFromTracks:(DB_playItem_t **)tracks count:(int)count {
    deadbeef->background_job_increment ();

    ReplayGainScannerController *ctl = [[ReplayGainScannerController alloc] initWithWindowNibName:@"ReplayGain"];

    [ctl removeRgTagsFromTracks:tracks count:count];
    if (!g_rgControllers) {
        g_rgControllers = [NSMutableArray new];
    }
    [g_rgControllers addObject:ctl];
    return ctl;
}

+ (void)cleanup {
    g_rgControllers = nil;
}

- (void)dealloc {
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
}

- (void)dismissController:(id)sender {
    [self.window close];
    [super dismissController:sender];
    if (g_rgControllers) {
        [g_rgControllers removeObject:self];
    }
}

- (IBAction)progressCancelAction:(id)sender {
    [self.window endSheet:self.scanProgressWindow returnCode:NSModalResponseCancel];
}

- (void)windowWillClose:(NSNotification *)notification {
    if (g_rgControllers) {
        [g_rgControllers removeObject:self];
    }
}

- (void)runScanner:(int)mode forTracks:(DB_playItem_t **)tracks count:(int)count {
    if (![ReplayGainScannerController initPlugin]) {
        return;
    }

    self.window.delegate = self;
    self.window.isVisible = YES;
    [self.window makeKeyWindow];

    [self.window beginSheet:_scanProgressWindow completionHandler:^(NSModalResponse returnCode) {
        if (returnCode == NSModalResponseCancel) {
            self.abort_flag = 1;
        }
    }];

    memset (&_rg_settings, 0, sizeof (ddb_rg_scanner_settings_t));
    _rg_settings._size = sizeof (ddb_rg_scanner_settings_t);
    _rg_settings.mode = mode;
    _rg_settings.tracks = tracks;
    _rg_settings.num_tracks = count;
    _rg_settings.ref_loudness = deadbeef->conf_get_float ("rg_scanner.target_db", DDB_RG_SCAN_DEFAULT_LOUDNESS);
    _rg_settings.results = calloc (count, sizeof (ddb_rg_scanner_result_t));
    _rg_settings.pabort = &_abort_flag;
    _rg_settings.progress_callback = _scan_progress;
    _rg_settings.progress_cb_user_data = (__bridge void *)self;

    gettimeofday (&_rg_start_tv, NULL);
    [self progress:0];

    dispatch_queue_t aQueue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
    __weak ReplayGainScannerController *weakSelf = self;
    dispatch_async(aQueue, ^{
        ReplayGainScannerController *strongSelf = weakSelf;
        if (self == nil) {
            return;
        }
        _rg->scan (&self->_rg_settings);
        deadbeef->background_job_decrement ();
        dispatch_async(dispatch_get_main_queue(), ^{
            if (strongSelf.abort_flag) {
                [self dismissController:self];
                return;
            }
            [self scanFinished];
        });
    });
}

- (void)removeRgTagsFromTracks:(DB_playItem_t **)tracks count:(int)count {
    if (![ReplayGainScannerController initPlugin]) {
        return;
    }

    memset (&_rg_settings, 0, sizeof (ddb_rg_scanner_settings_t));
    _rg_settings._size = sizeof (ddb_rg_scanner_settings_t);
    _rg_settings.tracks = tracks;
    _rg_settings.num_tracks = count;

    (void)self.window; // access main window to make sure the NIB is loaded
    _updateTagsProgressWindow.isVisible = YES;
    _abortTagWriting = NO;

    __weak ReplayGainScannerController *weakSelf = self;
    dispatch_queue_t aQueue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
    dispatch_async(aQueue, ^{
        ReplayGainScannerController *strongSelf = weakSelf;
        if (strongSelf == nil) {
            return;
        }
        for (int i = 0; i < strongSelf->_rg_settings.num_tracks; i++) {
            _rg->remove (strongSelf->_rg_settings.tracks[i]);
            if (strongSelf.abortTagWriting) {
                break;
            }
            dispatch_async(dispatch_get_main_queue(), ^{
                // progress
                deadbeef->pl_lock ();
                NSString *path = @(deadbeef->pl_find_meta_raw (strongSelf->_rg_settings.tracks[i], ":URI"));
                deadbeef->pl_unlock ();
                strongSelf.updateTagsProgressText.stringValue = path;
                strongSelf.updateTagsProgressIndicator.doubleValue = (double)i/strongSelf->_rg_settings.num_tracks*100;
            });
        }
        deadbeef->pl_save_all ();
        deadbeef->background_job_decrement ();
        dispatch_async(dispatch_get_main_queue(), ^{
            [strongSelf.updateTagsProgressWindow close];
            [self dismissController:self];
        });
    });
}


- (NSString *)formatTime:(float)sec extraPrecise:(BOOL)extraPrecise {
    int hr;
    int min;
    hr = (int)floor (sec / 3600);
    sec -= hr * 3600;
    min = (int)floor (sec / 60);
    sec -= min * 60;

    if (extraPrecise) {
        if (hr > 0) {
            return [NSString stringWithFormat:@"%d:%02d:%0.3f", hr, min, sec];
        }
        return [NSString stringWithFormat:@"%02d:%0.3f", min, sec];
    }
    if (hr > 0) {
        return [NSString stringWithFormat:@"%d:%02d:%02d", hr, min, (int)floor(sec)];
    }
    return [NSString stringWithFormat:@"%02d:%02d", min, (int)floor(sec)];
}

- (float)getScanSpeed:(uint64_t)cd_samples_processed overTime:(float)time {
    return cd_samples_processed / 44100.f / time;
}

- (void)progress:(int)current {
    deadbeef->pl_lock ();
    const char *uri = deadbeef->pl_find_meta (_rg_settings.tracks[current], ":URI");

    _progressText.stringValue = @(uri);
    _progressIndicator.doubleValue = (double)current/_rg_settings.num_tracks*100;

    struct timeval tv;
    gettimeofday (&tv, NULL);
    float timePassed = (tv.tv_sec-_rg_start_tv.tv_sec) + (tv.tv_usec - _rg_start_tv.tv_usec) / 1000000.f;
    if (timePassed > 0 && _rg_settings.cd_samples_processed > 0 && current > 0) {
        float speed = [self getScanSpeed:_rg_settings.cd_samples_processed overTime:timePassed];
        float predicted_samples_total = _rg_settings.cd_samples_processed / (float)current * _rg_settings.num_tracks;

        float frac = (float)((double)predicted_samples_total / _rg_settings.cd_samples_processed);
        float est = timePassed * frac;

        NSString *elapsed = [self formatTime:timePassed extraPrecise:NO];
        NSString *estimated = [self formatTime:est extraPrecise:NO];

        _statusLabel.stringValue = [NSString stringWithFormat:@"Time elapsed: %@, estimated: %@, speed: %0.2fx (%i of %i files)", elapsed, estimated, speed, current, _rg_settings.num_tracks];
    }
    else {
        _statusLabel.stringValue = @"";
    }

    deadbeef->pl_unlock ();
}

- (void)scanFinished {
    struct timeval tv;
    gettimeofday (&tv, NULL);
    float timePassed = (tv.tv_sec-_rg_start_tv.tv_sec) + (tv.tv_usec - _rg_start_tv.tv_usec) / 1000000.f;
    NSString *elapsed = [self formatTime:timePassed extraPrecise:YES];
    float speed = [self getScanSpeed:_rg_settings.cd_samples_processed overTime:timePassed];
    _resultStatusLabel.stringValue = [NSString stringWithFormat:@"Calculated in: %@, speed: %0.2fx", elapsed, speed];

    [self.window endSheet:_scanProgressWindow returnCode:NSModalResponseOK];
    _resultsTableView.dataSource = self;
    [_resultsTableView reloadData];
}

- (IBAction)updateFileTagsAction:(id)sender {
    [self.window beginSheet:_updateTagsProgressWindow completionHandler:^(NSModalResponse returnCode) {
    }];

    _abortTagWriting = NO;
    dispatch_queue_t aQueue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);

    __weak ReplayGainScannerController *weakSelf = self;

    dispatch_async(aQueue, ^{
        ReplayGainScannerController *strongSelf = weakSelf;
        if (strongSelf == nil) {
            return;
        }
        for (int i = 0; i < strongSelf->_rg_settings.num_tracks; i++) {
            if (strongSelf.abortTagWriting) {
                break;
            }

            if (strongSelf->_rg_settings.results[i].scan_result == DDB_RG_SCAN_RESULT_SUCCESS) {
                deadbeef->pl_lock ();
                NSString *path = @(deadbeef->pl_find_meta_raw (strongSelf->_rg_settings.tracks[i], ":URI"));
                deadbeef->pl_unlock ();
                dispatch_async(dispatch_get_main_queue(), ^{
                    strongSelf.updateTagsProgressText.stringValue = path;
                    strongSelf.updateTagsProgressIndicator.doubleValue = (double)i/strongSelf->_rg_settings.num_tracks*100;
                });

                uint32_t flags = (1<<DDB_REPLAYGAIN_TRACKGAIN)|(1<<DDB_REPLAYGAIN_TRACKPEAK);
                if (strongSelf->_rg_settings.mode != DDB_RG_SCAN_MODE_TRACK) {
                    flags |= (1<<DDB_REPLAYGAIN_ALBUMGAIN)|(1<<DDB_REPLAYGAIN_ALBUMPEAK);
                }
                _rg->apply (strongSelf->_rg_settings.tracks[i], flags, strongSelf->_rg_settings.results[i].track_gain, strongSelf->_rg_settings.results[i].track_peak, strongSelf->_rg_settings.results[i].album_gain, strongSelf->_rg_settings.results[i].album_peak);
            }
        }
        deadbeef->pl_save_all ();

        dispatch_async(dispatch_get_main_queue(), ^{
            [self.window endSheet:strongSelf.updateTagsProgressWindow returnCode:NSModalResponseOK];
            [self dismissController:self];
        });
    });
}

- (IBAction)resultsCancelAction:(id)sender {
    [self dismissController:self];
}

- (IBAction)updateTagsCancelAction:(id)sender {
    _abortTagWriting = YES;
}

// NSTableViewDataSource
- (NSInteger)numberOfRowsInTableView:(NSTableView *)aTableView
{
    return _rg_settings.num_tracks;
}

- (id)tableView:(NSTableView *)aTableView objectValueForTableColumn:(NSTableColumn *)aTableColumn row:(NSInteger)rowIndex {
    DB_playItem_t *it = _rg_settings.tracks[rowIndex];
    NSUInteger colIdx = [aTableView.tableColumns indexOfObject:aTableColumn];

    NSString *status_str[] = {
        @"Success",
        @"File not found",
        @"Invalid file",
    };

    switch (colIdx) {
    case 0: {
            ddb_tf_context_t ctx;
            memset (&ctx, 0, sizeof(ctx));
            ctx._size = sizeof (ddb_tf_context_t);
            ctx.it = it;

            char buffer[100];
            deadbeef->tf_eval (&ctx, _title_tf, buffer, sizeof (buffer));
            return @(buffer);
        }
        break;
    case 1:
        return -_rg_settings.results[rowIndex].scan_result < 3 ? status_str[-_rg_settings.results[rowIndex].scan_result] : @"Unknown error";
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
