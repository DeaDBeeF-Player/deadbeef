/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2024 Oleksiy Yakovenko and other contributors

    This software is provided 'as-is', without any express or implied
    warranty.  In no event will the authors be held liable for any damages
    arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.

    3. This notice may not be removed or altered from any source distribution.
*/

#import "KeyboardShortcutConverter.h"
#import "KeyboardShortcutEditorViewController.h"
#import "KeyboardShortcutTextField.h"

@interface KeyboardShortcutEditorViewController () <NSOutlineViewDelegate, NSOutlineViewDataSource, KeyboardShortcutTextFieldDelegate>
@property (weak) IBOutlet NSOutlineView *outlineView;

@property (nonatomic, nullable) KeyboardShortcutViewItem *viewItem;

@end

@implementation KeyboardShortcutEditorViewController

- (void)viewDidLoad {
    [super viewDidLoad];
}

- (void)updateWithViewItem:(KeyboardShortcutViewItem *)viewItem {
    self.viewItem = viewItem;
    [self.outlineView reloadData];
    [self.outlineView expandItem:nil expandChildren:YES];
}

- (void)delete:(id)sender {
    NSInteger row = self.outlineView.selectedRow;
    if (row == -1) {
        return;
    }

    KeyboardShortcutViewItem *viewItem = [self.outlineView itemAtRow:row];
    if (viewItem != nil) {
        if (ddb_keyboard_shortcut_is_modified(viewItem.shortcut)
            && !strcmp (ddb_keyboard_shortcut_get_key_character(viewItem.shortcut), "")) {
            ddb_keyboard_shortcut_reset_to_default (viewItem.shortcut);
        }
        else {
            ddb_keyboard_shortcut_set_clear (viewItem.shortcut);
        }

        [self.model applyShortcut:viewItem.shortcut];
        [self.outlineView reloadItem:viewItem];
    }
}

#pragma mark - NSOutlineViewDataSource

- (NSInteger)outlineView:(NSOutlineView *)outlineView numberOfChildrenOfItem:(id)item {
    KeyboardShortcutViewItem *viewItem = item;
    if (item == nil) {
        viewItem = self.viewItem;
    }
    return viewItem.children.count;
}

- (id)outlineView:(NSOutlineView *)outlineView child:(NSInteger)index ofItem:(id)item {
    KeyboardShortcutViewItem *viewItem = item;
    if (item == nil) {
        viewItem = self.viewItem;
    }
    return viewItem.children[index];
}

- (BOOL)outlineView:(NSOutlineView *)outlineView isItemExpandable:(id)item {
    KeyboardShortcutViewItem *viewItem = item;
    return viewItem.children != nil;
}

#pragma mark - NSOutlineViewDelegate

- (nullable NSView *)outlineView:(NSOutlineView *)outlineView viewForTableColumn:(nullable NSTableColumn *)tableColumn item:(id)item {
    NSTableCellView *view;

    KeyboardShortcutViewItem *viewItem = item;

    if (viewItem.children != nil) {
        if ([tableColumn.identifier isEqualToString:@"TextCell1"]) {
            view = [outlineView makeViewWithIdentifier:@"Group" owner:self];
            view.textField.stringValue = [viewItem.displayText stringByAppendingString:@" Menu"];
        }
    }
    else {
        view = [outlineView makeViewWithIdentifier:tableColumn.identifier owner:self];

        if ([tableColumn.identifier isEqualToString:@"TextCell1"]) {
            view.textField.stringValue = viewItem.displayText;
        }
        else {
            NSString *keyCharacter = @(ddb_keyboard_shortcut_get_key_character (viewItem.shortcut));
            NSEventModifierFlags keyModifiers = [KeyboardShortcutConverter.shared appKitModifiersFromDdbModifiers:ddb_keyboard_shortcut_get_key_modifiers(viewItem.shortcut)];

            NSString *keyCombination = [KeyboardShortcutConverter.shared keyCombinationDisplayStringFromKeyEquivalent:keyCharacter modifierMask:keyModifiers];
            view.textField.stringValue = keyCombination ?: @"";
            view.textField.delegate = self;
        }
    }

    return view;
}

#pragma mark - KeyboardShortcutTextFieldDelegate

- (void)textFieldDidAssignShortcut:(KeyboardShortcutTextField *)textField {
    NSInteger row = [self.outlineView rowForView:textField];
    if (row != -1) {
        KeyboardShortcutViewItem *viewItem = [self.outlineView itemAtRow:row];

        ddb_keyboard_shortcut_set_key_character(viewItem.shortcut, textField.key.UTF8String);

        ddb_keyboard_shortcut_modifiers_t modifiers = [KeyboardShortcutConverter.shared ddbModifiersFromAppKitModifiers:textField.modifierFlags];
        ddb_keyboard_shortcut_set_key_modifiers(viewItem.shortcut, modifiers);

        [self.model applyShortcut:viewItem.shortcut];

    }
    [self.view.window makeFirstResponder:self.outlineView];
}

@end
