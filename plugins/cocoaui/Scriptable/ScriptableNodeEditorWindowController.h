//
//  ScriptableNodeEditorWindowController.h
//  DeaDBeeF
//
//  Created by Oleksiy Yakovenko on 4/25/19.
//  Copyright © 2019 Oleksiy Yakovenko. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "ScriptableTableDataSource.h"
#import "ScriptableProtocols.h"

NS_ASSUME_NONNULL_BEGIN

@interface ScriptableNodeEditorWindowController : NSWindowController

@property (weak) ScriptableTableDataSource *dataSource;
@property (weak) NSObject<ScriptableItemDelegate> *delegate;
@property (weak) NSObject<ScriptableErrorViewer> *errorViewer;

@end

NS_ASSUME_NONNULL_END
