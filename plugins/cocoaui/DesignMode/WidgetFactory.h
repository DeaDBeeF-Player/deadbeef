//
//  WidgetFactory.h
//  deadbeef
//
//  Created by Alexey Yakovenko on 20/02/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "DesignModeDefs.h"

NS_ASSUME_NONNULL_BEGIN

typedef id<WidgetProtocol>_Nonnull(^WidgetInstantiatorBlockType)(void);

@interface WidgetFactory : NSObject<WidgetFactoryProtocol>

@property (nonatomic,class,readonly) WidgetFactory *sharedFactory;

- (nullable id<WidgetProtocol>)createWidgetWithType:(NSString *)type;
- (void)registerType:(NSString *)type instantiatorBlock:(WidgetInstantiatorBlockType)instantiatorBlock;
- (void)unregisterType:(NSString *)type;

@end

NS_ASSUME_NONNULL_END
