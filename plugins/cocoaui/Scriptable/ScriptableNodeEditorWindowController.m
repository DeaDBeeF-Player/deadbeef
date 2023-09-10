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
@property (weak) IBOutlet NSButton *resetButton;

@property ScriptableNodeEditorViewController *nodeEditorViewController;

@end

@implementation ScriptableNodeEditorWindowController

- (void)windowDidLoad {
    [super windowDidLoad];

    self.nodeEditorViewController = [ScriptableNodeEditorViewController new];
    self.nodeEditorViewController.dataSource = self.dataSource;
    self.nodeEditorViewController.delegate = self.delegate;
    self.nodeEditorViewController.errorViewer = self.errorViewer;
    self.nodeEditorViewController.view.frame = self.nodeEditorViewContainer.bounds;
    [self.nodeEditorViewContainer addSubview:self.nodeEditorViewController.view];

    self.resetButton.hidden = !self.nodeEditorViewController.canReset;

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

- (IBAction)resetAction:(id)sender {
    NSAlert *alert = [NSAlert new];
    [alert addButtonWithTitle:@"Yes"];
    [alert addButtonWithTitle:@"Cancel"];
    alert.messageText = @"Reset to default settings?";
    alert.informativeText = @"Selecting Yes will discard your local changes.";
    alert.alertStyle = NSAlertStyleWarning;

    [alert beginSheetModalForWindow:self.window completionHandler:^(NSModalResponse returnCode) {
        if (returnCode == NSAlertFirstButtonReturn) {
            [self.nodeEditorViewController reset];
        }
    }];
}

@end
