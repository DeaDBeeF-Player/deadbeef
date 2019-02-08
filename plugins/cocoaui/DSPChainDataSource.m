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

- (DSPChainDataSource *)initWithChain:(ddb_dsp_context_t *)chain {
    self = [super init];

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

@end
