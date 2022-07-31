//
//  MainContentViewController.h
//  DeaDBeeF
//
//  Created by Oleksiy Yakovenko on 02/10/2021.
//  Copyright © 2021 Oleksiy Yakovenko. All rights reserved.
//

#import <Cocoa/Cocoa.h>

NS_ASSUME_NONNULL_BEGIN

@interface MainContentViewController : NSViewController

@property (weak) IBOutlet NSView *designableView;
@property (weak) IBOutlet NSView *wrapperView;

@end

NS_ASSUME_NONNULL_END
