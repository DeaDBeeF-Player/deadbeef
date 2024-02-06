/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2024 Oleksiy Yakovenko and other contributors

    This software is provided 'as-is', without any express or implied
    warranty.  In no event will the authors be held liable for any damages
    arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.

    3. This notice may not be removed or altered from any source distribution.
*/

#import <deadbeef/deadbeef.h>
#import "KeyboardShortcutConverter.h"
#import "KeyboardShortcutManager.h"
#import "KeyboardShortcutViewItem.h"
#import "keyboard_shortcuts.h"
#import "keyboard_shortcut_serializer.h"

extern DB_functions_t *deadbeef;

@interface KeyboardShortcutManager()

@property (nonatomic) NSMenu *menu;
@property (nonatomic) NSMutableDictionary<NSString *, NSMenuItem *> *mapActionToMenuItem;

@end

@implementation KeyboardShortcutManager

- (void)dealloc {
    ddb_keyboard_shortcuts_deinit ();
}

- (instancetype)initWithMenu:(NSMenu *)menu {
    self = [super init];
    if (self == nil) {
        return nil;
    }
    self.menu = menu;

    ddb_keyboard_shortcut_t *root = ddb_keyboard_shortcuts_get_root ();

    self.mapActionToMenuItem = [NSMutableDictionary new];

    [self traverseMenuItems:menu.itemArray parent:root];

    char *buffer = malloc (100000);
    deadbeef->conf_get_str("cocoaui.shortcuts", "", buffer, 100000);
    if (*buffer) {
        ddb_keyboard_shortcuts_load (root, buffer);
    }
    free (buffer);

    ddb_keyboard_shortcut_for_each_recursive(root, ^(ddb_keyboard_shortcut_t *shortcut) {
        const char *action = ddb_keyboard_shortcut_get_mac_action (shortcut);
        if (action == NULL) {
            return;
        }
        if (!ddb_keyboard_shortcut_is_modified (shortcut)) {
            return;
        }
        [self applyShortcut:shortcut];
    });

    return self;
}

- (void)traverseMenuItems:(NSArray<NSMenuItem *> *)items parent:(ddb_keyboard_shortcut_t *)parent {
    for (NSMenuItem *item in items) {
        NSString *title = item.title;
        NSString *keyCombination;

        NSString *selectorString;
        SEL action = item.action;

        if (action == nil && item.submenu == nil) {
            continue;
        }

        if (action != nil && item.submenu == nil) {
            keyCombination = [KeyboardShortcutConverter.shared keyCombinationDisplayStringFromKeyEquivalent:item.keyEquivalent modifierMask:item.keyEquivalentModifierMask];
        }

        if (item.submenu == nil && action != nil) {
            selectorString = NSStringFromSelector (action);
        }

        ddb_keyboard_shortcut_t *shortcut = ddb_keyboard_shortcut_append (parent);
        ddb_keyboard_shortcut_set_title (shortcut, title.UTF8String);
        if (selectorString != NULL) {
            ddb_keyboard_shortcut_set_mac_action (shortcut, selectorString.UTF8String);
            ddb_keyboard_shortcut_set_key_character (shortcut, item.keyEquivalent.UTF8String);
            ddb_keyboard_shortcut_modifiers_t modifiers = [KeyboardShortcutConverter.shared ddbModifiersFromAppKitModifiers:item.keyEquivalentModifierMask];
            ddb_keyboard_shortcut_set_key_modifiers (shortcut, modifiers);
            ddb_keyboard_shortcut_set_default_key_character (shortcut, item.keyEquivalent.UTF8String);
            ddb_keyboard_shortcut_set_default_key_modifiers (shortcut, modifiers);

            self.mapActionToMenuItem[selectorString] = item;
        }

        if (item.submenu != nil) {
            [self traverseMenuItems:item.submenu.itemArray parent:shortcut];
        }
    }
}

