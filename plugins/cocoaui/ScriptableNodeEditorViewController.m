//
//  ScriptableNodeEditorViewController.m
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 4/24/19.
//  Copyright © 2019 Alexey Yakovenko. All rights reserved.
//

#import "ScriptableNodeEditorViewController.h"
#import "PropertySheetViewController.h"
#import "ScriptablePropertySheetDataSource.h"

@interface ScriptableNodeEditorViewController () <ScriptableItemDelegate,NSMenuDelegate>

@property (unsafe_unretained) IBOutlet NSTableView *nodeList;
@property (strong) IBOutlet NSPanel *propertiesPanel;
@property (strong) IBOutlet PropertySheetViewController *propertiesViewController;
@property ScriptablePropertySheetDataSource *propertiesDataSource;

@end

@implementation ScriptableNodeEditorViewController

- (void)viewDidLoad {
    [super viewDidLoad];

    // Do view setup here.
    self.dataSource.delegate = self;
    _nodeList.dataSource = self.dataSource;
    [_nodeList registerForDraggedTypes: [NSArray arrayWithObjects: _dataSource.pasteboardItemIdentifier, nil]];
}

#pragma mark - ScriptableItemDelegate

- (void)scriptableItemChanged:(scriptableItem_t *)scriptable {
    [self.delegate scriptableItemChanged:scriptable];
}


- (NSMenu *)getCreateItemMenu {
    scriptableStringListItem_t *names = scriptableItemFactoryItemNames (self.dataSource.scriptable);
    if (!names) {
        return NULL;
    }

    NSMenu *menu = [[NSMenu alloc] init];
    menu.delegate = self;
    [menu setAutoenablesItems:NO];

    NSInteger index = 0;
    scriptableStringListItem_t *n = names;
    while (n) {
        NSMenuItem *item = [[NSMenuItem alloc] initWithTitle:[NSString stringWithUTF8String:n->str] action:@selector(createNode:) keyEquivalent:@""];
        item.tag = index;
        [menu addItem:item];
        n = n->next;
        index++;
    }

    scriptableStringListFree(names);

    return menu;
}

- (void)createNode:(id)sender {
    NSMenuItem *item = sender;

    scriptableStringListItem_t *types = scriptableItemFactoryItemTypes (self.dataSource.scriptable);
    if (!types) {
        return;
    }

    NSInteger index = 0;//= item.tag;

    scriptableStringListItem_t *t = types;
    while (t) {
        if (index == item.tag) {
            break;
        }
        t = t->next;
        index++;
    }

    if (t) {
        scriptableItem_t *node = scriptableItemCreateItemOfType(self.dataSource.scriptable, t->str);
        id<NSTableViewDataSource> ds = _dataSource;
        NSInteger cnt = [ds numberOfRowsInTableView:_nodeList];
        NSInteger index = [_nodeList selectedRow];
        if (index < 0) {
            index = cnt;
        }

        NSIndexSet *is = [NSIndexSet indexSetWithIndex:index];
        [_nodeList beginUpdates];
        [_nodeList insertRowsAtIndexes:is withAnimation:NSTableViewAnimationSlideDown];
        [_dataSource insertItem:node atIndex:index];
        [_nodeList endUpdates];
        [_nodeList selectRowIndexes:is byExtendingSelection:NO];
    }

    scriptableStringListFree (types);
}

- (IBAction)addAction:(id)sender {
    NSMenu *menu = [self getCreateItemMenu];
    if (menu) {
        [NSMenu popUpContextMenu:menu withEvent:[NSApp currentEvent] forView:sender];
    }
}

- (IBAction)removeAction:(id)sender {
    NSInteger index = [_nodeList selectedRow];
    if (index < 0) {
        return;
    }

    [_nodeList beginUpdates];
    NSIndexSet *is = [NSIndexSet indexSetWithIndex:index];
    [_nodeList removeRowsAtIndexes:is withAnimation:NSTableViewAnimationSlideUp];
    [_nodeList endUpdates];
    [_dataSource removeItemAtIndex:(int)index];

    if (index >= [_nodeList numberOfRows]) {
        index--;
    }
    if (index >= 0) {
        [_nodeList selectRowIndexes:[NSIndexSet indexSetWithIndex:index] byExtendingSelection:NO];
    }
}

- (IBAction)configureAction:(id)sender {
    NSInteger index = [_nodeList selectedRow];
    if (index < 0) {
        return;
    }
    scriptableItem_t *item = scriptableItemChildAtIndex(_dataSource.scriptable, (unsigned int)index);
    self.propertiesDataSource = [[ScriptablePropertySheetDataSource alloc] initWithScriptable:item];
    self.propertiesDataSource.delegate = self;

    _propertiesViewController.dataSource = self.propertiesDataSource;
    [NSApp beginSheet:_propertiesPanel modalForWindow:self.view.window modalDelegate:self didEndSelector:@selector(didEndConfigPanel:returnCode:contextInfo:) contextInfo:nil];
}

- (void)didEndConfigPanel:(NSWindow *)sheet returnCode:(NSInteger)returnCode contextInfo:(void *)contextInfo {
    [_propertiesPanel orderOut:self];
}

- (IBAction)configCancelAction:(id)sender {
    [NSApp endSheet:_propertiesPanel returnCode:NSCancelButton];
}

- (IBAction)configOkAction:(id)sender {
    [NSApp endSheet:_propertiesPanel returnCode:NSOKButton];
}

- (IBAction)configResetAction:(id)sender {
    [_propertiesViewController reset];
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
    }
}

- (IBAction)saveAction:(id)sender {
}

- (IBAction)loadAction:(id)sender {
}

@end
