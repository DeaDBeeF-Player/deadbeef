//
//  ScriptableNodeEditorViewController.h
//  DeaDBeeF
//
//  Created by Oleksiy Yakovenko on 4/24/19.
//  Copyright © 2019 Oleksiy Yakovenko. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "ScriptableTableDataSource.h"
#import "ScriptableProtocols.h"

@class ScriptableNodeEditorViewController;


@protocol ScriptableNodeEditorCustomButtonsInitializer

- (void)scriptableNodeEditorCustomButtonsInitializer:(ScriptableNodeEditorViewController *_Nonnull)controller initButtonsInSegmentedControl:(NSSegmentedControl *_Nonnull)segmentedControl;

@end


NS_ASSUME_NONNULL_BEGIN

@interface ScriptableNodeEditorViewController : NSViewController

@property (weak) ScriptableTableDataSource *dataSource;
@property (weak) NSObject<ScriptableItemDelegate> *delegate;
@property (weak) NSObject<ScriptableNodeEditorCustomButtonsInitializer> *scriptableNodeEditorDelegate;
@property (weak) NSObject<ScriptableErrorViewer> *errorViewer;

@property (nonatomic) BOOL canReset;

- (void)reloadData;
- (void)reset;

@end

NS_ASSUME_NONNULL_END
