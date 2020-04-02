//
//  ScriptableProtocols.h
//  deadbeef
//
//  Created by Alexey Yakovenko on 3/29/20.
//  Copyright © 2020 Alexey Yakovenko. All rights reserved.
//


#include "scriptable.h"

typedef NS_CLOSED_ENUM(NSUInteger, ScriptableItemChange) {
    ScriptableItemChangeCreate,
    ScriptableItemChangeUpdate,
    ScriptableItemChangeDelete,
};

@protocol ScriptableItemDelegate

- (void)scriptableItemChanged:(scriptableItem_t *_Nonnull)scriptable change:(ScriptableItemChange)change;

@end

@protocol ScriptableErrorViewer
- (void)scriptableErrorViewer:(id _Nonnull)sender duplicateNameErrorForItem:(scriptableItem_t *_Nonnull)item;

- (void)scriptableErrorViewer:(id _Nonnull )sender invalidNameErrorForItem:(scriptableItem_t *_Nonnull)item;

@end
