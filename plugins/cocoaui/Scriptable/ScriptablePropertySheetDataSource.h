//
//  ScriptablePropertySheetDataSource.h
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 4/24/19.
//  Copyright © 2019 Alexey Yakovenko. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "PropertySheetViewController.h"
#import "ScriptableProtocols.h"

NS_ASSUME_NONNULL_BEGIN

@interface ScriptablePropertySheetDataSource : NSObject<PropertySheetDataSource>
@property (weak) NSObject<ScriptableItemDelegate> *delegate;
- (instancetype)initWithScriptable:(scriptableItem_t *)scriptable;

@end

NS_ASSUME_NONNULL_END
