#ifndef ScriptableItemDelegate_h
#define ScriptableItemDelegate_h

#include "scriptable.h"

@protocol ScriptableItemDelegate

@optional
- (void)scriptableItemChanged:(scriptableItem_t *)scriptable;

@end

#endif /* ScriptableItemDelegate_h */
