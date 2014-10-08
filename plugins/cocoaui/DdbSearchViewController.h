//
//  DdbSearchViewController.h
//  deadbeef
//
//  Created by Oleksiy Yakovenko on 08/10/14.
//  Copyright (c) 2014 Alexey Yakovenko. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "DdbListview.h"
#import "DdbPlaylistViewController.h"

@interface DdbSearchViewController : DdbPlaylistViewController

@property (unsafe_unretained) IBOutlet NSTextField *entry;
@property (unsafe_unretained) IBOutlet DdbListview *listview;

- (id)init;
@end
