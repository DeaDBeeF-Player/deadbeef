//
//  ScriptableErrorViewer.m
//  DeaDBeeF
//
//  Created by Oleksiy Yakovenko on 4/4/20.
//  Copyright © 2020 Oleksiy Yakovenko. All rights reserved.
//

#import "ScriptableErrorViewer.h"
#include "scriptable/scriptable.h"

@implementation ScriptableErrorViewer

+ (ScriptableErrorViewer *)sharedInstance {
    static ScriptableErrorViewer *instance;
    if (!instance) {
        instance = [ScriptableErrorViewer new];
    }
    return instance;
}

- (void)displayDuplicateNameError {
    NSAlert *alert = [NSAlert new];
    alert.messageText = @"Preset with this name already exists.";
    alert.informativeText = @"Try a different name.";
    alert.alertStyle = NSAlertStyleWarning;
    [alert addButtonWithTitle:@"OK"];

    [alert runModal];
}

- (void)displayInvalidNameError {
    NSAlert *alert = [NSAlert new];
    alert.messageText = @"This name is not allowed.";
    alert.informativeText = @"Try a different name.";
    alert.alertStyle = NSAlertStyleWarning;
    [alert addButtonWithTitle:@"OK"];

    [alert runModal];
}

#pragma mark - ScriptableErrorViewer

- (void)scriptableErrorViewer:(id)sender duplicateNameErrorForItem:(scriptableItem_t *)item {
    [self displayDuplicateNameError];
}

- (void)scriptableErrorViewer:(id)sender invalidNameErrorForItem:(scriptableItem_t *)item {
    [self displayInvalidNameError];
}

@end
