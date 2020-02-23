//
//  NSMenu+ActionItems.m
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 2/23/20.
//  Copyright © 2020 Alexey Yakovenko. All rights reserved.
//

#import "NSMenu+ActionItems.h"
#import <AppKit/AppKit.h>
#import "PluginActionMenuItem.h"
#include "deadbeef.h"

extern DB_functions_t *deadbeef;

@implementation NSMenu (ActionItems)

- (void)pluginAction:(PluginActionMenuItem *)sender {
    sender.pluginAction->callback2 (sender.pluginAction, sender.pluginActionContext);
}

- (void)addActionItemsForContext:(ddb_action_context_t)context track:(nullable DB_playItem_t *)track filter:(BOOL(^)(DB_plugin_action_t *action))filter {
    DB_plugin_t **plugins = deadbeef->plug_get_list();
    int i;

    for (i = 0; plugins[i]; i++)
    {
        if (!plugins[i]->get_actions)
            continue;

        DB_plugin_action_t *actions = plugins[i]->get_actions (track);
        DB_plugin_action_t *action = NULL;

        for (action = actions; action; action = action->next)
        {
            char *tmp = NULL;

            if (action->flags&DB_ACTION_DISABLED) {
                continue;
            }

            int has_addmenu = filter(action);

            if (!has_addmenu)
                continue;

            // 1st check if we have slashes
            const char *slash_test = action->title;
            while (NULL != (slash_test = strchr (slash_test, '/'))) {
                if (slash_test && slash_test > action->title && *(slash_test-1) == '\\') {
                    slash_test++;
                    continue;
                }
                break;
            }
            if (!slash_test && ((action->flags&DB_ACTION_ADD_MENU) && context == DDB_ACTION_CTX_MAIN)) {
                continue;
            }

            tmp = strdup (action->title);
            const char *ptr = tmp;

            const char *prev_title = NULL;

            NSMenu *current = self;

            while (1) {
                // find unescaped forward slash
                char *slash = strchr (ptr, '/');
                if (slash && slash > ptr && *(slash-1) == '\\') {
                    ptr = slash + 1;
                    continue;
                }

                if (!slash) {
                    PluginActionMenuItem *actionitem = [[PluginActionMenuItem alloc] initWithTitle:[NSString stringWithUTF8String:ptr] action:@selector(pluginAction:) keyEquivalent:@""];
                    actionitem.pluginAction = action;
                    actionitem.pluginActionContext = context;
                    actionitem.target = self;

                    // Special cases for positioning in standard submenus
                    if (prev_title && !strcmp ("File", prev_title)) {
                        [current insertItem:actionitem atIndex:5];
                    }
                    else if (prev_title && !strcmp ("Edit", prev_title)) {
                        [current insertItem:actionitem atIndex:7];
                    }
                    else {
                        [current addItem:actionitem];
                    }

                    break;
                }
                *slash = 0;


                // get submenu
                NSMenu *previous = current;
                current = [current itemWithTitle:[NSString stringWithUTF8String:ptr]].submenu;
                if (!current) {
                    // create new item with submenu
                    NSMenuItem *newitem = [[NSMenuItem alloc] initWithTitle:[NSString stringWithUTF8String:ptr] action:nil keyEquivalent:@""];
                    newitem.submenu = [[NSMenu alloc] initWithTitle:[NSString stringWithUTF8String:ptr]];

                    // If we add new submenu in main bar, add it before 'Help'
                    if (NULL == prev_title) {
                        [previous insertItem:newitem atIndex:4];
                    }
                    else {
                        [previous addItem:newitem];
                    }

                    current = newitem.submenu;
                }
                prev_title = ptr;
                ptr = slash + 1;
            }
            if (tmp) {
                free (tmp);
            }
        }
    }
}

@end
