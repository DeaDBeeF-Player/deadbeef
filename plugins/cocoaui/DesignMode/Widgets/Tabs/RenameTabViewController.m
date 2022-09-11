//
//  RenameTabViewController.m
//  deadbeef
//
//  Created by Oleksiy Yakovenko on 18/11/2021.
//  Copyright © 2021 Oleksiy Yakovenko. All rights reserved.
//

#import "RenameTabViewController.h"

@interface RenameTabViewController ()

@property (weak) IBOutlet NSTextField *nameTextField;

@end

@implementation RenameTabViewController

- (void)viewDidLoad {
    [super viewDidLoad];

    self.nameTextField.stringValue = self.name;
}

- (IBAction)doneAction:(id)sender {
    self.name = self.nameTextField.stringValue;

    [self.delegate renameTabDone:self withName:self.name];
    if (self.popover == nil) {
        [NSApp endSheet:self.view.window returnCode:NSModalResponseOK];
    }
    else {
        [self.popover close];
    }
}

@end

