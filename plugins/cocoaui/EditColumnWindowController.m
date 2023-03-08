//
//  EditColumnWindowController.m
//  DeaDBeeF
//
//  Created by Oleksiy Yakovenko on 11/26/19.
//  Copyright © 2019 Oleksiy Yakovenko. All rights reserved.
//

#import "PlaylistViewController.h"
#import "EditColumnWindowController.h"
#include <deadbeef/deadbeef.h>

@interface EditColumnWindowController()

@property (nonatomic) NSString *title;
@property (nonatomic) int type;
@property (nonatomic) NSString *format;
@property (nonatomic) NSString *sortFormat;
@property (nonatomic) PlaylistColumnAlignment alignment;
@property (nonatomic) BOOL setTextColor;
@property (nonatomic) NSColor *textColor;

@end

@implementation EditColumnWindowController

- (void)windowDidLoad {
    [super windowDidLoad];
    [self initUI];
}

- (void)initAddColumnSheet {
    [self initEditColumnSheetWithTitle:@""
                                  type:DB_COLUMN_CUSTOM
                                format:@""
                                sortFormat:@""
                             alignment:ColumnAlignmentLeft
                          setTextColor:NO
                             textColor:NSColor.blackColor];
}

- (void)initUI {
    if (!self.titleTextField) {
        return;
    }

    int type = 10; // custom
    switch (self.type) {
    case DB_COLUMN_FILENUMBER:
        type = 0;
        break;
    case DB_COLUMN_PLAYING:
        type = 1;
        break;
    case DB_COLUMN_ALBUM_ART:
        type = 2;
        break;
    }

    self.titleTextField.stringValue = self.title;
    [self.typePopUpButton selectItemAtIndex: type];
    self.formatTextField.enabled = type == 10;
    self.formatTextField.stringValue = self.format;
    self.sortFormatTextField.stringValue = self.sortFormat;
    [self.alignmentPopUpButton selectItemAtIndex:self.alignment];
    self.setColorButton.state = self.setTextColor;
    self.colorWell.enabled = self.setTextColor;
    NSColorPanel.sharedColorPanel.showsAlpha = YES;
}

- (void)initEditColumnSheetWithTitle:(NSString *)title
                                type:(int)inputType
                              format:(NSString *)format
                          sortFormat:(NSString *)sortFormat
                           alignment:(PlaylistColumnAlignment)alignment
                        setTextColor:(BOOL)setTextColor
                           textColor:(NSColor *)textColor {

    self.title = title;
    self.type = inputType;
    self.format = format;
    self.sortFormat = sortFormat;
    self.alignment = alignment;
    self.setTextColor = setTextColor;
    self.textColor = textColor;

    [self initUI];
}

- (IBAction)addColumnTypeChanged:(id)sender {
    BOOL isCustom = self.typePopUpButton.indexOfSelectedItem == 10;
    self.formatTextField.enabled =  isCustom;
    self.titleTextField.stringValue = self.typePopUpButton.selectedItem.title;
}

- (IBAction)addColumnSetColorChanged:(NSButton *)sender {
    self.colorWell.enabled = sender.state == NSControlStateValueOn;
}

- (IBAction)addColumnCancel:(id)sender {
    [NSApp endSheet:self.window returnCode:NSModalResponseCancel];
}

- (IBAction)addColumnOK:(id)sender {
    [NSApp endSheet:self.window returnCode:NSModalResponseOK];
}



@end
