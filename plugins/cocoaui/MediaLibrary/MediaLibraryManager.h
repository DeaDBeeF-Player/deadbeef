//
//  MediaLibraryManager.h
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 8/29/20.
//  Copyright © 2020 Alexey Yakovenko. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "deadbeef.h"
#include "medialib.h"

NS_ASSUME_NONNULL_BEGIN

@interface MediaLibraryManager : NSObject

@property (nonatomic,readonly) ddb_medialib_source_t source;

@end

NS_ASSUME_NONNULL_END
