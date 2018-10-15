#import <Foundation/Foundation.h>
#import "deadbeef-Swift.h"
#include "../../deadbeef.h"
#include "plugins.h"
#include "dsp.h"
#include "dsppreset.h"
#include "messagepump.h"
#include "streamer.h"

void
util_dsp_preset_save (DSPPreset *preset) {
    NSArray<id<Scriptable>> *items = [preset getItems];

    if (preset.isCurrent) {
        // check if interactive update is possible, without resetting the whole thing
        // NOTE: this is completely non-threadsafe!
        ddb_dsp_context_t *c = NULL;
        ddb_dsp_context_t *node = c = streamer_get_dsp_chain ();

        BOOL canReuseChain = YES;
        for (id<Scriptable> item in items) {
            const char *type = [[item getType] UTF8String];

            if (strcmp (node->plugin->plugin.id, type)) {
                canReuseChain = NO;
                break;
            }
            node = node->next;
        }

        if (canReuseChain) {
            // apply to current chain
            node = c;

            for (id<Scriptable> item in items) {
                DB_dsp_t *plug = node->plugin;
                int enabled = [item getEnabled];
                if (plug->num_params) {
                    for (id<Scriptable> param in [item getItems]) {
                        int index = [[param getName] intValue];
                        NSString *value = [param getValue];
                        if (index >= 0 && index < plug->num_params()) {
                            plug->set_param (node, index, [value UTF8String]);
                        }
                    }
                }
                node->enabled = enabled;

                node = node->next;
            }

            messagepump_push (DB_EV_DSPCHAINCHANGED, 0, 0, 0);
            return;
        }
    }

    ddb_dsp_context_t *chain = NULL;
    ddb_dsp_context_t *tail = NULL;
    for (id<Scriptable> item in items) {
        int enabled = [item getEnabled];

        const char *type = [[item getType] UTF8String];
        DB_dsp_t *plug = (DB_dsp_t *)plug_get_for_id (type);
        if (!plug) {
            fprintf (stderr, "streamer_dsp_chain_load: plugin %s not found. preset will not be loaded\n", type);
            continue;
        }
        ddb_dsp_context_t *ctx = plug->open ();
        if (!ctx) {
            fprintf (stderr, "streamer_dsp_chain_load: failed to open instance of plugin %s\n", type);
            continue;
        }

        if (tail) {
            tail->next = ctx;
            tail = ctx;
        }
        else {
            tail = chain = ctx;
        }

        if (plug->num_params) {
            for (id<Scriptable> param in [item getItems]) {
                int index = [[param getName] intValue];
                NSString *value = [param getValue];
                if (index >= 0 && index < plug->num_params()) {
                    plug->set_param (ctx, index, [value UTF8String]);
                }
            }
        }
        ctx->enabled = enabled;
    }

    if (preset.isCurrent) {
        streamer_set_dsp_chain (chain);
    }
    else {
        dsp_preset_save ([preset.savePath UTF8String], chain);
    }
    dsp_chain_free (chain);
    chain = NULL;
}
