#import <Foundation/Foundation.h>
#include "../../deadbeef.h"

@interface CoverManager : NSObject

+ (CoverManager *)defaultCoverManager;
- (CoverManager *)init;
- (NSImage *)getTestCover;
- (NSImage *)getCoverForTrack:(DB_playItem_t *)track withCallbackWhenReady:(void (*) (NSImage *img, void *user_data))callback withUserDataForCallback:(void *)user_data;

@end
