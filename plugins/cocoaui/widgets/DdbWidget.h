//
//  DdbWidget.h
//  deadbeef
//
//  Created by waker on 29/08/14.
//  Copyright (c) 2014 Alexey Yakovenko. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface DdbWidget : NSView

@property NSString *widgetType;
@property UInt32 widgetFlags;
@property NSArray *widgetChildren;
@property BOOL inDesignMode;

// methods which can be overriden
- (void)widgetInit;
- (NSString *)widgetSave;
- (NSString *)widgetLoad:(NSString *)type;
- (void)widgetDestroy;
- (void)widgetAppend:(DdbWidget *)child;
- (void)widgetRemove:(DdbWidget *)child;
- (void)widgetReplace:(DdbWidget *)child withWidget:(DdbWidget *)newChild;
- (NSView *)widgetGetContainer;
- (int)widgetMessage:(uint32_t)_id ctx:(uintptr_t)ctx p1:(uint32_t)p1 p2:(uint32_t)p2;
- (void)widgetInitMenu:(NSMenu *)menu;
- (void)widgetInitChildMenu:(NSMenu *)menu;

@end
