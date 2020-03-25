#ifndef ScriptableItemDelegate_h
#define ScriptableItemDelegate_h

#include "scriptable.h"

typedef NS_CLOSED_ENUM(NSUInteger, ScriptableItemChange) {
    ScriptableItemChangeCreate,
    ScriptableItemChangeUpdate,
    ScriptableItemChangeDelete,
};

@protocol ScriptableItemDelegate

- (void)scriptableItemChanged:(scriptableItem_t *)scriptable change:(ScriptableItemChange)change;

@end

#endif /* ScriptableItemDelegate_h */
