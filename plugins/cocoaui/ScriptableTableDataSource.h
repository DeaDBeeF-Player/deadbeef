#import <Foundation/Foundation.h>
#include "../../deadbeef.h"
#include "../../scriptable/scriptable.h"
#import "ScriptableItemDelegate.h"

@interface ScriptableTableDataSource : NSObject<NSTableViewDataSource>
@property NSString *pasteboardItemIdentifier;
@property (nonatomic) scriptableItem_t *scriptable;
@property (weak) NSObject<ScriptableItemDelegate> *delegate;

- (ScriptableTableDataSource *)initWithScriptable:(scriptableItem_t *)scriptable pasteboardItemIdentifier:(NSString *)identifier;
- (void)insertItem:(scriptableItem_t *)item atIndex:(NSInteger)index;
- (void)removeItemAtIndex:(NSInteger)index;
- (scriptableItem_t *)itemAtIndex:(NSInteger)index;
@end
