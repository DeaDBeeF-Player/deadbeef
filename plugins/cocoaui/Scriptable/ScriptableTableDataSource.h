#import <Foundation/Foundation.h>
#include "deadbeef.h"
#include "../../../scriptable/scriptable.h"
#import "ScriptableProtocols.h"

@interface ScriptableTableDataSource : NSObject<NSTableViewDataSource>

+ (ScriptableTableDataSource *)dataSourceWithScriptable:(scriptableItem_t *)scriptable;

@property (nonatomic,readonly) NSString *pasteboardItemIdentifier;
@property (nonatomic) scriptableItem_t *scriptable;
@property (weak) NSObject<ScriptableItemDelegate> *delegate;

- (ScriptableTableDataSource *)initWithScriptable:(scriptableItem_t *)scriptable NS_DESIGNATED_INITIALIZER;
- (void)insertItem:(scriptableItem_t *)item atIndex:(NSInteger)index;
- (void)removeItemAtIndex:(NSInteger)index;
- (scriptableItem_t *)itemAtIndex:(NSInteger)index;

@end

