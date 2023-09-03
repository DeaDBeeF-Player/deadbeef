//
//  ScriptableNodeEditorViewController.m
//  DeaDBeeF
//
//  Created by Oleksiy Yakovenko on 4/24/19.
//  Copyright © 2019 Oleksiy Yakovenko. All rights reserved.
//

#import "ScriptableNodeEditorViewController.h"
#import "PropertySheetViewController.h"
#import "ScriptablePropertySheetDataSource.h"
#import "ScriptableNodeEditorWindowController.h"

#pragma mark - ScriptableNodeEditorViewController

@interface ScriptableNodeEditorViewController () <ScriptableItemDelegate,NSMenuDelegate,NSTableViewDelegate>

@property (unsafe_unretained) IBOutlet NSTableView *nodeList;
@property (strong) IBOutlet NSPanel *propertiesPanel;
@property (weak) IBOutlet NSButton *propertiesPanelResetButton;
@property (strong) IBOutlet PropertySheetViewController *propertiesViewController;
@property ScriptablePropertySheetDataSource *propertiesDataSource;
@property (weak) IBOutlet NSSegmentedControl *segmentedControl;
@property (weak) IBOutlet NSSegmentedControl *customButtonsSegmentedControl;

// for recursion
@property ScriptableNodeEditorWindowController *nodeEditorWindowController;
@property ScriptableTableDataSource *nodeDataSource;

@property (nonatomic,readonly) BOOL addEnabled;
@property (nonatomic,readonly) BOOL removeEnabled;
@property (nonatomic,readonly) BOOL configureEnabled;
@property (nonatomic,readonly) BOOL duplicateEnabled;

@end

@implementation ScriptableNodeEditorViewController

- (void)viewDidLoad {
    [super viewDidLoad];

    // Do view setup here.
    self.dataSource.delegate = self;
    self.nodeList.dataSource = self.dataSource;

    if (self.scriptableNodeEditorDelegate) {
        [self.scriptableNodeEditorDelegate scriptableNodeEditorCustomButtonsInitializer:self initButtonsInSegmentedControl:self.customButtonsSegmentedControl];
        self.customButtonsSegmentedControl.hidden = NO;
    }

    if (scriptableItemIsReorderable(self.dataSource.scriptable)) {
        [self.nodeList registerForDraggedTypes: @[_dataSource.pasteboardItemIdentifier]];
    }

    [self updateButtons];
}

- (void)reloadData {
    [self.nodeList reloadData];
    [self updateButtons];
}

- (NSMenu *)getCreateItemMenu {
    scriptableStringListItem_t *names = scriptableItemFactoryItemNames (self.dataSource.scriptable);
    if (!names) {
        return NULL;
    }

    NSMenu *menu = [NSMenu new];
    menu.delegate = self;
    menu.autoenablesItems = NO;

    NSInteger index = 0;
    scriptableStringListItem_t *n = names;
    while (n) {
        NSMenuItem *item = [[NSMenuItem alloc] initWithTitle:@(n->str) action:@selector(createNode:) keyEquivalent:@""];
        item.tag = index;
        [menu addItem:item];
        n = n->next;
        index++;
    }

    scriptableStringListFree(names);

    return menu;
}

- (NSInteger)insertionIndex {
    NSInteger cnt = [_dataSource numberOfRowsInTableView:_nodeList];
    NSInteger index = _nodeList.selectedRow;
    if (cnt == 0) {
        index = 0;
    }
    else if (index < 0) {
        index = cnt;
    }
    else {
        index++;
    }
    return index;
}

- (void)createNodeWithType:(const char *)type {
    scriptableItem_t *node = scriptableItemCreateItemOfType(self.dataSource.scriptable, type);
    NSInteger index = [self insertionIndex];

    NSIndexSet *is = [NSIndexSet indexSetWithIndex:index];

    [_nodeList beginUpdates];
    [_nodeList insertRowsAtIndexes:is withAnimation:NSTableViewAnimationSlideDown];
    [_dataSource insertItem:node atIndex:index];
    [_nodeList endUpdates];
    [_nodeList selectRowIndexes:is byExtendingSelection:NO];
    [_nodeList scrollRowToVisible:index];
}

