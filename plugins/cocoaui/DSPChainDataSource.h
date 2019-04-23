#import <Foundation/Foundation.h>
#include "../../deadbeef.h"
#include "scriptable_dsp.h"

@interface DSPChainDataSource : NSObject<NSTableViewDataSource>
@property NSString *dspNodeDraggedItemType;

- (DSPChainDataSource *)initWithChain:(scriptableItem_t *)dspChain domain:(NSString *)domain;
- (void)insertItem:(scriptableItem_t *)item atIndex:(NSInteger)index;
- (void)removeItemAtIndex:(NSInteger)index;
- (scriptableItem_t *)itemAtIndex:(NSInteger)index;
- (void)apply;
@end
