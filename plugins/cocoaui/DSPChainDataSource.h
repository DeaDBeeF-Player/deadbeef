#import <Foundation/Foundation.h>
#include "../../deadbeef.h"

@interface DSPChainDataSource : NSObject

- (DSPChainDataSource *)initWithChain:(ddb_dsp_context_t *)chain;

@end