- (void)createNode:(id)sender {
    NSMenuItem *item = sender;

    scriptableStringListItem_t *types = scriptableItemFactoryItemTypes (self.dataSource.scriptable);
    if (!types) {
        return;
    }

    NSInteger index = 0;

    scriptableStringListItem_t *t = types;
    while (t) {
        if (index == item.tag) {
            break;
        }
        t = t->next;
        index++;
    }

    if (t) {
        [self createNodeWithType:t->str];
    }

    scriptableStringListFree (types);
}

- (IBAction)addAction:(id)sender {
    scriptableStringListItem_t *names = scriptableItemFactoryItemNames (self.dataSource.scriptable);
    if (!names) {
        return;
    }

    if (!names->next) {
        // single action
        scriptableStringListItem_t *types = scriptableItemFactoryItemTypes (self.dataSource.scriptable);
        if (!types) {
            return;
        }

        [self createNodeWithType:types->str];

        scriptableStringListFree (names);
        scriptableStringListFree (types);

        return;
    }


    NSMenu *menu = [self getCreateItemMenu];
    if (menu) {
        [NSMenu popUpContextMenu:menu withEvent:NSApp.currentEvent forView:sender];
    }
}

- (IBAction)removeAction:(id)sender {
    NSInteger index = _nodeList.selectedRow;
    if (index < 0) {
        return;
    }

    [_nodeList beginUpdates];
    NSIndexSet *is = [NSIndexSet indexSetWithIndex:index];
    [_nodeList removeRowsAtIndexes:is withAnimation:NSTableViewAnimationSlideUp];
    [_nodeList endUpdates];
    [_dataSource removeItemAtIndex:(int)index];

    if (index >= _nodeList.numberOfRows) {
        index--;
    }
    if (index >= 0) {
        [_nodeList selectRowIndexes:[NSIndexSet indexSetWithIndex:index] byExtendingSelection:NO];
    }
    [self updateButtons];
    [self.delegate scriptableItemChanged:self.dataSource.scriptable change:ScriptableItemChangeUpdate];
}

- (IBAction)configureAction:(id)sender {
    NSInteger index = _nodeList.selectedRow;
    if (index < 0) {
        return;
    }
    scriptableItem_t *item = scriptableItemChildAtIndex(_dataSource.scriptable, (unsigned int)index);

    if (scriptableItemIsList(item)) {
        // recurse!
        self.nodeEditorWindowController = [[ScriptableNodeEditorWindowController alloc] initWithWindowNibName:@"ScriptableNodeEditorWindow"];
        self.nodeDataSource = [ScriptableTableDataSource dataSourceWithScriptable:item];
        self.nodeEditorWindowController.dataSource = self.nodeDataSource;
        self.nodeEditorWindowController.delegate = self.delegate;
        self.nodeEditorWindowController.window.title = @(scriptableItemPropertyValueForKey(item, "name")); // preset name
        NSWindow *window = self.view.window;
        [window beginSheet:self.nodeEditorWindowController.window completionHandler:^(NSModalResponse returnCode) {
        }];

    }
    else {
        self.propertiesViewController.labelFontSize = 10;
        self.propertiesViewController.contentFontSize = 11;
        self.propertiesViewController.unitSpacing = 4;
        self.propertiesViewController.autoAlignLabels = NO;

        self.propertiesDataSource.delegate = self;
        self.propertiesDataSource = [[ScriptablePropertySheetDataSource alloc] initWithScriptable:item];

        self.propertiesViewController.dataSource = self.propertiesDataSource;
        self.propertiesPanelResetButton.enabled = !scriptableItemIsReadOnly(item);
        [self.view.window beginSheet:_propertiesPanel completionHandler:^(NSModalResponse returnCode) {
        }];
    }
}

- (void)duplicateAction:(id)sender {
    NSInteger selectedIndex = _nodeList.selectedRow;
    if (selectedIndex == -1) {
        return;
    }
    scriptableItem_t *item = scriptableItemChildAtIndex(self.dataSource.scriptable, (unsigned int)selectedIndex);

    NSInteger insertIndex = [self insertionIndex];
    NSIndexSet *is = [NSIndexSet indexSetWithIndex:insertIndex];
    [_nodeList beginUpdates];
    [_nodeList insertRowsAtIndexes:is withAnimation:NSTableViewAnimationSlideDown];
    [self.dataSource duplicateItem:item atIndex:insertIndex];
    [_nodeList endUpdates];
    [_nodeList selectRowIndexes:is byExtendingSelection:NO];
    [_nodeList scrollRowToVisible:insertIndex];
}

