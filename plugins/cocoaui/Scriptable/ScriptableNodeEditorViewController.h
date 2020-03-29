//
//  ScriptableNodeEditorViewController.h
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 4/24/19.
//  Copyright © 2019 Alexey Yakovenko. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "ScriptableTableDataSource.h"
#import "ScriptableItemDelegate.h"

@class ScriptableNodeEditorViewController;

@protocol ScriptableNodeEditorCustomButtonsInitializer

- (void)customButtonsInitializer:(ScriptableNodeEditorViewController *_Nonnull)controller initButtonsInSegmentedControl:(NSSegmentedControl *_Nonnull)segmentedControl;

@end

NS_ASSUME_NONNULL_BEGIN

@interface ScriptableNodeEditorViewController : NSViewController

@property (weak) ScriptableTableDataSource *dataSource;
@property (weak) NSObject<ScriptableItemDelegate> *delegate;
@property (weak) NSObject<ScriptableNodeEditorCustomButtonsInitializer> *customButtonsInitializer;

- (void)reloadData;

@end

NS_ASSUME_NONNULL_END
