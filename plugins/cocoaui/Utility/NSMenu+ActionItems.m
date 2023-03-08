//
//  NSMenu+ActionItems.m
//  DeaDBeeF
//
//  Created by Oleksiy Yakovenko on 2/23/20.
//  Copyright © 2020 Oleksiy Yakovenko. All rights reserved.
//

#include <deadbeef/deadbeef.h>
#import <AppKit/AppKit.h>
#import "DdbShared.h"
#import "NSMenu+ActionItems.h"
#import "PluginActionMenuItem.h"

extern DB_functions_t *deadbeef;

@implementation NSMenu (ActionItems)

- (void)pluginAction:(PluginActionMenuItem *)sender {
    sender.pluginAction->callback2 (sender.pluginAction, sender.pluginActionContext);
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
}

- (BOOL)addPluginActionItemsForSelectedTrack:(ddb_playItem_t *)selected selectedCount:(int)selectedCount actionContext:(ddb_action_context_t)actionContext {
    DB_plugin_t **plugins = deadbeef->plug_get_list();
    int i;
    int added_entries = 0;

    int hide_remove_from_disk = deadbeef->conf_get_int ("cocoaui.hide_remove_from_disk", 0);

    for (i = 0; plugins[i]; i++)
    {
        if (!plugins[i]->get_actions)
            continue;

        DB_plugin_action_t *actions = plugins[i]->get_actions (selected);
        DB_plugin_action_t *action;

        int count = 0;
        for (action = actions; action; action = action->next)
        {
            if (!strstr(action->title, "Duplicate")) {
                __unused int n = 0;
            }

            if (action->name && !strcmp (action->name, "delete_from_disk") && hide_remove_from_disk) {
                continue;
            }

            if (action->flags&DB_ACTION_DISABLED) {
                continue;
            }

            if (!((action->callback2 && (action->flags & DB_ACTION_ADD_MENU)) || action->callback)) {
                continue;
            }

            if (actionContext == DDB_ACTION_CTX_SELECTION) {
                if ((action->flags & DB_ACTION_COMMON)
                    || !(action->flags & (DB_ACTION_MULTIPLE_TRACKS | DB_ACTION_SINGLE_TRACK))) {
                    continue;
                }
            }

            if (actionContext == DDB_ACTION_CTX_PLAYLIST) {
                if (action->flags & DB_ACTION_EXCLUDE_FROM_CTX_PLAYLIST) {
                    continue;
                }
                if (action->flags & DB_ACTION_COMMON) {
                    continue;
                }
            }
            else if (actionContext == DDB_ACTION_CTX_MAIN) {
                if (!((action->flags & (DB_ACTION_COMMON|DB_ACTION_ADD_MENU)) == (DB_ACTION_COMMON|DB_ACTION_ADD_MENU))) {
                    continue;
                }
                const char *slash_test = action->title;
                while (NULL != (slash_test = strchr (slash_test, '/'))) {
                    if (slash_test && slash_test > action->title && *(slash_test-1) == '\\') {
                        slash_test++;
                        continue;
                    }
                    break;
                }

                if (slash_test == NULL) {
                    continue;
                }
            }

            // create submenus (separated with '/')
            const char *prev = action->title;
            while (*prev && *prev == '/') {
                prev++;
            }

            NSMenu *lastMenu = self;

            for (;;) {
                const char *slash = strchr (prev, '/');
                if (!(slash && *(slash-1) != '\\')) {
                    break; // found the leaf item
                }
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
                NSMenu *newMenu;

                // find menu item with the name
                for (NSMenuItem *item in lastMenu.itemArray) {
                    if ([item.title isEqualToString:[NSString stringWithUTF8String:name]]) {
                        if (item.menu == nil) {
                            item.submenu = [NSMenu new];
                        }
                        newMenu = item.submenu;
                        break;
                    }
                }

                // create
                if (!newMenu) {
                    NSMenuItem *item = [[NSMenuItem alloc] initWithTitle:[NSString stringWithUTF8String:name] action:@selector(pluginAction:) keyEquivalent:@""];
                    newMenu = [NSMenu new];
                    item.submenu = newMenu;
                    [lastMenu addItem:item];
                    item.target = self;
                }
                lastMenu = newMenu;
                prev = slash+1;
            }

            count++;
            added_entries++;

            if (actionContext == DDB_ACTION_CTX_MAIN && lastMenu == nil) {
                // Don't add leaf items into menubar
                continue;
            }

            // replace \/ with /
            const char *p = lastMenu ? prev : action->title;
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
            actionitem.pluginActionContext = actionContext;

            if (lastMenu != nil) {
                if (actionContext == DDB_ACTION_CTX_MAIN && [lastMenu.title isEqualToString:@"File"]) {
                    [lastMenu insertItem:actionitem atIndex:5];
                }
                else if (actionContext == DDB_ACTION_CTX_MAIN && [lastMenu.title isEqualToString:@"Edit"]) {
                    [lastMenu insertItem:actionitem atIndex:7];
                }
                else {
                    [lastMenu addItem:actionitem];
                }
            }
            else {
                [self addItem:actionitem];
            }
            int isPlaylistAction = (action->flags & DB_ACTION_PLAYLIST) && actionContext == DDB_ACTION_CTX_PLAYLIST;

            if (!isPlaylistAction) {
                if ((selectedCount > 1 && !(action->flags & DB_ACTION_MULTIPLE_TRACKS)) ||
                    (action->flags & DB_ACTION_DISABLED)) {
                    actionitem.enabled = NO;
                }
            }
        }
        if (count > 0 && deadbeef->conf_get_int ("cocoaui.action_separators", 0)) { // FIXME: UI
            [self addItem:NSMenuItem.separatorItem];
        }
    }

    return added_entries > 0;
}

@end
