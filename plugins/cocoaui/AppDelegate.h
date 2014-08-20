//
//  AppDelegate.h
//  deadbeef
//
//  Created by Oleksiy Yakovenko on 16/11/13.
//  Copyright (c) 2013 Alexey Yakovenko. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface AppDelegate : NSObject <NSApplicationDelegate>

@property (assign) IBOutlet NSWindow *window;
@property (unsafe_unretained) IBOutlet NSTableView *playlist;
- (IBAction)openFilesAction:(id)sender;
- (IBAction)addFilesAction:(id)sender;
- (IBAction)addFoldersAction:(id)sender;
- (IBAction)clearAction:(id)sender;


@end
