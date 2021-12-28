//
//  MediaLibraryPreferencesViewController.m
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 8/24/20.
//  Copyright © 2020 Alexey Yakovenko. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "AppDelegate.h"
#import "MediaLibraryPreferencesViewController.h"
#include "deadbeef.h"
#include "medialib.h"

static NSString * const kMedialibBrowseInitialFolder = @"MedialibBrowseInitialFolder";

extern DB_functions_t *deadbeef;

@interface MediaLibraryPreferencesViewController () <NSTableViewDelegate,NSTableViewDataSource>

@property (weak) IBOutlet NSTableView *tableView;

@property (nonatomic) ddb_medialib_plugin_t *medialibPlugin;
@property (nonatomic) ddb_mediasource_source_t medialibSource;
@property (nonatomic) BOOL enabled;

@property (nonatomic) int listenerId;

@end

@implementation MediaLibraryPreferencesViewController

- (void)dealloc
{
    _medialibPlugin->plugin.remove_listener (_medialibSource, _listenerId);
    _listenerId = 0;
}

static void
_listener (ddb_mediasource_event_type_t _event, void *user_data) {
    MediaLibraryPreferencesViewController *self = (__bridge MediaLibraryPreferencesViewController *)(user_data);

    if (_event < DDB_MEDIASOURCE_EVENT_MAX) {
        switch (_event) {
        case DDB_MEDIASOURCE_EVENT_ENABLED_DID_CHANGE:
            {
                dispatch_async(dispatch_get_main_queue(), ^{
                    self.enabled = self.medialibPlugin->plugin.is_source_enabled(self.medialibSource);
                });
            }
            break;
        default:
            break;
        }
        return;
    }

    ddb_medialib_mediasource_event_type_t event = (ddb_medialib_mediasource_event_type_t)_event;

    switch (event) {
    case DDB_MEDIALIB_MEDIASOURCE_EVENT_FOLDERS_DID_CHANGE:
        dispatch_async(dispatch_get_main_queue(), ^{
            [self.tableView reloadData];
        });
        break;
    }
}

- (void)viewDidLoad {
    [super viewDidLoad];

    self.medialibPlugin = (ddb_medialib_plugin_t *)deadbeef->plug_get_for_id ("medialib");
    if (self.medialibPlugin == nil) {
        return;
    }

    AppDelegate *appDelegate = NSApplication.sharedApplication.delegate;
    self.medialibSource = appDelegate.mediaLibraryManager.source;

    _listenerId = self.medialibPlugin->plugin.add_listener(self.medialibSource, _listener, (__bridge void *)(self));

    _enabled = self.medialibPlugin->plugin.is_source_enabled(self.medialibSource);
    [self willChangeValueForKey:@"enabled"];
    [self didChangeValueForKey:@"enabled"];
}

- (BOOL)isAvailable {
    return self.medialibPlugin != nil;
}

- (void)setEnabled:(BOOL)enabled {
    if (enabled != _enabled) {
        _enabled = enabled;
        self.medialibPlugin->plugin.set_source_enabled(self.medialibSource, _enabled);
        self.medialibPlugin->plugin.refresh (self.medialibSource);
    }
}

- (IBAction)addRemoveAction:(NSSegmentedControl *)sender {
    NSInteger selectedSegment = [sender selectedSegment];

    switch (selectedSegment) {
    case 0:
        [self addAction:sender];
        break;
    case 1:
        [self removeAction:sender];
        break;
    }
}

- (void)addAction:(id)sender {
    NSOpenPanel *panel = [NSOpenPanel openPanel];
    panel.canChooseDirectories = YES;
    panel.canChooseFiles = NO;
    panel.allowsMultipleSelection = YES;
    panel.canCreateDirectories = YES;
    panel.message = @"Select music folders";
    NSString *initialPath = [NSUserDefaults.standardUserDefaults stringForKey:kMedialibBrowseInitialFolder];
    if (initialPath) {
        panel.directoryURL = [NSURL URLWithString:initialPath];
    }

    // Display the panel attached to the document's window.
    [panel beginSheetModalForWindow:self.view.window completionHandler:^(NSInteger result){
        [NSUserDefaults.standardUserDefaults setObject:panel.directoryURL.absoluteString forKey:kMedialibBrowseInitialFolder];
        if (result == NSModalResponseOK) {
            [self.tableView beginUpdates];
            for (NSURL *url in panel.URLs) {
                self.medialibPlugin->append_folder (self.medialibSource, url.path.UTF8String);
                self.medialibPlugin->plugin.refresh (self.medialibSource);
            }
            [self.tableView endUpdates];
        }
    }];

}

- (void)removeAction:(id)sender {
    NSInteger index = self.tableView.selectedRowIndexes.firstIndex;
    self.medialibPlugin->remove_folder_at_index(self.medialibSource, (int)index);
    self.medialibPlugin->plugin.refresh (self.medialibSource);
}

#pragma mark NSTableViewDataSource

- (NSInteger)numberOfRowsInTableView:(NSTableView *)tableView {
    // This may get called even if medialib is unavaliable, because the viewcontroller is instantiated anyway.
    // For example, when changing macOS color settings.
    if (self.medialibPlugin == NULL) {
        return 0;
    }
    return self.medialibPlugin->folder_count(self.medialibSource);
}

#pragma mark - NSTableViewDelegate

- (nullable NSView *)tableView:(NSTableView *)tableView viewForTableColumn:(nullable NSTableColumn *)tableColumn row:(NSInteger)row {
    NSTableCellView *view = [tableView makeViewWithIdentifier:tableColumn.identifier owner:self];
    char folder[PATH_MAX];
    self.medialibPlugin->folder_at_index(self.medialibSource, (int)row, folder, sizeof (folder));
    view.textField.stringValue = [NSString stringWithUTF8String:folder];
    return view;
}

@end
