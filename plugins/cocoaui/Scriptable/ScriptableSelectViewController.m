//
//  SciptableSelectViewController.m
//  DeaDBeeF
//
//  Created by Oleksiy Yakovenko on 4/24/19.
//  Copyright © 2019 Oleksiy Yakovenko. All rights reserved.
//

#import "ScriptableSelectViewController.h"
#import "ScriptableNodeEditorWindowController.h"

@interface ScriptableSelectViewController () <ScriptableItemDelegate> {
    scriptableModelAPI_t *_modelAPI;
    int64_t _modelListenerId;
    BOOL _updatingModel;
}

@property (weak) IBOutlet NSPopUpButton *nameList;
@property (weak) IBOutlet NSButton *browseButton;
@property (nonatomic) ScriptableNodeEditorWindowController *nodeEditorWindowController;

@end

@implementation ScriptableSelectViewController

@synthesize scriptableModel = _scriptableModel;

- (void)setDataSource:(ScriptableTableDataSource *)dataSource {
    _dataSource = dataSource;
    [self reloadData];
}

- (IBAction)nameSelectedAction:(NSPopUpButton *)sender {
    NSUInteger index = sender.indexOfSelectedItem;
    [self updateModelFromCurrent];
    [self.delegate scriptableSelectItemSelected:scriptableItemChildAtIndex(self.dataSource.scriptable, (unsigned int)index)];
}

- (void)initNodeEditorWindowController {
    self.nodeEditorWindowController = [[ScriptableNodeEditorWindowController alloc] initWithWindowNibName:@"ScriptableNodeEditorWindow"];
    self.nodeEditorWindowController.dataSource = self.dataSource;
    self.nodeEditorWindowController.delegate = self;
    self.nodeEditorWindowController.errorViewer = self.errorViewer;
}

- (IBAction)browseButtonAction:(id)sender {
    [self initNodeEditorWindowController];

    [self.view.window beginSheet:self.nodeEditorWindowController.window completionHandler:^(NSModalResponse returnCode) {
    }];
}

- (void)reloadData {
    if (self.dataSource == nil) {
        return;
    }
    NSInteger index = self.indexOfSelectedItem;

    [self.nameList removeAllItems];
    for (scriptableItem_t *c = scriptableItemChildren(self.dataSource.scriptable); c; c = scriptableItemNext(c)) {
        const char *name = scriptableItemPropertyValueForKey(c, "name");
        if (name) {
            [self.nameList addItemWithTitle:@(name)];
        }
    }

    if (index != -1) {
        [self.nameList selectItemAtIndex:index];
    }
}

- (NSInteger)indexOfSelectedItem {
    return self.nameList.indexOfSelectedItem;
}

- (void)selectItem:(scriptableItem_t *)item {
    int index = scriptableItemIndexOfChild(self.dataSource.scriptable, item);
    if (index != -1) {
        [self.nameList selectItemAtIndex:index];
    }
}

- (void)updateCurrentFromModel {
    if (_scriptableModel == NULL) {
        return;
    }
    char *preset = _modelAPI->get_active_name (_scriptableModel);
    scriptableItem_t *currentPreset = scriptableItemSubItemForName (self.dataSource.scriptable, preset);
    if (currentPreset != NULL) {
        [self selectItem:currentPreset];
    }
    free (preset);
}

- (void)updateModelFromCurrent {
    if (_scriptableModel == NULL) {
        return;
    }

    NSUInteger index = self.nameList.indexOfSelectedItem;
    scriptableItem_t *item = scriptableItemChildAtIndex(self.dataSource.scriptable, (unsigned int)index);

    const char *name = "";
    if (item != NULL) {
        name = scriptableItemPropertyValueForKey (item, "name");
    }
    _updatingModel = YES;
    _modelAPI->set_active_name (_scriptableModel, name);
    _updatingModel = NO;
}

static void
_model_listener (struct scriptableModel_t *model, void *user_data) {
    ScriptableSelectViewController *self = (__bridge ScriptableSelectViewController *)user_data;
    [self modelListener];
}

- (void)modelListener {
    if (_updatingModel) {
        return;
    }

    [self updateCurrentFromModel];
}

- (scriptableModel_t *)scriptableModel {
    return _scriptableModel;
}

- (void)setScriptableModel:(scriptableModel_t *)scriptableModel {
    if (_scriptableModel != NULL) {
        _modelAPI->remove_listener (_scriptableModel, self->_modelListenerId);
        _modelListenerId = 0;
        _modelAPI = NULL;
    }

    _scriptableModel = scriptableModel;

    if (scriptableModel != NULL) {
        _modelAPI = scriptableModelGetAPI (scriptableModel);
        _modelListenerId = _modelAPI->add_listener (scriptableModel, _model_listener, (__bridge void *)self);

        [self updateCurrentFromModel];
    }
}

#pragma mark - ScriptableItemDelegate

- (void)scriptableItemDidChange:(scriptableItem_t *)scriptable change:(ScriptableItemChange)change {
    [self updateModelFromCurrent];
    [self.delegate scriptableItemDidChange:scriptable change:change];
}

@end
