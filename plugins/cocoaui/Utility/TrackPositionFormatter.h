//
//  TrackPositionFormatter.h
//  deadbeef
//
//  Created by Alexey Yakovenko on 11/07/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface TrackPositionFormatter : NSFormatter

- (nullable NSString *)stringForObjectValue:(nullable NSControl *)obj;

- (BOOL)getObjectValue:(out id  _Nullable * _Nullable)obj forString:(NSString * _Nonnull)string errorDescription:(out NSString * _Nullable * _Nullable)error;

@end
