//
//  MainContentViewController.h
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 02/10/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "DdbTabStrip.h"

NS_ASSUME_NONNULL_BEGIN

@interface MainContentViewController : NSViewController

@property (weak) IBOutlet DdbTabStrip *tabStrip;
@property (weak) IBOutlet NSView *designableView;
@property (weak) IBOutlet NSView *wrapperView;

@end

NS_ASSUME_NONNULL_END
