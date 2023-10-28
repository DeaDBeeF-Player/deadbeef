#import "ScriptableTableDataSource.h"
#include <deadbeef/deadbeef.h>
#include "scriptable/scriptable.h"

extern DB_functions_t *deadbeef;

@interface ScriptableReorerableTableDataSource : ScriptableTableDataSource
@end

@implementation ScriptableTableDataSource

+ (ScriptableTableDataSource *)dataSourceWithScriptable:(scriptableItem_t *)scriptable {
    if (scriptableItemFlags(scriptable) & SCRIPTABLE_FLAG_IS_REORDABLE) {
        return [[ScriptableReorerableTableDataSource alloc] initWithScriptable:scriptable];
    }
    else {
        return [[ScriptableTableDataSource alloc] initWithScriptable:scriptable];
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
    [self.delegate scriptableItemDidChange:_scriptable change:ScriptableItemChangeCreate];
}

- (void)removeItemAtIndex:(NSInteger)index {
    scriptableItem_t *item = scriptableItemChildAtIndex(_scriptable, (unsigned int)index);
    if (item) {
        [self.delegate scriptableItemDidChange:_scriptable change:ScriptableItemChangeDelete];
        scriptableItemRemoveSubItem(_scriptable, item);
        scriptableItemFree (item);
    }
}

- (void)duplicateItem:(scriptableItem_t *)item atIndex:(NSInteger)index {
    scriptableItem_t *duplicate = scriptableItemClone(item);
    char name[100];
    snprintf (name, sizeof (name), "%s (Copy)", scriptableItemPropertyValueForKey(item, "name"));
    scriptableItemSetUniqueNameUsingPrefixAndRoot(duplicate, name, self.scriptable);
    [self insertItem:duplicate atIndex:index];
}

- (scriptableItem_t *)itemAtIndex:(NSInteger)index {
    return scriptableItemChildAtIndex(_scriptable, (unsigned int)index);
}

- (void)setScriptable:(scriptableItem_t *)scriptable {
    _scriptable = scriptable;
    [self.delegate scriptableItemDidChange:_scriptable change:ScriptableItemChangeUpdate];
}

#pragma mark NSTableViewDataSource

- (NSInteger)numberOfRowsInTableView:(NSTableView *)tableView {
    return scriptableItemNumChildren (_scriptable);
}

@end

#pragma mark - ScriptableReorerableTableDataSource

@implementation ScriptableReorerableTableDataSource

- (NSString *)pasteboardItemIdentifier {
    const char *identifier = scriptableItemPasteboardIdentifier(self.scriptable);
    if (identifier == NULL) {
        return nil;
    }
    return @(identifier);
}

#pragma mark NSTableViewDataSource Drag & Drop

- (id <NSPasteboardWriting>)tableView:(NSTableView *)tableView pasteboardWriterForRow:(NSInteger)row {
    if (!(scriptableItemFlags(self.scriptable) & SCRIPTABLE_FLAG_IS_REORDABLE)) {
        return nil;
    }
    scriptableItem_t *item = scriptableItemChildAtIndex(self.scriptable, (unsigned int)row);
    if (!item) {
        return nil;
    }

    NSMutableDictionary *plist = [NSMutableDictionary new];
    plist[@"sourceRowIndex"] = @(row);

    NSPasteboardItem *pboardItem = [NSPasteboardItem new];
    [pboardItem setPropertyList:plist forType:self.pasteboardItemIdentifier];

    return pboardItem;
}

- (NSDragOperation)tableView:(NSTableView *)tableView validateDrop:(id<NSDraggingInfo>)info proposedRow:(NSInteger)row proposedDropOperation:(NSTableViewDropOperation)dropOperation {

    BOOL canDrag = row >= 0 && info.draggingSource == tableView;

    if (canDrag) {
        return NSDragOperationMove;
    }else {
        return NSDragOperationNone;
    }
}

- (BOOL)tableView:(NSTableView *)tableView acceptDrop:(id<NSDraggingInfo>)info row:(NSInteger)row dropOperation:(NSTableViewDropOperation)dropOperation {

    NSPasteboard *p = info.draggingPasteboard;

    NSDictionary *pboardPList = [p propertyListForType:self.pasteboardItemIdentifier];
    NSNumber *rowIndexNum = pboardPList[@"sourceRowIndex"];
    NSInteger sourceRowIndex = -1;
    if (rowIndexNum) {
        sourceRowIndex = rowIndexNum.intValue;
    }
    else {
        return NO;
    }


    // move to new position
    if (sourceRowIndex == row || sourceRowIndex >= [self numberOfRowsInTableView:tableView] || sourceRowIndex < 0) {
        return NO;
    }

    // fetch and remove list item
    if (row > sourceRowIndex) {
        row--;
    }

    scriptableItem_t *node = scriptableItemChildAtIndex(self.scriptable, (unsigned int)sourceRowIndex);

    scriptableItemRemoveSubItem(self.scriptable, node);

    // reinsert the node at new position
    scriptableItemInsertSubItemAtIndex (self.scriptable, node, (unsigned int)row);

    [self.delegate scriptableItemDidChange:self.scriptable change:ScriptableItemChangeCreate];

    [tableView beginUpdates];
    [tableView moveRowAtIndex:sourceRowIndex toIndex:row];
    [tableView endUpdates];

    return YES;
}

@end
