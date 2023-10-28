//
//  ScriptableProtocols.h
//  deadbeef
//
//  Created by Oleksiy Yakovenko on 3/29/20.
//  Copyright © 2020 Oleksiy Yakovenko. All rights reserved.
//

#import <Foundation/Foundation.h>
#include "scriptable/scriptable.h"

typedef NS_ENUM(NSUInteger, ScriptableItemChange) {
    ScriptableItemChangeCreate,
    ScriptableItemChangeUpdate,
    ScriptableItemChangeDelete,
};

@protocol ScriptableItemDelegate

- (void)scriptableItemDidChange:(scriptableItem_t *_Nonnull)scriptable change:(ScriptableItemChange)change;

@end

@protocol ScriptableErrorViewer
- (void)scriptableErrorViewer:(id _Nonnull)sender duplicateNameErrorForItem:(scriptableItem_t *_Nonnull)item;

- (void)scriptableErrorViewer:(id _Nonnull )sender invalidNameErrorForItem:(scriptableItem_t *_Nonnull)item;

@end
