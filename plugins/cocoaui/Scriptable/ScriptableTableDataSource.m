#import "ScriptableTableDataSource.h"
#include "deadbeef.h"
#include "../../../scriptable/scriptable.h"

extern DB_functions_t *deadbeef;

@interface ScriptableTableDataSource()
@property (nonatomic,readwrite) NSString *pasteboardItemIdentifier;
@property (nonatomic,readwrite) scriptableItem_t *scriptable;
@end

@implementation ScriptableTableDataSource

- (void)dealloc {
    if (_scriptable) {
        scriptableItemFree (_scriptable);
        _scriptable = NULL;
    }
}

- (BOOL)editableNames {
    if (!_scriptable) {
        return NO;
    }
    const char *editable = scriptableItemPropertyValueForKey(_scriptable, "editableNames");
    return editable != NULL;
}

- (ScriptableTableDataSource *)initWithScriptable:(scriptableItem_t *)scriptable pasteboardItemIdentifier:(NSString *)identifier {
    self = [super init];
    self.pasteboardItemIdentifier = identifier;

    _scriptable = scriptable;
    return self;
}

- (void)insertItem:(scriptableItem_t *)item atIndex:(NSInteger)index {
    scriptableItemInsertSubItemAtIndex(_scriptable, item, (unsigned int)index);
    [self.delegate scriptableItemChanged:_scriptable change:ScriptableItemChangeCreate];
}

- (void)removeItemAtIndex:(NSInteger)index {
    scriptableItem_t *item = scriptableItemChildAtIndex(_scriptable, (unsigned int)index);
    if (item) {
        [self.delegate scriptableItemChanged:_scriptable change:ScriptableItemChangeDelete];
        scriptableItemRemoveSubItem(_scriptable, item);
        scriptableItemFree (item);
    }
}

- (scriptableItem_t *)itemAtIndex:(NSInteger)index {
    return scriptableItemChildAtIndex(_scriptable, (unsigned int)index);
}

#pragma mark - NSTableViewDataSource

- (NSInteger)numberOfRowsInTableView:(NSTableView *)tableView {
    return scriptableItemNumChildren (_scriptable);
}

#pragma mark Drag & Drop

- (id <NSPasteboardWriting>)tableView:(NSTableView *)tableView pasteboardWriterForRow:(NSInteger)row {
    NSString *identifier = [NSString stringWithFormat:@"%d", (int)row];

    NSPasteboardItem *pboardItem = [NSPasteboardItem new];
    [pboardItem setString:identifier forType: self.pasteboardItemIdentifier];

    return pboardItem;
}

- (NSDragOperation)tableView:(NSTableView *)tableView validateDrop:(id<NSDraggingInfo>)info proposedRow:(NSInteger)row proposedDropOperation:(NSTableViewDropOperation)dropOperation {

    BOOL canDrag = row >= 0;

    if (canDrag) {
        return NSDragOperationMove;
    }else {
        return NSDragOperationNone;
    }
}

- (BOOL)tableView:(NSTableView *)tableView acceptDrop:(id<NSDraggingInfo>)info row:(NSInteger)row dropOperation:(NSTableViewDropOperation)dropOperation {

    NSPasteboard *p = [info draggingPasteboard];
    NSInteger sourceRow = [[p stringForType:self.pasteboardItemIdentifier] intValue];

    if (sourceRow == row || sourceRow >= [self numberOfRowsInTableView:tableView] || sourceRow < 0) {
        return NO;
    }

    // fetch and remove list item
    if (row > sourceRow) {
        row--;
    }

    scriptableItem_t *node = scriptableItemChildAtIndex(_scriptable, (unsigned int)sourceRow);
    scriptableItemRemoveSubItem(_scriptable, node);

    // reinsert the node at new position
    scriptableItemInsertSubItemAtIndex (_scriptable, node, (unsigned int)row);

    [self.delegate scriptableItemChanged:_scriptable change:ScriptableItemChangeCreate];

    [tableView beginUpdates];
    [tableView moveRowAtIndex:sourceRow toIndex:row];
    [tableView endUpdates];

    return YES;
}

@end
