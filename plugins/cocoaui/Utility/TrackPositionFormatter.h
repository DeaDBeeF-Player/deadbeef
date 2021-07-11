//
//  TrackPositionFormatter.h
//  deadbeef
//
//  Created by Alexey Yakovenko on 11/07/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface TrackPositionFormatter : NSFormatter

- (NSString *)stringForObjectValue:(NSControl *)obj;

- (BOOL)getObjectValue:(out id  _Nullable *)obj forString:(NSString *)string errorDescription:(out NSString * _Nullable *)error;

@end
