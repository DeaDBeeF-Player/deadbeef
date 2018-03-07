#import <Foundation/Foundation.h>
#include "../../deadbeef.h"

@interface DSPChainDataSource : NSObject

- (DSPChainDataSource *)initWithChain:(ddb_dsp_context_t *)chain;
- (void)addItem:(DB_dsp_t *)plugin;
- (void)removeItemAtIndex:(int)index;
- (ddb_dsp_context_t *)getItemAtIndex:(int)index;
- (void)apply;
@end
