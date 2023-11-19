//
//  ScriptableSelectViewController.h
//  DeaDBeeF
//
//  Created by Oleksiy Yakovenko on 4/24/19.
//  Copyright © 2019 Oleksiy Yakovenko. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#include "scriptable/scriptable.h"
#include "scriptable/scriptable_model.h"
#import "ScriptableTableDataSource.h"
#import "ScriptableProtocols.h"

NS_ASSUME_NONNULL_BEGIN

@protocol ScriptableSelectDelegate

- (void)scriptableSelectItemSelected:(scriptableItem_t *)item;
- (void)scriptableItemDidChange:(scriptableItem_t *_Nonnull)scriptable change:(ScriptableItemChange)change;

@end

@interface ScriptableSelectViewController : NSViewController

@property (nonatomic,strong) IBOutlet ScriptableTableDataSource *dataSource;
@property (weak) NSObject<ScriptableSelectDelegate> *delegate;
@property (weak) NSObject<ScriptableErrorViewer> *errorViewer;
@property scriptableModel_t *scriptableModel;

@property (nonatomic,readonly) NSInteger indexOfSelectedItem;

- (void)reloadData;
- (void)selectItem:(scriptableItem_t *)item;

@end

NS_ASSUME_NONNULL_END
