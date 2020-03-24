//
//  DspPreferencesViewController.m
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 2/26/20.
//  Copyright © 2020 Alexey Yakovenko. All rights reserved.
//

#import "DspPreferencesViewController.h"
#import "ScriptableTableDataSource.h"
#import "ScriptableSelectViewController.h"
#import "ScriptableNodeEditorViewController.h"
#include "scriptable_dsp.h"
#include "pluginsettings.h"
#include "deadbeef.h"

extern DB_functions_t *deadbeef;

@interface DspPreferencesViewController ()

@property (weak) IBOutlet NSView *dspPresetSelectorContainer;
@property (weak) IBOutlet NSView *dspNodeEditorContainer;

@property (nonatomic) ScriptableSelectViewController *dspSelectViewController;
@property (nonatomic) ScriptableNodeEditorViewController *dspNodeEditorViewController;

@property (nonatomic) ScriptableTableDataSource *dspChainDataSource;
@property (nonatomic) ScriptableTableDataSource *dspPresetsDataSource;


@end

@implementation DspPreferencesViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do view setup here.

    // dsp
    scriptableItem_t *chain = scriptableDspConfigFromDspChain (deadbeef->streamer_get_dsp_chain ());
    self.dspChainDataSource = [[ScriptableTableDataSource alloc] initWithScriptable:chain pasteboardItemIdentifier:@"deadbeef.dspnode.preferences"];

    self.dspPresetsDataSource = [[ScriptableTableDataSource alloc] initWithScriptable:scriptableDspRoot() pasteboardItemIdentifier:@"deadbeef.dsppreset.preferences"];

    // preset list and browse button
    self.dspSelectViewController = [[ScriptableSelectViewController alloc] initWithNibName:@"ScriptableSelectView" bundle:nil];
    self.dspSelectViewController.dataSource = self.dspPresetsDataSource;
    self.dspSelectViewController.delegate = self;
    self.dspSelectViewController.view.frame = _dspPresetSelectorContainer.bounds;
    [_dspPresetSelectorContainer addSubview:self.dspSelectViewController.view];

    // current dsp chain node list / editor
    self.dspNodeEditorViewController = [[ScriptableNodeEditorViewController alloc] initWithNibName:@"ScriptableNodeEditorView" bundle:nil];
    self.dspNodeEditorViewController.dataSource = self.dspChainDataSource;
    self.dspNodeEditorViewController.delegate = self;
    self.dspNodeEditorViewController.view.frame = _dspNodeEditorContainer.bounds;
    [_dspNodeEditorContainer addSubview:self.dspNodeEditorViewController.view];

    self.dspSelectViewController.scriptable = scriptableDspRoot();
}

#pragma mark - ScriptableItemDelegate

- (void)scriptableItemChanged:(scriptableItem_t *)scriptable {
    if (scriptable == self.dspChainDataSource.scriptable
        || scriptableItemIndexOfChild(self.dspChainDataSource.scriptable, scriptable) >= 0) {
        ddb_dsp_context_t *chain = scriptableDspConfigToDspChain (self.dspChainDataSource.scriptable);
        deadbeef->streamer_set_dsp_chain (chain);
        deadbeef->dsp_preset_free (chain);
    }
}

@end
