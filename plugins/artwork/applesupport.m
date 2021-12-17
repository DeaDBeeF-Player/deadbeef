#import <Foundation/Foundation.h>
#include "applesupport.h"

void
apple_get_artwork_cache_path(char *buffer, size_t size) {
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSCachesDirectory, NSUserDomainMask, YES);
    NSString *cacheDirectory = [paths objectAtIndex:0];
    const char *cacheDirectoryCString = cacheDirectory.UTF8String;
    *buffer = 0;
    strncat (buffer, cacheDirectoryCString, size-1);
}
