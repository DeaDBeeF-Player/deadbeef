#import "DSPChainDataSource.h"
#include "../../deadbeef.h"

extern DB_functions_t *deadbeef;

@implementation DSPChainDataSource {
    ddb_dsp_context_t *_chain;
}

static ddb_dsp_context_t *
dsp_clone (ddb_dsp_context_t *from) {
    ddb_dsp_context_t *dsp = from->plugin->open ();
    char param[2000];
    if (from->plugin->num_params) {
        int n = from->plugin->num_params ();
        for (int i = 0; i < n; i++) {
            from->plugin->get_param (from, i, param, sizeof (param));
            dsp->plugin->set_param (dsp, i, param);
        }
    }
    dsp->enabled = from->enabled;
    return dsp;
}

static ddb_dsp_context_t *
dsp_chain_clone (ddb_dsp_context_t *source_chain) {
    ddb_dsp_context_t *chain = NULL;
    ddb_dsp_context_t *tail = NULL;
    while (source_chain) {
        ddb_dsp_context_t *new = dsp_clone (source_chain);
        if (tail) {
            tail->next = new;
            tail = new;
        }
        else {
            chain = tail = new;
        }
        source_chain = source_chain->next;
    }
    return chain;
}

- (void)dealloc {
    deadbeef->dsp_preset_free (_chain);
    _chain = NULL;
}

- (DSPChainDataSource *)initWithChain:(ddb_dsp_context_t *)chain domain:(NSString *)domain {
    self = [super init];
    self.dspNodeDraggedItemType = [@"deadbeef.dspnode." stringByAppendingString:domain];

    _chain = dsp_chain_clone(chain);

    return self;
}

// NSTableViewDataSource
- (NSInteger)numberOfRowsInTableView:(NSTableView *)tableView {
    int n = 0;
    ddb_dsp_context_t *chain = _chain;
    while (chain) {
        n++;
        chain = chain->next;
    }
    return n;
}

- (id)tableView:(NSTableView *)tableView objectValueForTableColumn:(NSTableColumn *)tableColumn row:(NSInteger)row {
    int n = 0;
    ddb_dsp_context_t *node = _chain;
    while (n < row && node) {
        n++;
        node = node->next;
    }
    if (!node) {
        return nil;
    }
    return [NSString stringWithUTF8String:node->plugin->plugin.name];
}

- (void)addItem:(DB_dsp_t *)plugin atIndex:(NSInteger)index {
    ddb_dsp_context_t *node = _chain;
    ddb_dsp_context_t *tail = NULL;
    while (node && --index >= 0) {
        tail = node;
        node = node->next;
    }

    ddb_dsp_context_t *inst = plugin->open ();

    if (tail) {
        tail->next = inst;
    }
    else {
        _chain = inst;
    }
    inst->next = node;
    deadbeef->streamer_set_dsp_chain (_chain);
}

- (void)removeItemAtIndex:(NSInteger)index {
    ddb_dsp_context_t *node = _chain;
    ddb_dsp_context_t *prev = NULL;
    while (index > 0 && node) {
        prev = node;
        node = node->next;
        index--;
    }

    if (node) {
        if (prev) {
            prev->next = node->next;
        }
        else {
            _chain = node->next;
        }

        node->plugin->close (node);
    }
    deadbeef->streamer_set_dsp_chain (_chain);
}

- (ddb_dsp_context_t *)getItemAtIndex:(NSInteger)index {
    ddb_dsp_context_t *node = _chain;
    while (index > 0 && node) {
        node = node->next;
        index--;
    }
    return node;
}

- (void)apply {
    deadbeef->streamer_set_dsp_chain (_chain);
}

#pragma mark - Drag & Drop

- (id <NSPasteboardWriting>)tableView:(NSTableView *)tableView pasteboardWriterForRow:(NSInteger)row {
    NSString *identifier = [NSString stringWithFormat:@"%d", (int)row];

    NSPasteboardItem *pboardItem = [[NSPasteboardItem alloc] init];
    [pboardItem setString:identifier forType: self.dspNodeDraggedItemType];

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
    NSInteger sourceRow = [[p stringForType:self.dspNodeDraggedItemType] intValue];

    if (sourceRow == row || sourceRow >= [self numberOfRowsInTableView:tableView] || sourceRow < 0) {
        return NO;
    }

    // fetch and remove list item
    if (row > sourceRow) {
        row--;
    }
    ddb_dsp_context_t *node = _chain;
    ddb_dsp_context_t *prev = NULL;
    while (--sourceRow >= 0 && node) {
        prev = node;
        node = node->next;
    }

    if (prev) {
        prev->next = node->next;
    }
    else {
        _chain = node->next;
    }
    node->next = NULL;

    // reinsert the node at new position
    prev = NULL;
    ddb_dsp_context_t *at = _chain;
    while (--row >= 0 && at) {
        prev = at;
        at = at->next;
    }
    if (prev) {
        prev->next = node;
        node->next = at;
    }
    else {
        node->next = _chain;
        _chain = node;
    }

    [self apply];

    return YES;
}

@end
