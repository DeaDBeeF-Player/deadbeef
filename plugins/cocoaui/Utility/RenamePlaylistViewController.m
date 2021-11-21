//
//  RenamePlaylistViewController.m
//  deadbeef
//
//  Created by Alexey Yakovenko on 21/11/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//

#import "RenamePlaylistViewController.h"

@interface RenamePlaylistViewController ()

@property (weak) IBOutlet NSTextField *nameTextField;

@end

@implementation RenamePlaylistViewController

- (void)viewDidLoad {
    [super viewDidLoad];

    self.nameTextField.stringValue = self.name;
}

- (IBAction)cancelAction:(id)sender {
    [self.popover close];
}

- (IBAction)okAction:(id)sender {
    self.name = self.nameTextField.stringValue;
    [self.delegate renamePlaylist:self doneWithName:self.nameTextField.stringValue];
    [self.popover close];
}

@end
