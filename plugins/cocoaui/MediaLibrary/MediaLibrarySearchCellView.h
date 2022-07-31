//
//  MediaLibrarySearchCellView.h
//  DeaDBeeF
//
//  Created by Oleksiy Yakovenko on 8/27/20.
//  Copyright © 2020 Oleksiy Yakovenko. All rights reserved.
//

#import <Cocoa/Cocoa.h>

NS_ASSUME_NONNULL_BEGIN

@protocol MediaLibrarySearchCellViewDelegate

- (void)mediaLibrarySearchCellViewTextChanged:(NSString *)text;

@end

@interface MediaLibrarySearchCellView : NSTableCellView

@property (nonatomic) IBOutlet NSSearchField *searchField;
@property (weak,nonatomic,nullable) id<MediaLibrarySearchCellViewDelegate> delegate;

@end

NS_ASSUME_NONNULL_END
