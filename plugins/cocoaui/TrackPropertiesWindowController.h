//
//  TrackPropertiesWindowController.h
//  deadbeef
//
//  Created by Oleksiy Yakovenko on 16/02/15.
//  Copyright (c) 2015 Alexey Yakovenko. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface TrackPropertiesWindowController : NSWindowController

- (void)initWithData:(int)iter;
- (void)fill;

@property (unsafe_unretained) IBOutlet NSTableView *metadataTableView;
@property (unsafe_unretained) IBOutlet NSTableView *propertiesTableView;
@property (unsafe_unretained) IBOutlet NSTextField *filename;

@end
