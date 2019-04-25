//
//  ScriptableNodeEditorViewController.h
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 4/24/19.
//  Copyright © 2019 Alexey Yakovenko. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "ScriptableTableDataSource.h"

NS_ASSUME_NONNULL_BEGIN

@interface ScriptableNodeEditorViewController : NSViewController

@property (weak) ScriptableTableDataSource *dataSource;

@end

NS_ASSUME_NONNULL_END
