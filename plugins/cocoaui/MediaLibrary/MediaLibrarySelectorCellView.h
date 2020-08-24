//
//  MediaLibrarySelectorCellView.h
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 8/2/20.
//  Copyright © 2020 Alexey Yakovenko. All rights reserved.
//

#import <Cocoa/Cocoa.h>

NS_ASSUME_NONNULL_BEGIN

@class MediaLibrarySelectorCellView;

@protocol MediaLibraryFilterSelectorCellViewDelegate

- (void)filterSelectorChanged:(MediaLibrarySelectorCellView *)cellView;

@end

@interface MediaLibrarySelectorCellView : NSTableCellView

@property (nonatomic,weak) id<MediaLibraryFilterSelectorCellViewDelegate> delegate;
@property (strong) IBOutlet  NSPopUpButton *popupButton;

@end

NS_ASSUME_NONNULL_END
