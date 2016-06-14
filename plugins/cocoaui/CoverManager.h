#import <Foundation/Foundation.h>

@interface CoverManager : NSObject

+ (CoverManager *)defaultCoverManager;
- (CoverManager *)init;
- (NSImage *)getTestCover;

@end
