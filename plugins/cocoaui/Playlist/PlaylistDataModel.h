//
//  PlaylistDataModel.h
//  deadbeef
//
//  Created by Oleksiy Yakovenko on 20/02/2021.
//  Copyright © 2021 Oleksiy Yakovenko. All rights reserved.
//

#import <Foundation/Foundation.h>

typedef intptr_t DdbListviewRow_t;

@protocol DdbListviewDataModelProtocol<NSObject>

@property (nonatomic,readonly) int rowCount;
@property (nonatomic) int cursor;

@property (nonatomic,readonly) DdbListviewRow_t firstRow;
- (DdbListviewRow_t)nextRow:(DdbListviewRow_t)row;

@property (nonatomic,readonly) DdbListviewRow_t invalidRow;

@property (nonatomic,readonly) int modificationIdx;
@property (nonatomic,readonly) int selectedCount;
- (void)lock;
- (void)unlock;

- (void)activate:(int)idx;

- (DdbListviewRow_t)rowForIndex:(int)idx;
- (void)refRow:(DdbListviewRow_t)row;
- (void)unrefRow:(DdbListviewRow_t)row;
- (void)selectRow:(DdbListviewRow_t)row withState:(BOOL)state;
- (BOOL)rowSelected:(DdbListviewRow_t)row;
- (void)deselectAll;

@end

@interface PlaylistDataModel : NSObject<DdbListviewDataModelProtocol>

+ (instancetype)new NS_UNAVAILABLE;
- (instancetype)init NS_UNAVAILABLE;
- (instancetype)initWithIter:(int)iter NS_DESIGNATED_INITIALIZER;

@end
