#import <Foundation/Foundation.h>
#include <deadbeef/deadbeef.h>
#include "scriptable/scriptable.h"
#import "ScriptableProtocols.h"

NS_ASSUME_NONNULL_BEGIN

@interface ScriptableTableDataSource : NSObject<NSTableViewDataSource>

+ (ScriptableTableDataSource *)dataSourceWithScriptable:(scriptableItem_t *)scriptable;

@property (nonatomic,readonly,nullable) NSString *pasteboardItemIdentifier;
@property (nonatomic,nullable) scriptableItem_t *scriptable;
@property (weak) NSObject<ScriptableItemDelegate> *delegate;

- (ScriptableTableDataSource *)initWithScriptable:(scriptableItem_t * _Nullable)scriptable NS_DESIGNATED_INITIALIZER;
- (void)insertItem:(scriptableItem_t *)item atIndex:(NSInteger)index;
- (void)removeItemAtIndex:(NSInteger)index;
- (void)duplicateItem:(scriptableItem_t *)item atIndex:(NSInteger)index;
- (scriptableItem_t *)itemAtIndex:(NSInteger)index;

@end

NS_ASSUME_NONNULL_END
