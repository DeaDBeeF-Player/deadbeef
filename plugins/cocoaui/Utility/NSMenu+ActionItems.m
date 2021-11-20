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
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
}

- (void)addActionItemsForContext:(ddb_action_context_t)context track:(nullable DB_playItem_t *)track filter:(BOOL(^)(DB_plugin_action_t *action))filter {
    DB_plugin_t **plugins = deadbeef->plug_get_list();
    int i;

    int hide_remove_from_disk = deadbeef->conf_get_int ("cocoaui.hide_remove_from_disk", 0);

    for (i = 0; plugins[i]; i++)
    {
        if (!plugins[i]->get_actions)
            continue;

        DB_plugin_action_t *actions = plugins[i]->get_actions (track);
        DB_plugin_action_t *action = NULL;

        for (action = actions; action; action = action->next)
        {
            char *tmp = NULL;

            if (action->name && !strcmp (action->name, "delete_from_disk") && hide_remove_from_disk) {
                continue;
            }
            if (action->flags&DB_ACTION_DISABLED) {
                continue;
            }

            BOOL has_addmenu = YES;
            if (filter != nil) {
                has_addmenu = filter(action);
            }

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
                    if (prev_title && !strcmp ("File", prev_title) && context == DDB_ACTION_CTX_MAIN) {
                        [current insertItem:actionitem atIndex:5];
                    }
                    else if (prev_title && !strcmp ("Edit", prev_title) && context == DDB_ACTION_CTX_MAIN) {
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
                    if (NULL == prev_title && context == DDB_ACTION_CTX_MAIN) {
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

- (BOOL)addContextPluginActionItemsForSelectedTrack:(ddb_playItem_t *)selected selectedCount:(int)selectedCount {
    DB_plugin_t **plugins = deadbeef->plug_get_list();
    int i;
    int added_entries = 0;
    for (i = 0; plugins[i]; i++)
    {
        if (!plugins[i]->get_actions)
            continue;

        DB_plugin_action_t *actions = plugins[i]->get_actions (selected);
        DB_plugin_action_t *action;

        int count = 0;
        for (action = actions; action; action = action->next)
        {
            if ((action->flags & DB_ACTION_COMMON) || !((action->callback2 && (action->flags & DB_ACTION_ADD_MENU)) || action->callback) || !(action->flags & (DB_ACTION_MULTIPLE_TRACKS | DB_ACTION_SINGLE_TRACK)))
                continue;

            // create submenus (separated with '/')
            const char *prev = action->title;
            while (*prev && *prev == '/') {
                prev++;
            }

            NSMenu *popup = NULL;

            for (;;) {
                const char *slash = strchr (prev, '/');
                if (slash && *(slash-1) != '\\') {
                    char name[slash-prev+1];
                    // replace \/ with /
                    const char *p = prev;
                    char *t = name;
                    while (*p && p < slash) {
                        if (*p == '\\' && *(p+1) == '/') {
                            *t++ = '/';
                            p += 2;
                        }
                        else {
                            *t++ = *p++;
                        }
                    }
                    *t = 0;

                    // add popup
                    NSMenu *prev_menu = popup ? popup : self;

                    // find menu item with the name
                    for (NSMenuItem *item in prev_menu.itemArray) {
                        if ([item.title isEqualToString:[NSString stringWithUTF8String:name]]) {
                            if (item.menu == nil) {
                                item.submenu = [NSMenu new];
                            }
                            popup = item.submenu;
                            break;
                        }
                    }
                    if (!popup) {
                        NSMenuItem *item = [[NSMenuItem alloc] initWithTitle:[NSString stringWithUTF8String:name] action:@selector(pluginAction:) keyEquivalent:@""];
                        item.submenu = [NSMenu new];
                        if (prev_menu != self) {
                            [prev_menu addItem:item];
                        }
                        else {
                            [prev_menu addItem:item];
                        }
                        item.target = self;
                        item.submenu = [NSMenu new];
                        popup = item.submenu;
                    }
                }
                else {
                    break;
                }
                prev = slash+1;
            }


            count++;
            added_entries++;

            // replace \/ with /
            const char *p = popup ? prev : action->title;
            char title[strlen (p)+1];
            char *t = title;
            while (*p) {
                if (*p == '\\' && *(p+1) == '/') {
                    *t++ = '/';
                    p += 2;
                }
                else {
                    *t++ = *p++;
                }
            }
            *t = 0;

            PluginActionMenuItem *actionitem = [[PluginActionMenuItem alloc] initWithTitle:[NSString stringWithUTF8String:title] action:@selector(pluginAction:) keyEquivalent:@""];
            actionitem.target = self;
            actionitem.pluginAction = action;
            actionitem.pluginActionContext = DDB_ACTION_CTX_SELECTION;

            if (popup != nil) {
                [popup addItem:actionitem];
            }
            else {
                [self addItem:actionitem];
            }
            if ((selectedCount > 1 && !(action->flags & DB_ACTION_MULTIPLE_TRACKS)) ||
                (action->flags & DB_ACTION_DISABLED)) {
                actionitem.enabled = NO;
            }
        }
        if (count > 0 && deadbeef->conf_get_int ("cocoaui.action_separators", 0)) { // FIXME: UI
            [self addItem:NSMenuItem.separatorItem];
        }
    }

    return added_entries > 0;
}

@end
