#import "CoverManager.h"

static CoverManager *g_DefaultCoverManager = nil;

@implementation CoverManager {
    NSImage *_testCover;
}

+ (CoverManager *)defaultCoverManager {
    if (!g_DefaultCoverManager) {
        g_DefaultCoverManager = [[CoverManager alloc] init];
    }
    return g_DefaultCoverManager;
}

- (CoverManager *)init {
    self = [super init];
    _testCover = [NSImage imageNamed:@"noartwork.png"];
    return self;
}

- (NSImage *)getTestCover {
    return _testCover;
}

@end
