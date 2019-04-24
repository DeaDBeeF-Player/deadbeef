#import "ScriptableManagerTableDataSource.h"
#include "deadbeef.h"
#include "../../scriptable/scriptable.h"

extern DB_functions_t *deadbeef;

// FIXME: implement Save in scriptableItem, instead of directly calling streamer_set_dsp_chain
@implementation ScriptableManagerTableDataSource {
    scriptableItem_t *_chain;
}

- (void)dealloc {
    if (_chain) {
        scriptableItemFree (_chain);
        _chain = NULL;
    }
}

- (ScriptableManagerTableDataSource *)initWithChain:(scriptableItem_t *)chain pasteboardItemIdentifier:(NSString *)identifier {
    self = [super init];
    self.pasteboardItemIdentifier = identifier;

    _chain = chain;
    return self;
}

- (void)insertItem:(scriptableItem_t *)item atIndex:(NSInteger)index {
    scriptableItemInsertSubItemAtIndex(_chain, item, (unsigned int)index);
    // FIXME    deadbeef->streamer_set_dsp_chain (_chain);
}

- (void)removeItemAtIndex:(NSInteger)index {
    scriptableItem_t *item = scriptableItemChildAtIndex(_chain, (unsigned int)index);
    if (item) {
        scriptableItemRemoveSubItem(_chain, item);
        scriptableItemFree (item);
    }
    // FIXME    deadbeef->streamer_set_dsp_chain (_chain);
}

- (scriptableItem_t *)itemAtIndex:(NSInteger)index {
    return scriptableItemChildAtIndex(_chain, (unsigned int)index);
}

- (void)apply {
    // FIXME    deadbeef->streamer_set_dsp_chain (_chain);
}


#pragma mark - NSTableViewDataSource
- (NSInteger)numberOfRowsInTableView:(NSTableView *)tableView {
    return scriptableItemNumChildren (_chain);
}

- (id)tableView:(NSTableView *)tableView objectValueForTableColumn:(NSTableColumn *)tableColumn row:(NSInteger)row {
    scriptableItem_t *node = scriptableItemChildAtIndex(_chain, (unsigned int)row);
    const char *name = scriptableItemPropertyValueForKey(node, "name");
    return [NSString stringWithUTF8String:name];
}

#pragma mark - Drag & Drop

- (id <NSPasteboardWriting>)tableView:(NSTableView *)tableView pasteboardWriterForRow:(NSInteger)row {
    NSString *identifier = [NSString stringWithFormat:@"%d", (int)row];

    NSPasteboardItem *pboardItem = [[NSPasteboardItem alloc] init];
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

    scriptableItem_t *node = scriptableItemChildAtIndex(_chain, (unsigned int)sourceRow);
    scriptableItemRemoveSubItem(_chain, node);


    // reinsert the node at new position
    scriptableItemInsertSubItemAtIndex (_chain, node, (unsigned int)row);

    [self apply];

    return YES;
}

@end
