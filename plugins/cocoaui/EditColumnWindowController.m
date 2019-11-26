//
//  EditColumnWindowController.m
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 11/26/19.
//  Copyright © 2019 Alexey Yakovenko. All rights reserved.
//

#import "EditColumnWindowController.h"
#import "deadbeef.h"

@interface EditColumnWindowController()

@property (nonatomic) NSString *title;
@property (nonatomic) int type;
@property (nonatomic) NSString *format;
@property (nonatomic) int alignment;
@property (nonatomic) BOOL setTextColor;
@property (nonatomic) NSColor *textColor;

@end

@implementation EditColumnWindowController

- (void)windowDidLoad {
    [super windowDidLoad];

    [self initEditColumnSheetWithTitle:self.title
                                  type:self.type
                                format:self.format
                             alignment:self.alignment
                          setTextColor:self.setTextColor
                             textColor:self.textColor];
}

- (void)initAddColumnSheet {
    [self initEditColumnSheetWithTitle:@""
                                  type:DB_COLUMN_CUSTOM
                                format:@""
                             alignment:-1
                          setTextColor:NO
                             textColor:NSColor.blackColor];
}

- (void)initEditColumnSheetWithTitle:(NSString *)title
                                type:(int)inputType
                              format:(NSString *)format
                           alignment:(int)alignment
                        setTextColor:(BOOL)setTextColor
                           textColor:(NSColor *)textColor {

    self.title = title;
    self.type = inputType;
    self.format = format;
    self.alignment = alignment;
    self.setTextColor = setTextColor;
    self.textColor = textColor;

    int type = 10; // custom
    switch (inputType) {
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

    if (!self.titleTextField) {
        return;
    }

    self.titleTextField.stringValue = title;
    [self.typePopUpButton selectItemAtIndex: type];
    self.formatTextField.enabled = type == 10;
    self.formatTextField.stringValue = format;
    [self.alignmentPopUpButton selectItemAtIndex:alignment];
    self.setColorButton.state = setTextColor;
    self.colorWell.enabled = setTextColor;
    NSColorPanel.sharedColorPanel.showsAlpha = YES;
}

- (IBAction)addColumnTypeChanged:(id)sender {
    BOOL isCustom = self.typePopUpButton.indexOfSelectedItem == 10;
    self.formatTextField.enabled =  isCustom;
    self.titleTextField.stringValue = self.typePopUpButton.selectedItem.title;
}

- (IBAction)addColumnSetColorChanged:(NSButton *)sender {
    self.colorWell.enabled = sender.state == NSOnState;
}

- (IBAction)addColumnCancel:(id)sender {
    [NSApp endSheet:self.window returnCode:NSModalResponseCancel];
}

- (IBAction)addColumnOK:(id)sender {
    [NSApp endSheet:self.window returnCode:NSModalResponseOK];
}



@end
