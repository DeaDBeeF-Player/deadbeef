//
//  ScriptableNodeEditorWindowController.m
//  DeaDBeeF
//
//  Created by Oleksiy Yakovenko on 4/25/19.
//  Copyright © 2019 Oleksiy Yakovenko. All rights reserved.
//

#import "ScriptableNodeEditorWindowController.h"
#import "ScriptableNodeEditorViewController.h"

@interface ScriptableNodeEditorWindowController ()

@property (weak) IBOutlet NSView *nodeEditorViewContainer;
@property (weak) IBOutlet NSTextField *title;

@property ScriptableNodeEditorViewController *nodeEditorViewController;

@end

@implementation ScriptableNodeEditorWindowController

- (void)windowDidLoad {
    [super windowDidLoad];
    
    // Implement this method to handle any initialization after your window controller's window has been loaded from its nib file.

    self.nodeEditorViewController = [ScriptableNodeEditorViewController new];
    self.nodeEditorViewController.dataSource = self.dataSource;
    self.nodeEditorViewController.delegate = self.delegate;
    self.nodeEditorViewController.errorViewer = self.errorViewer;
    self.nodeEditorViewController.view.frame = self.nodeEditorViewContainer.bounds;
    [self.nodeEditorViewContainer addSubview:self.nodeEditorViewController.view];

    const char *title = scriptableItemPropertyValueForKey(self.dataSource.scriptable, "name");

    self.title.stringValue = [NSString stringWithFormat:@"Edit %s", title];
}

- (IBAction)closeAction:(id)sender {
    if (self.window.sheetParent) {
        [self.window.sheetParent endSheet:self.window returnCode:NSModalResponseOK];
    }
    else {
        [self.window close];
    }
}

@end
