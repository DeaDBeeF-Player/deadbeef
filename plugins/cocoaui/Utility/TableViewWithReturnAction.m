//
//  TableViewWithReturnAction.m
//  deadbeef
//
//  Created by Oleksiy Yakovenko on 21/11/2021.
//  Copyright © 2021 Oleksiy Yakovenko. All rights reserved.
//

#import "TableViewWithReturnAction.h"

@implementation TableViewWithReturnAction

- (void)keyDown:(NSEvent *)event {
    if (event.characters.length == 1 && event.keyCode == 36) {
        [self sendAction:self.doubleAction to:self.target];
    }
    else {
        [super keyDown:event];
    }
}

@end
