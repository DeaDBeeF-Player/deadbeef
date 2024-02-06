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

#import "KeyboardShortcutConverter.h"

@implementation KeyboardShortcutConverter

+ (KeyboardShortcutConverter *)shared {
    static KeyboardShortcutConverter *instance;
    if (instance == nil) {
        instance = [KeyboardShortcutConverter new];
    }
    return instance;
}

- (NSString *)keyCombinationDisplayStringFromKeyEquivalent:(NSString *)keyEquivalent modifierMask:(NSEventModifierFlags)modifierMask {
    if (keyEquivalent == nil || [keyEquivalent isEqualToString:@""]) {
        return nil;
    }

    NSString *result = keyEquivalent.uppercaseString;

    unichar uc = [result characterAtIndex:0];

    if (uc == 0x7f) {
        result = @"⌫";
    }
    if (uc == 63272) {
        result = @"⌦";
        modifierMask &= ~NSEventModifierFlagFunction;
    }
    else if (uc == 0x1b) {
        result = @"⎋";
    }
    else if (uc == 0x0d) {
        result = @"⏎";
    }
    else if (uc == 63276) {
        result = @"⇞"; // pgup
        modifierMask &= ~NSEventModifierFlagFunction;
    }
    else if (uc == 63277) {
        result = @"⇟"; // pgdn
        modifierMask &= ~NSEventModifierFlagFunction;
    }
    else if (uc == 63273) {
        result = @"⇱"; // home
        modifierMask &= ~NSEventModifierFlagFunction;
    }
    else if (uc == 63275) {
        result = @"⇲"; // end
        modifierMask &= ~NSEventModifierFlagFunction;
    }
    else if (uc == 63232) {
        result = @"↑"; // up arrow
        modifierMask &= ~NSEventModifierFlagFunction;
    }
    else if (uc == 63233) {
        result = @"↓"; // down arrow
        modifierMask &= ~NSEventModifierFlagFunction;
    }
    else if (uc == 63234) {
        result = @"←"; // left arrow
        modifierMask &= ~NSEventModifierFlagFunction;
    }
    else if (uc == 63235) {
        result = @"→"; // right arrow
        modifierMask &= ~NSEventModifierFlagFunction;
    }
    else if (uc == 0x2327) {
        result = @"⌧"; // clear
    }
    else if (uc == 0x09) {
        result = @"⇥"; // tab
    }
    else if (uc == 0x19) {
        result = @"⇤"; // backtab
    }
    else if (uc == 0x2423 || uc == 0x20) {
        result = @"␣"; // space
    }
    else if (uc >= 63236 && uc <= 63247) {
        result = [NSString stringWithFormat:@"F%d", uc-63235];
        modifierMask &= ~NSEventModifierFlagFunction;
    }

    if (modifierMask & NSEventModifierFlagCommand) {
        result = [@"⌘" stringByAppendingString:result];
    }

    if (modifierMask & NSEventModifierFlagShift) {
        result = [@"⇧" stringByAppendingString:result];
    }

    if (modifierMask & NSEventModifierFlagOption) {
        result = [@"⌥" stringByAppendingString:result];
    }

    if (modifierMask & NSEventModifierFlagControl) {
        result = [@"⌃" stringByAppendingString:result];
    }

    if (modifierMask & NSEventModifierFlagFunction) {
        result = [@"🌐" stringByAppendingString:result];
    }

    return result;
}

- (ddb_keyboard_shortcut_modifiers_t)ddbModifiersFromAppKitModifiers:(NSEventModifierFlags)modifierMask {
    ddb_keyboard_shortcut_modifiers_t result = 0;

    if (modifierMask & NSEventModifierFlagCommand) {
        result |= ddb_keyboard_shortcut_modifiers_super;
    }

    if (modifierMask & NSEventModifierFlagShift) {
        result |= ddb_keyboard_shortcut_modifiers_shift;
    }

    if (modifierMask & NSEventModifierFlagOption) {
        result |= ddb_keyboard_shortcut_modifiers_option;
    }

    if (modifierMask & NSEventModifierFlagControl) {
        result |= ddb_keyboard_shortcut_modifiers_control;
    }

    if (modifierMask & NSEventModifierFlagFunction) {
        result |= ddb_keyboard_shortcut_modifiers_fn;
    }

    return result;
}

- (NSEventModifierFlags)appKitModifiersFromDdbModifiers:(ddb_keyboard_shortcut_modifiers_t)modifiers {
    NSEventModifierFlags result = 0;

    if (modifiers & ddb_keyboard_shortcut_modifiers_super) {
        result |= NSEventModifierFlagCommand;
    }

    if (modifiers & ddb_keyboard_shortcut_modifiers_shift) {
        result |= NSEventModifierFlagShift;
    }

    if (modifiers & ddb_keyboard_shortcut_modifiers_option) {
        result |= NSEventModifierFlagOption;
    }

    if (modifiers & ddb_keyboard_shortcut_modifiers_control) {
        result |= NSEventModifierFlagControl;
    }

    if (modifiers & ddb_keyboard_shortcut_modifiers_fn) {
        result |= NSEventModifierFlagFunction;
    }

    return result;
}

@end
