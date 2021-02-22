//
//  MainWindow.m
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 21/02/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//

#import "MainWindow.h"
#import "DesignModeEventHandler.h"
#import "DesignModeDeps.h"

@interface MainWindow()

@property (nonatomic) DesignModeEventHandler *designModeEventHandler;

@end


@implementation MainWindow

- (void)awakeFromNib {
    _designModeEventHandler = [[DesignModeEventHandler alloc] initWithDeps:DesignModeDeps.sharedInstance];
}

- (void)sendEvent:(NSEvent *)event {
    if ([self.designModeEventHandler sendEvent:event]) {
        return;
    }
    [super sendEvent:event];
}

@end
