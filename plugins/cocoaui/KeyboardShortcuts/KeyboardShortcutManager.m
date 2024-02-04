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

#import "KeyboardShortcutManager.h"
#import "keyboard_shortcuts.h"

@implementation KeyboardShortcutManager

- (void)dealloc {
    ddb_keyboard_shortcuts_deinit ();
}

- (instancetype)initWithMenu:(NSMenu *)menu {
    self = [super init];
    if (self == nil) {
        return nil;
    }

    ddb_keyboard_shortcut_t *root = ddb_keyboard_shortcuts_get_root ();

    [self traverseMenuItems:menu.itemArray parent:root];

    return self;
}

- (void)traverseMenuItems:(NSArray<NSMenuItem *> *)items parent:(ddb_keyboard_shortcut_t *)parent {
    for (NSMenuItem *item in items) {
        NSString *title = item.title;
        NSString *keyCombination = item.keyEquivalent;
        
        NSString *selectorString;
        SEL action = item.action;

        if (action != nil) {
            selectorString = NSStringFromSelector (action);
        }

        ddb_keyboard_shortcut_t *shortcut = ddb_keyboard_shortcut_append (parent);
        ddb_keyboard_shortcut_set_title (shortcut, title.UTF8String);
        ddb_keyboard_shortcut_set_selector (shortcut, selectorString.UTF8String);
        ddb_keyboard_shortcut_set_key_combination (shortcut, keyCombination.UTF8String);
        ddb_keyboard_shortcut_set_default_key_combination (shortcut, keyCombination.UTF8String);
    }
}

@end
