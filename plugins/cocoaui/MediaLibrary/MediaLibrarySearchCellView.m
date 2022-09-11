//
//  MediaLibrarySearchCellView.m
//  DeaDBeeF
//
//  Created by Oleksiy Yakovenko on 8/27/20.
//  Copyright © 2020 Oleksiy Yakovenko. All rights reserved.
//

#import "MediaLibrarySearchCellView.h"

@interface MediaLibrarySearchCellView()
@end

@implementation MediaLibrarySearchCellView

- (void)awakeFromNib {
}

- (IBAction)searchFieldAction:(id)sender {
    [self.delegate mediaLibrarySearchCellViewTextChanged:self.searchField.stringValue];
}

- (void)searchFieldDidStartSearching:(NSSearchField *)sender {
}

- (void)searchFieldDidEndSearching:(NSSearchField *)sender {
}

@end
