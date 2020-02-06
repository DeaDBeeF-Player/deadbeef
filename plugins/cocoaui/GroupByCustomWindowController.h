//
//  GroupByCustomWindowController.h
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 11/26/19.
//  Copyright © 2019 Alexey Yakovenko. All rights reserved.
//

#import <Cocoa/Cocoa.h>

NS_ASSUME_NONNULL_BEGIN

@interface GroupByCustomWindowController : NSWindowController

@property (weak) IBOutlet NSTextField *formatTextField;

- (void)initWithFormat:(NSString *)format;

@end

NS_ASSUME_NONNULL_END
