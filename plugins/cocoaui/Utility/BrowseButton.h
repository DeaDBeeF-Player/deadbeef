//
//  BrowseButton.h
//  NesEdit
//
//  Created by Oleksiy Yakovenko on 3/19/19.
//  Copyright © 2019 Oleksiy Yakovenko. All rights reserved.
//

#import <Cocoa/Cocoa.h>

NS_ASSUME_NONNULL_BEGIN

@interface BrowseButton : NSButton

@property BOOL isDir;
@property (nonatomic, copy, nullable) void (^fileSelectedBlock)(NSString *path);
@property NSString *initialPath;

@end

NS_ASSUME_NONNULL_END
