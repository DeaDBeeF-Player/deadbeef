//
//  ScriptableNodeEditorViewController.m
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 4/24/19.
//  Copyright © 2019 Alexey Yakovenko. All rights reserved.
//

#import "ScriptableNodeEditorViewController.h"
#import "PropertySheetViewController.h"
#import "ScriptablePropertySheetDataSource.h"
#include "deadbeef.h"

extern DB_functions_t *deadbeef;

@interface ScriptableNodeEditorViewController () <ScriptableItemDelegate,NSMenuDelegate>

@property (unsafe_unretained) IBOutlet NSTableView *nodeList;
@property (strong) IBOutlet NSPanel *propertiesPanel;
@property (strong) IBOutlet PropertySheetViewController *propertiesViewController;
@property ScriptablePropertySheetDataSource *propertiesDataSource;

@end

@implementation ScriptableNodeEditorViewController

- (void)viewDidLoad {
    [super viewDidLoad];

    // Do view setup here.
    self.dataSource.delegate = self;
    _nodeList.dataSource = self.dataSource;
    [_nodeList registerForDraggedTypes: [NSArray arrayWithObjects: _dataSource.pasteboardItemIdentifier, nil]];
}

#pragma mark - ScriptableItemDelegate

- (void)scriptableItemChanged:(scriptableItem_t *)scriptable {
    if (scriptable == _dataSource.scriptable
        || scriptableItemIndexOfChild(_dataSource.scriptable, scriptable) >= 0) {
        ddb_dsp_context_t *chain = scriptableDspConfigToDspChain (_dataSource.scriptable);
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

    id<NSTableViewDataSource> ds = _dataSource;
    NSInteger cnt = [ds numberOfRowsInTableView:_nodeList];
    NSInteger index = [_nodeList selectedRow];
    if (index < 0) {
        index = cnt;
    }

    NSIndexSet *is = [NSIndexSet indexSetWithIndex:index];
    [_nodeList beginUpdates];
    [_nodeList insertRowsAtIndexes:is withAnimation:NSTableViewAnimationSlideDown];
    [_dataSource insertItem:node atIndex:index];
    [_nodeList endUpdates];
    [_nodeList selectRowIndexes:is byExtendingSelection:NO];
}

- (IBAction)dspAddAction:(id)sender {
    NSMenu *menu = [self getDSPMenu];
    [NSMenu popUpContextMenu:menu withEvent:[NSApp currentEvent] forView:sender];
}

- (IBAction)dspRemoveAction:(id)sender {
    NSInteger index = [_nodeList selectedRow];
    if (index < 0) {
        return;
    }

    [_nodeList beginUpdates];
    NSIndexSet *is = [NSIndexSet indexSetWithIndex:index];
    [_nodeList removeRowsAtIndexes:is withAnimation:NSTableViewAnimationSlideUp];
    [_nodeList endUpdates];
    [_dataSource removeItemAtIndex:(int)index];

    if (index >= [_nodeList numberOfRows]) {
        index--;
    }
    if (index >= 0) {
        [_nodeList selectRowIndexes:[NSIndexSet indexSetWithIndex:index] byExtendingSelection:NO];
    }
}

- (IBAction)dspConfigureAction:(id)sender {
    NSInteger index = [_nodeList selectedRow];
    if (index < 0) {
        return;
    }
    scriptableItem_t *item = scriptableItemChildAtIndex(_dataSource.scriptable, (unsigned int)index);
    self.propertiesDataSource = [[ScriptablePropertySheetDataSource alloc] initWithScriptable:item];
    self.propertiesDataSource.delegate = self;

    _propertiesViewController.dataSource = self.propertiesDataSource;
    [NSApp beginSheet:_propertiesPanel modalForWindow:self.view.window modalDelegate:self didEndSelector:@selector(didEndDspConfigPanel:returnCode:contextInfo:) contextInfo:nil];
}

- (void)didEndDspConfigPanel:(NSWindow *)sheet returnCode:(NSInteger)returnCode contextInfo:(void *)contextInfo {
    [_propertiesPanel orderOut:self];
}

- (IBAction)dspConfigCancelAction:(id)sender {
    [NSApp endSheet:_propertiesPanel returnCode:NSCancelButton];
}

- (IBAction)dspConfigOkAction:(id)sender {
    [NSApp endSheet:_propertiesPanel returnCode:NSOKButton];
}

- (IBAction)dspConfigResetAction:(id)sender {
    [_propertiesViewController reset];
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
