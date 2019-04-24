//
//  ScriptableNodeEditorViewController.m
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 4/24/19.
//  Copyright © 2019 Alexey Yakovenko. All rights reserved.
//

#import "ScriptableNodeEditorViewController.h"
#import "PropertySheetViewController.h"
#import "ScriptableTableDataSource.h"
#import "ScriptablePropertySheetDataSource.h"
#include "deadbeef.h"

extern DB_functions_t *deadbeef;

@interface ScriptableNodeEditorViewController () <ScriptableItemDelegate,NSMenuDelegate>
@property (unsafe_unretained) IBOutlet NSTableView *dspList;
// dsp properties
@property (strong) IBOutlet NSPanel *dspConfigPanel;
@property (strong) IBOutlet PropertySheetViewController *dspConfigViewController;
@property ScriptableTableDataSource *dspChainDataSource;
@property ScriptablePropertySheetDataSource *dspPropertySheetDataSource;


- (IBAction)dspChainAction:(id)sender;

@end

@implementation ScriptableNodeEditorViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do view setup here.

    scriptableItem_t *chain = scriptableDspConfigFromDspChain (deadbeef->streamer_get_dsp_chain ());
    _dspChainDataSource = [[ScriptableTableDataSource alloc] initWithScriptable:chain pasteboardItemIdentifier:@"deadbeef.dspnode.preferences"];
    _dspChainDataSource.delegate = self;
    _dspList.dataSource = _dspChainDataSource;
    [_dspList registerForDraggedTypes: [NSArray arrayWithObjects: _dspChainDataSource.pasteboardItemIdentifier, nil]];
}

#pragma mark - ScriptableItemDelegate

- (void)scriptableItemChanged:(scriptableItem_t *)scriptable {
    if (scriptable == _dspChainDataSource.scriptable
        || scriptableItemIndexOfChild(_dspChainDataSource.scriptable, scriptable) >= 0) {
        ddb_dsp_context_t *chain = scriptableDspConfigToDspChain (_dspChainDataSource.scriptable);
        deadbeef->streamer_set_dsp_chain (chain);
        deadbeef->dsp_preset_free (chain);
    }
}


// FIXME: needs to query preset manager for the list of names / ids
- (NSMenu *)getDSPMenu {
    NSMenu *menu = [[NSMenu alloc] initWithTitle:@"DspChainMenu"];
    menu.delegate = self;
    [menu setAutoenablesItems:NO];

    DB_dsp_t **plugins = deadbeef->plug_get_dsp_list ();

    for (int i = 0; plugins[i]; i++) {
        NSMenuItem *item = [[NSMenuItem alloc] initWithTitle:[NSString stringWithUTF8String:plugins[i]->plugin.name] action:@selector(addDspNode:) keyEquivalent:@""];
        item.tag = i;
        [menu addItem:item];
    }

    return menu;
}

- (void)addDspNode:(id)sender {
    NSMenuItem *item = sender;

    DB_dsp_t **plugins = deadbeef->plug_get_dsp_list ();

    scriptableItem_t *node = scriptableItemCreateItemOfType(scriptableDspRoot (), plugins[item.tag]->plugin.id);

    id<NSTableViewDataSource> ds = _dspChainDataSource;
    NSInteger cnt = [ds numberOfRowsInTableView:_dspList];
    NSInteger index = [_dspList selectedRow];
    if (index < 0) {
        index = cnt;
    }

    NSIndexSet *is = [NSIndexSet indexSetWithIndex:index];
    [_dspList beginUpdates];
    [_dspList insertRowsAtIndexes:is withAnimation:NSTableViewAnimationSlideDown];
    [_dspChainDataSource insertItem:node atIndex:index];
    [_dspList endUpdates];
    [_dspList selectRowIndexes:is byExtendingSelection:NO];
}

- (IBAction)dspAddAction:(id)sender {
    NSMenu *menu = [self getDSPMenu];
    [NSMenu popUpContextMenu:menu withEvent:[NSApp currentEvent] forView:sender];
}

- (IBAction)dspRemoveAction:(id)sender {
    NSInteger index = [_dspList selectedRow];
    if (index < 0) {
        return;
    }

    [_dspList beginUpdates];
    NSIndexSet *is = [NSIndexSet indexSetWithIndex:index];
    [_dspList removeRowsAtIndexes:is withAnimation:NSTableViewAnimationSlideUp];
    [_dspList endUpdates];
    [_dspChainDataSource removeItemAtIndex:(int)index];

    if (index >= [_dspList numberOfRows]) {
        index--;
    }
    if (index >= 0) {
        [_dspList selectRowIndexes:[NSIndexSet indexSetWithIndex:index] byExtendingSelection:NO];
    }
}

- (IBAction)dspConfigureAction:(id)sender {
    NSInteger index = [_dspList selectedRow];
    if (index < 0) {
        return;
    }
    scriptableItem_t *item = scriptableItemChildAtIndex(_dspChainDataSource.scriptable, (unsigned int)index);
    self.dspPropertySheetDataSource = [[ScriptablePropertySheetDataSource alloc] initWithScriptable:item];
    self.dspPropertySheetDataSource.delegate = self;

    _dspConfigViewController.dataSource = self.dspPropertySheetDataSource;
    [NSApp beginSheet:_dspConfigPanel modalForWindow:self.view.window modalDelegate:self didEndSelector:@selector(didEndDspConfigPanel:returnCode:contextInfo:) contextInfo:nil];
}

- (void)didEndDspConfigPanel:(NSWindow *)sheet returnCode:(NSInteger)returnCode contextInfo:(void *)contextInfo {
    [_dspConfigPanel orderOut:self];
}

- (IBAction)dspConfigCancelAction:(id)sender {
    [NSApp endSheet:_dspConfigPanel returnCode:NSCancelButton];
}

- (IBAction)dspConfigOkAction:(id)sender {
    [NSApp endSheet:_dspConfigPanel returnCode:NSOKButton];
}

- (IBAction)dspConfigResetAction:(id)sender {
    [_dspConfigViewController reset];
}

- (IBAction)dspChainAction:(id)sender {
    NSInteger selectedSegment = [sender selectedSegment];

    switch (selectedSegment) {
        case 0:
            [self dspAddAction:sender];
            break;
        case 1:
            [self dspRemoveAction:sender];
            break;
        case 2:
            [self dspConfigureAction:sender];
            break;
    }
}

- (IBAction)dspSaveAction:(id)sender {
}

- (IBAction)dspLoadAction:(id)sender {
}

@end
