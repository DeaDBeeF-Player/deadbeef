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
@property (nonatomic,readonly) ddb_mediasource_source_t medialibSource;
@property (nonatomic) NSMutableArray<NSString *> *folders;
@property (nonatomic) BOOL enabled;

@end

@implementation MediaLibraryPreferencesViewController

- (ddb_mediasource_source_t)medialibSource {
    AppDelegate *appDelegate = NSApplication.sharedApplication.delegate;
    return appDelegate.mediaLibraryManager.source;
}

- (void)initializeList {
    [self.folders removeAllObjects];
    NSInteger count = (NSInteger)self.medialibPlugin->folder_count(self.medialibSource);
    for (NSInteger i = 0; i < count; i++) {
        char folder[PATH_MAX];
        self.medialibPlugin->folder_at_index(self.medialibSource, (int)i, folder, sizeof (folder));

        [self.folders addObject:[NSString stringWithUTF8String:folder]];
    }
}

- (void)viewDidLoad {
    [super viewDidLoad];

    self.medialibPlugin = (ddb_medialib_plugin_t *)deadbeef->plug_get_for_id ("medialib");

    self.folders = [NSMutableArray new];
    self.enabled = self.medialibPlugin->plugin.get_source_enabled(self.medialibSource);

    [self initializeList];
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
    __block NSInteger index = 0;
    if (self.tableView.selectedRowIndexes.count != 0) {
        index = self.tableView.selectedRowIndexes.firstIndex;
    }

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
                [self.tableView insertRowsAtIndexes:[NSIndexSet indexSetWithIndex:index] withAnimation:NSTableViewAnimationEffectFade];
                [self.folders insertObject:url.path atIndex:index];
                index++;
            }
            [self.tableView endUpdates];
            [self applyChanges];
        }
    }];

}

- (void)removeAction:(id)sender {
    if (self.tableView.selectedRowIndexes.count == 0) {
        return;
    }
    NSInteger index = self.tableView.selectedRowIndexes.firstIndex;
    [self.tableView removeRowsAtIndexes:self.tableView.selectedRowIndexes withAnimation:NSTableViewAnimationEffectFade];
    [self.folders removeObjectAtIndex:index];

    [self applyChanges];
}

- (void)applyChanges {
    const char **paths = self.folders.count ? calloc (sizeof (char *), self.folders.count) : NULL;
    for (NSUInteger i = 0; i < self.folders.count; i++) {
        paths[i] = self.folders[i].UTF8String;
    }

    self.medialibPlugin->set_folders (self.medialibSource, paths, self.folders.count);
    self.medialibPlugin->plugin.refresh (self.medialibSource);

    free (paths);
}

#pragma mark NSTableViewDataSource

- (NSInteger)numberOfRowsInTableView:(NSTableView *)tableView {
    return self.folders.count;
}

#pragma mark - NSTableViewDelegate

- (nullable NSView *)tableView:(NSTableView *)tableView viewForTableColumn:(nullable NSTableColumn *)tableColumn row:(NSInteger)row {
    NSTableCellView *view = [tableView makeViewWithIdentifier:tableColumn.identifier owner:self];
    view.textField.stringValue = self.folders[row];
    return view;
}

@end
