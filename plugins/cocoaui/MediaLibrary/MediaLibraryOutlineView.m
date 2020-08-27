//
//  MediaLibraryOutlineView.m
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 8/27/20.
//  Copyright © 2020 Alexey Yakovenko. All rights reserved.
//

#import "MediaLibraryOutlineView.h"

@implementation MediaLibraryOutlineView

- (BOOL)validateProposedFirstResponder:(NSResponder *)responder forEvent:(NSEvent *)event {
    // This is required so that the Search field can become first responder.
    return YES;
}

@end
