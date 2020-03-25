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

- (IBAction)nameSelectedAction:(id)sender {
}

- (IBAction)browseButtonAction:(id)sender {
    if (!self.nodeEditorWindowController) {
        self.nodeEditorWindowController = [[ScriptableNodeEditorWindowController alloc] initWithWindowNibName:@"ScriptableNodeEditorWindow"];
        self.nodeEditorWindowController.dataSource = self.dataSource;
        self.nodeEditorWindowController.delegate = self.delegate;
    }

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
