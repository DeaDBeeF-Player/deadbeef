//
//  DdbPlaylistWidgetView.h
//  deadbeef
//
//  Created by Oleksiy Yakovenko on 22/09/14.
//  Copyright (c) 2014 Alexey Yakovenko. All rights reserved.
//

#import "DdbWidget.h"
#import "DdbListview.h"

@interface DdbPlaylistWidget : DdbWidget {
    id<DdbListviewDelegate> _delegate;
}
@property (readwrite) DdbListview *listview;
- (void)setDelegate:(id<DdbListviewDelegate>)delegate;
@end
