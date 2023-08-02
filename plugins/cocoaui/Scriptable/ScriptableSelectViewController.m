//
//  SciptableSelectViewController.m
//  DeaDBeeF
//
//  Created by Oleksiy Yakovenko on 4/24/19.
//  Copyright © 2019 Oleksiy Yakovenko. All rights reserved.
//

#import "ScriptableSelectViewController.h"
#import "ScriptableNodeEditorWindowController.h"

@interface ScriptableSelectViewController ()

@property (weak) IBOutlet NSPopUpButton *nameList;
@property (weak) IBOutlet NSButton *browseButton;
@property (nonatomic) ScriptableNodeEditorWindowController *nodeEditorWindowController;

@end

@implementation ScriptableSelectViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do view setup here.
}

- (void)setDataSource:(ScriptableTableDataSource *)dataSource {
    _dataSource = dataSource;
    [self reloadData];
}

- (IBAction)nameSelectedAction:(NSPopUpButton *)sender {
    NSUInteger index = sender.indexOfSelectedItem;
    [self.scriptableSelectDelegate scriptableSelectItemSelected:scriptableItemChildAtIndex(self.dataSource.scriptable, (unsigned int)index)];
}

- (void)initNodeEditorWindowController {
    self.nodeEditorWindowController = [[ScriptableNodeEditorWindowController alloc] initWithWindowNibName:@"ScriptableNodeEditorWindow"];
    self.nodeEditorWindowController.dataSource = self.dataSource;
    self.nodeEditorWindowController.delegate = self.scriptableItemDelegate;
    self.nodeEditorWindowController.errorViewer = self.errorViewer;
}

- (IBAction)browseButtonAction:(id)sender {
    [self initNodeEditorWindowController];

    [self.view.window beginSheet:self.nodeEditorWindowController.window completionHandler:^(NSModalResponse returnCode) {
    }];
}

- (void)reloadData {
    NSInteger index = self.indexOfSelectedItem;

    [self.nameList removeAllItems];
    for (scriptableItem_t *c = self.dataSource.scriptable->children; c; c = c->next) {
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

@end
