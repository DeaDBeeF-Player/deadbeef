//
//  SciptableSelectViewController.m
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 4/24/19.
//  Copyright © 2019 Alexey Yakovenko. All rights reserved.
//

#import "ScriptableSelectViewController.h"
#import "ScriptableNodeEditorWindowController.h"

@interface ScriptableSelectViewController ()

@property (nonatomic) scriptableItem_t *scriptable;

@property (weak) IBOutlet NSPopUpButton *nameList;
@property (weak) IBOutlet NSButton *browseButton;
@property (nonatomic) ScriptableNodeEditorWindowController *nodeEditorWindowController;

@end

@implementation ScriptableSelectViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do view setup here.
}

- (void)setScriptable:(scriptableItem_t *)scriptable {
    _scriptable = scriptable;
    [self reloadData];
}

- (IBAction)nameSelectedAction:(NSPopUpButton *)sender {
    NSUInteger index = sender.indexOfSelectedItem;
    [self.scriptableSelectDelegate scriptableSelectItemSelected:scriptableItemChildAtIndex(self.scriptable, (unsigned int)index)];
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
    [self.nameList removeAllItems];
    for (scriptableItem_t *c = self.scriptable->children; c; c = c->next) {
        const char *name = scriptableItemPropertyValueForKey(c, "name");
        if (name) {
            [self.nameList addItemWithTitle:[NSString stringWithUTF8String:name]];
        }
    }
}

@end
