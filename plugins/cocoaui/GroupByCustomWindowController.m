//
//  GroupByCustomWindowController.m
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 11/26/19.
//  Copyright © 2019 Alexey Yakovenko. All rights reserved.
//

#import "GroupByCustomWindowController.h"

@interface GroupByCustomWindowController ()

@property (nonatomic) NSString *format;

@end

@implementation GroupByCustomWindowController

- (void)windowDidLoad {
    [super windowDidLoad];

    [self initUI];
}

- (void)initUI {
    self.formatTextField.stringValue = self.format;
}

- (void)initWithFormat:(NSString *)format {
    self.format = format;

    [self initUI];
}

- (IBAction)cancelAction:(id)sender {
    [NSApp endSheet:self.window returnCode:NSModalResponseCancel];
}

- (IBAction)okAction:(id)sender {
    [NSApp endSheet:self.window returnCode:NSModalResponseOK];
}

@end
