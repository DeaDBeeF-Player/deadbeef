/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2025 Oleksiy Yakovenko and other contributors

    This software is provided 'as-is', without any express or implied
    warranty.  In no event will the authors be held liable for any damages
    arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.

    3. This notice may not be removed or altered from any source distribution.
*/

#import "TrackPropertiesMultipleFieldsTableData.h"
#import "TrackPropertiesSingleLineFormatter.h"

@implementation TrackPropertiesMultipleFieldsTableData

- (NSInteger)numberOfRowsInTableView:(NSTableView *)tableView {
    return self.fields.count;
}

- (NSView *)tableView:(NSTableView *)tableView viewForTableColumn:(NSTableColumn *)tableColumn row:(NSInteger)row {
    NSUserInterfaceItemIdentifier ident = tableColumn.identifier;
    NSTableCellView *view = [tableView makeViewWithIdentifier:ident owner:self];
    NSTextField *textView = view.textField;
    if ([ident isEqualToString:@"Index"]) {
        textView.stringValue = [NSString stringWithFormat:@"%d", (int)row+1];
    }
    else if ([ident isEqualToString:@"Item"]) {
        textView.stringValue = self.items[row];
    }
    else if ([ident isEqualToString:@"Field"]) {
        textView.formatter = [TrackPropertiesSingleLineFormatter new];
        textView.stringValue = self.fields[row];
        textView.target = self;
        textView.action = @selector(fieldEditedAction:);
        textView.identifier = [NSString stringWithFormat:@"%d", (int)row];
    }
    return view;
}

- (void)fieldEditedAction:(NSTextField *)sender {
    NSString *value = sender.stringValue;
    int row = sender.identifier.intValue;
    self.fields[row] = value;
    // FIXME: add modified flag
}

@end
