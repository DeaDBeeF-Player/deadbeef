//
//  DdbSearchWidget.h
//  deadbeef
//
//  Created by Oleksiy Yakovenko on 08/10/14.
//  Copyright (c) 2014 Alexey Yakovenko. All rights reserved.
//

#import "DdbWidget.h"
#import "DdbListview.h"

@interface DdbSearchWidget : DdbWidget {
    id<DdbListviewDelegate> _delegate;
}
- (void)setDelegate:(id<DdbListviewDelegate>) delegate;
@property (unsafe_unretained) IBOutlet DdbListview *listview;
@end
