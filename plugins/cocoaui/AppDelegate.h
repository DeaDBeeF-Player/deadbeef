//
//  AppDelegate.h
//  deadbeef
//
//  Created by Oleksiy Yakovenko on 16/11/13.
//  Copyright (c) 2013 Alexey Yakovenko. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface AppDelegate : NSObject <NSApplicationDelegate>

- (int)numberOfRowsInTableView:(NSTableView *)aTableView;

- (id)tableView:(NSTableView *)aTableView
objectValueForTableColumn:(NSTableColumn *)aTableColumn
            row:(int)rowIndex;

@property (assign) IBOutlet NSWindow *window;
@property (assign) IBOutlet NSTableView *playlists;
@property (assign) IBOutlet NSArrayController *arrayController;

@end