- (KeyboardShortcutViewItem *)createViewItems {
    KeyboardShortcutViewItem *viewItemRoot = [KeyboardShortcutViewItem new];
    ddb_keyboard_shortcut_t *root = ddb_keyboard_shortcuts_get_root ();

    [self createViewItemHierarchy:viewItemRoot shortcut:root];

    // flatten the hierarchy to have only 2 levels
    for (KeyboardShortcutViewItem *topLevelItem in viewItemRoot.children) {
        NSMutableArray *children = [NSMutableArray new];
        BOOL haveMoreDepth = NO;
        do {
            for (KeyboardShortcutViewItem *childViewItem in topLevelItem.children) {
                if (childViewItem.children == nil) {
                    if (NULL == ddb_keyboard_shortcut_get_mac_action(childViewItem.shortcut)) {
                        continue; // skip empty submenu item
                    }
                    [children addObject:childViewItem];
                }
                else {
                    for (KeyboardShortcutViewItem *subViewItem in childViewItem.children) {
                        subViewItem.displayText = [childViewItem.displayText stringByAppendingString:[@" ▸ " stringByAppendingString:subViewItem.displayText]];
                        [children addObject:subViewItem];
                        if (subViewItem.children != nil) {
                            haveMoreDepth = YES;
                        }
                    }
                }
            }
        } while (haveMoreDepth);
        topLevelItem.children = children.copy;
    }

    return viewItemRoot;
}

- (void)createViewItemHierarchy:(KeyboardShortcutViewItem *)viewItem shortcut:(ddb_keyboard_shortcut_t *)shortcut {
    viewItem.shortcut = shortcut;
    viewItem.displayText = @(ddb_keyboard_shortcut_get_title(shortcut) ?: "");
    NSMutableArray<KeyboardShortcutViewItem *> *children = [NSMutableArray new];

    ddb_keyboard_shortcut_t *child = ddb_keyboard_shortcut_get_children (shortcut);
    while (child != NULL) {
        KeyboardShortcutViewItem *childViewItem = [KeyboardShortcutViewItem new];
        [self createViewItemHierarchy:childViewItem shortcut:child];
        [children addObject:childViewItem];
        child = ddb_keyboard_shortcut_get_next (child);
    }

    if (children.count != 0) {
        viewItem.children = children.copy;
    }
}

- (void)findMenuItems:(NSArray<NSMenuItem *> *)menuItems matchingTitle:(NSString *)title action:(SEL)action performBlock:(void (^)(NSMenuItem *menuItem))block {
    for (NSMenuItem *item in menuItems) {
        if (item.submenu != nil) {
            [self findMenuItems:item.submenu.itemArray matchingTitle:title action:action performBlock:block];
        }
        else {
            if ([item.title isEqualToString:title] && item.action == action) {
                block(item);
            }
        }
    }
}

- (void)applyShortcut:(nonnull ddb_keyboard_shortcut_t *)shortcut {
    const char *action = ddb_keyboard_shortcut_get_mac_action (shortcut);

    if (action == NULL) {
        return;
    }

    NSMenuItem *menuItem = self.mapActionToMenuItem[@(action)];
    if (menuItem == nil) {
        return;
    }

    NSString *keyEquivalent = @(ddb_keyboard_shortcut_get_key_character(shortcut) ?: "");
    NSEventModifierFlags modifiers = [KeyboardShortcutConverter.shared appKitModifiersFromDdbModifiers:ddb_keyboard_shortcut_get_key_modifiers(shortcut)];

    menuItem.keyEquivalent = keyEquivalent;
    menuItem.keyEquivalentModifierMask = modifiers;

    char *jsonString = ddb_keyboard_shortcuts_save (ddb_keyboard_shortcuts_get_root ());
    if (jsonString != NULL) {
        deadbeef->conf_set_str ("cocoaui.shortcuts", jsonString);
        deadbeef->conf_save();
        free (jsonString);
    }
}

@end
