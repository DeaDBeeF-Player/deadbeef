//
//  MediaLibraryManager.h
//  DeaDBeeF
//
//  Created by Oleksiy Yakovenko on 8/29/20.
//  Copyright © 2020 Oleksiy Yakovenko. All rights reserved.
//

#import <Foundation/Foundation.h>
#include <deadbeef/deadbeef.h>
#include "medialib.h"

NS_ASSUME_NONNULL_BEGIN

@interface MediaLibraryManager : NSObject

@property (nonatomic,readonly) DB_mediasource_t *medialibPlugin;
@property (nonatomic,readonly) ddb_mediasource_source_t *source;

@property (nullable, nonatomic) NSString *preset;

@end

NS_ASSUME_NONNULL_END
