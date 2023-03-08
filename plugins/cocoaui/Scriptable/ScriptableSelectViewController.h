//
//  ScriptableSelectViewController.h
//  DeaDBeeF
//
//  Created by Oleksiy Yakovenko on 4/24/19.
//  Copyright © 2019 Oleksiy Yakovenko. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#include "scriptable/scriptable.h"
#import "ScriptableTableDataSource.h"
#import "ScriptableProtocols.h"

NS_ASSUME_NONNULL_BEGIN

@protocol ScriptableSelectDelegate

- (void)scriptableSelectItemSelected:(scriptableItem_t *)item;

@end

@interface ScriptableSelectViewController : NSViewController

@property (nonatomic,strong) IBOutlet ScriptableTableDataSource *dataSource;
@property (weak) NSObject<ScriptableItemDelegate> *scriptableItemDelegate;
@property (weak) NSObject<ScriptableSelectDelegate> *scriptableSelectDelegate;
@property (weak) NSObject<ScriptableErrorViewer> *errorViewer;

@property (nonatomic,readonly) NSInteger indexOfSelectedItem;

- (void)reloadData;
- (void)selectItem:(scriptableItem_t *)item;

@end

NS_ASSUME_NONNULL_END
