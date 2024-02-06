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

#import "KeyboardShortcutTextField.h"
#import "KeyboardShortcutConverter.h"

@interface KeyboardShortcutTextField()
@property (nonatomic) id eventMonitor;
@property (nonatomic, readwrite) NSString *key;
@property (nonatomic, readwrite) NSEventModifierFlags modifierFlags;
@end

@implementation KeyboardShortcutTextField

@dynamic delegate;

- (void)dealloc
{
    if (self.eventMonitor != nil) {
        [NSEvent removeMonitor:self.eventMonitor];
    }
}

- (nullable NSEvent *)handleKeyDownEvent:(nonnull NSEvent *)event {
    NSString *key = event.charactersIgnoringModifiers;
    NSEventModifierFlags modifierFlags = event.modifierFlags;
    if (key.length != 1) {
        self.stringValue = @"";
        return nil;
    }
    if (event.modifierFlags & NSEventModifierFlagShift) {
        key = [key lowercaseString];
        unichar uc = [key characterAtIndex:0];

        // uppercase letter: remove shift and lowercase the char
        if (uc < 128 && isupper(uc)) {
            char buf[] = { tolower(uc), 0 };
            key = [NSString stringWithUTF8String:buf];
            modifierFlags &= ~NSEventModifierFlagShift;
        }
        // any non-alpha character which was pressed with shift,
        // but doesn't require shift for assigning to menu item:
        // remove shift modifier
        else if (!isalpha(uc) && [key isEqualToString:event.characters])  {
            modifierFlags &= ~NSEventModifierFlagShift;
        }
    }

    self.key = key;
    self.modifierFlags = modifierFlags;
    NSString *displayString = [KeyboardShortcutConverter.shared keyCombinationDisplayStringFromKeyEquivalent:key modifierMask:modifierFlags];
    self.stringValue = displayString;
    [NSEvent removeMonitor:self.eventMonitor];
    self.eventMonitor = nil;
    [self.delegate textFieldDidAssignShortcut:self];
    return nil;
}

- (BOOL)becomeFirstResponder {
    BOOL result = [super becomeFirstResponder];
    if (!result) {
        return NO;
    }

    NSEventMask eventMask = (NSEventMaskKeyDown | NSEventMaskFlagsChanged);
    __weak KeyboardShortcutTextField *weakSelf = self;
    self.eventMonitor = [NSEvent addLocalMonitorForEventsMatchingMask:eventMask handler:^NSEvent * _Nullable(NSEvent * _Nonnull event) {
        KeyboardShortcutTextField *strongSelf = weakSelf;
        if (event.type != NSEventTypeKeyDown) {
            return event;
        }
        if (strongSelf == nil) {
            return event;
        }

        return [strongSelf handleKeyDownEvent:event];
    }];

    return YES;
}

@end
