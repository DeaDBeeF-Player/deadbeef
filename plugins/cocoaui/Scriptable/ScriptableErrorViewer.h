//
//  ScriptableErrorViewer.h
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 4/4/20.
//  Copyright © 2020 Alexey Yakovenko. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "ScriptableProtocols.h"

NS_ASSUME_NONNULL_BEGIN

@interface ScriptableErrorViewer : NSObject<ScriptableErrorViewer>

@property (class,readonly) ScriptableErrorViewer *sharedInstance;

- (void)displayDuplicateNameError;
- (void)displayInvalidNameError;

@end

NS_ASSUME_NONNULL_END
