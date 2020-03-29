#import "ScriptableTableDataSource.h"
#include "deadbeef.h"
#include "../../../scriptable/scriptable.h"

extern DB_functions_t *deadbeef;

@interface ScriptableReorerableTableDataSource : ScriptableTableDataSource
@end

@implementation ScriptableTableDataSource

+ (ScriptableTableDataSource *)dataSourceWithScriptable:(scriptableItem_t *)scriptable {
    if (scriptable->callbacks && scriptable->callbacks->isReorderable) {
        return [[ScriptableReorerableTableDataSource alloc] initWithScriptable:scriptable];
    }
    else {
        return [[ScriptableTableDataSource alloc] initWithScriptable:scriptable];
    }
}

- (void)dealloc
{
    if (_scriptable) {
        scriptableItemFree (_scriptable);
        _scriptable = NULL;
    }
}

- (instancetype)init {
    return [self initWithScriptable:nil];
}

- (NSString *)pasteboardItemIdentifier {
    return nil;
}

- (ScriptableTableDataSource *)initWithScriptable:(scriptableItem_t *)scriptable {
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

- (void)setScriptable:(scriptableItem_t *)scriptable {
    scriptableItemFree(_scriptable);
    _scriptable = scriptable;
    [self.delegate scriptableItemChanged:_scriptable change:ScriptableItemChangeUpdate];
}

#pragma mark NSTableViewDataSource

- (NSInteger)numberOfRowsInTableView:(NSTableView *)tableView {
    return scriptableItemNumChildren (_scriptable);
}

@end

#pragma mark - ScriptableReorerableTableDataSource

@implementation ScriptableReorerableTableDataSource

- (NSString *)pasteboardItemIdentifier {
    return [NSString stringWithUTF8String:self.scriptable->callbacks->pasteboardItemIdentifier];
}

#pragma mark NSTableViewDataSource Drag & Drop

- (id <NSPasteboardWriting>)tableView:(NSTableView *)tableView pasteboardWriterForRow:(NSInteger)row {
    if (!self.scriptable->callbacks->isReorderable) {
        return nil;
    }
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

    scriptableItem_t *node = scriptableItemChildAtIndex(self.scriptable, (unsigned int)sourceRow);
    scriptableItemRemoveSubItem(self.scriptable, node);

    // reinsert the node at new position
    scriptableItemInsertSubItemAtIndex (self.scriptable, node, (unsigned int)row);

    [self.delegate scriptableItemChanged:self.scriptable change:ScriptableItemChangeCreate];

    [tableView beginUpdates];
    [tableView moveRowAtIndex:sourceRow toIndex:row];
    [tableView endUpdates];

    return YES;
}

@end