- (IBAction)configOkAction:(id)sender {
    [NSApp endSheet:_propertiesPanel returnCode:NSModalResponseOK];
}

- (IBAction)configResetAction:(id)sender {
    [self.propertiesViewController reset];
}

- (IBAction)segmentedControlAction:(id)sender {
    NSInteger selectedSegment = [sender selectedSegment];

    switch (selectedSegment) {
    case 0:
        [self addAction:sender];
        break;
    case 1:
        [self removeAction:sender];
        break;
    case 2:
        [self configureAction:sender];
        break;
    case 3:
        [self duplicateAction:sender];
        break;
    }
}

- (scriptableItem_t *)selectedItem {
    NSInteger selectedIndex = _nodeList.selectedRow;
    if (selectedIndex == -1) {
        return NULL;
    }
    return scriptableItemChildAtIndex(self.dataSource.scriptable, (unsigned int)selectedIndex);
}

- (BOOL)addEnabled {
    return !scriptableItemIsReadOnly(self.dataSource.scriptable);
}

- (BOOL)removeEnabled {
    scriptableItem_t *item = [self selectedItem];
    if (!item) {
        return NO;
    }
    return !scriptableItemIsReadOnly(item);
}

- (BOOL)configureEnabled {
    return [self selectedItem] != NULL;
}

- (BOOL)duplicateEnabled {
    return [self selectedItem] != NULL;
}

- (void)updateButtons {
    [self.segmentedControl setEnabled:self.addEnabled forSegment:0];
    [self.segmentedControl setEnabled:self.removeEnabled forSegment:1];
    [self.segmentedControl setEnabled:self.configureEnabled forSegment:2];
    [self.segmentedControl setEnabled:self.duplicateEnabled forSegment:3];
}

#pragma mark - ScriptableItemDelegate

- (void)scriptableItemChanged:(scriptableItem_t *)scriptable change:(ScriptableItemChange)change {
    [self.delegate scriptableItemChanged:scriptable change:change];
}

#pragma mark - NSTableViewDelegate


- (nullable NSView *)tableView:(NSTableView *)tableView viewForTableColumn:(nullable NSTableColumn *)tableColumn row:(NSInteger)row {
    NSTableCellView *view = [tableView makeViewWithIdentifier:tableColumn.identifier owner:self];

    scriptableItem_t *item = scriptableItemChildAtIndex(self.dataSource.scriptable, (unsigned int)row);
    char *name = scriptableItemFormattedName(item);

    view.textField.enabled = !scriptableItemIsReadOnly(item);
    if (scriptableItemIsRenamable(self.dataSource.scriptable)) {
        view.textField.selectable = NO;
        view.textField.editable = YES;
    }
    else {
        view.textField.editable = NO;
        view.textField.selectable = YES;
    }

    view.textField.stringValue = @(name);

    free (name);

    return view;
}

- (void)tableViewSelectionDidChange:(NSNotification *)notification {
    [self updateButtons];
}


#pragma mark Save edited text

- (IBAction)textEdited:(NSTextField *)textField {
    NSInteger row = [self.nodeList rowForView:textField];
    if (row != -1) {
        const char *value = textField.stringValue.UTF8String;
        scriptableItem_t *item = scriptableItemChildAtIndex(self.dataSource.scriptable, (unsigned int)row);
        const char *name = scriptableItemPropertyValueForKey(item, "name");
        if (!strcmp (name, value)) {
            return; // name unchanged
        }

        if (scriptableItemContainsSubItemWithName (scriptableItemParent(item), value)) {
            [self.errorViewer scriptableErrorViewer:self duplicateNameErrorForItem:item];
            [textField becomeFirstResponder];
        }
        else if (!scriptableItemIsSubItemNameAllowed (scriptableItemParent(item), value)) {
            [self.errorViewer scriptableErrorViewer:self invalidNameErrorForItem:item];
            [textField becomeFirstResponder];
        }
        else {
            scriptableItemSetPropertyValueForKey(item, value, "name");
            [self.delegate scriptableItemChanged:self.dataSource.scriptable change:ScriptableItemChangeUpdate];
        }
    }
}


@end
