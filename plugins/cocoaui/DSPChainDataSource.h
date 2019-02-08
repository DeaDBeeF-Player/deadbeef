#import <Foundation/Foundation.h>
#include "../../deadbeef.h"

@interface DSPChainDataSource : NSObject<NSTableViewDataSource>

- (DSPChainDataSource *)initWithChain:(ddb_dsp_context_t *)chain;
- (void)addItem:(DB_dsp_t *)plugin atIndex:(NSInteger)index;
- (void)removeItemAtIndex:(NSInteger)index;
- (ddb_dsp_context_t *)getItemAtIndex:(NSInteger)index;
- (void)apply;
@end
