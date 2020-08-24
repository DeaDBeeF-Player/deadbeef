//
//  MediaLibraryPreferencesViewController.m
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 8/24/20.
//  Copyright © 2020 Alexey Yakovenko. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "MediaLibraryPreferencesViewController.h"
#include "deadbeef.h"
#include "medialib.h"

extern DB_functions_t *deadbeef;

@interface MediaLibraryPreferencesViewController () <NSTableViewDelegate,NSTableViewDataSource>

@property (nonatomic) ddb_medialib_plugin_t *medialibPlugin;
@property (weak) IBOutlet NSTableView *tableView;
@property (nonatomic) NSMutableArray<NSString *> *folders;
@end

@implementation MediaLibraryPreferencesViewController

- (void)viewDidLoad {
    [super viewDidLoad];

    self.medialibPlugin = (ddb_medialib_plugin_t *)deadbeef->plug_get_for_id ("medialib");


    self.folders = [NSMutableArray new];

    NSInteger count = (NSInteger)self.medialibPlugin->folder_count();
    for (NSInteger i = 0; i < count; i++) {
        char folder[PATH_MAX];
        self.medialibPlugin->folder_for_index((int)i, folder, sizeof (folder));

        [self.folders addObject:[NSString stringWithUTF8String:folder]];
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
}

- (void)removeAction:(id)sender {
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
