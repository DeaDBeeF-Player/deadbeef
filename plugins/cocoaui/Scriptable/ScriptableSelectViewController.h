//
//  ScriptableSelectViewController.h
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 4/24/19.
//  Copyright © 2019 Alexey Yakovenko. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#include "scriptable.h"
#import "ScriptableTableDataSource.h"
#import "ScriptableProtocols.h"

NS_ASSUME_NONNULL_BEGIN

@protocol ScriptableSelectDelegate

- (void)scriptableSelectItemSelected:(scriptableItem_t *)item;

@end

@interface ScriptableSelectViewController : NSViewController

@property (weak) ScriptableTableDataSource *dataSource;
@property (weak) NSObject<ScriptableItemDelegate> *scriptableItemDelegate;
@property (weak) NSObject<ScriptableSelectDelegate> *scriptableSelectDelegate;
@property (weak) NSObject<ScriptableErrorViewer> *errorViewer;

- (void)setScriptable:(scriptableItem_t *)scriptable;
- (void)reloadData;

@end

NS_ASSUME_NONNULL_END
