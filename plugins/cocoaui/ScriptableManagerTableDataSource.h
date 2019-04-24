#import <Foundation/Foundation.h>
#include "../../deadbeef.h"
#include "scriptable_dsp.h"

@interface ScriptableManagerTableDataSource : NSObject<NSTableViewDataSource>
@property NSString *pasteboardItemIdentifier;

- (ScriptableManagerTableDataSource *)initWithChain:(scriptableItem_t *)dspChain pasteboardItemIdentifier:(NSString *)identifier;
- (void)insertItem:(scriptableItem_t *)item atIndex:(NSInteger)index;
- (void)removeItemAtIndex:(NSInteger)index;
- (scriptableItem_t *)itemAtIndex:(NSInteger)index;
- (void)apply;
@end
