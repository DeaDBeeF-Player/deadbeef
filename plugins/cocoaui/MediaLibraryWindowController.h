//
//  MediaLibraryWindowController.h
//  deadbeef
//
//  Created by Alexey Yakovenko on 2/4/17.
//  Copyright © 2017 Alexey Yakovenko. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface MediaLibraryWindowController : NSWindowController
@property (weak) IBOutlet NSOutlineView *outlineView;
- (IBAction)queryChanged:(id)sender;
@property (weak) IBOutlet NSProgressIndicator *scannerActiveIndicator;
@property (weak) IBOutlet NSTextField *scannerActiveState;

@end
