//
//  MediaLibraryOutlineView.h
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 8/27/20.
//  Copyright © 2020 Alexey Yakovenko. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@class MediaLibraryOutlineView;

@protocol MediaLibraryOutlineViewDelegate<NSOutlineViewDelegate>

@optional
- (void)mediaLibraryOutlineViewDidActivateAlternative:(MediaLibraryOutlineView *)outlineView;
- (BOOL)mediaLibraryOutlineView:(MediaLibraryOutlineView *)outlineView shouldDisplayMenuForRow:(NSInteger)row;
@end

NS_ASSUME_NONNULL_BEGIN

@interface MediaLibraryOutlineView : NSOutlineView

@end

NS_ASSUME_NONNULL_END
