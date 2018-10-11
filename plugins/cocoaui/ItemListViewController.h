//
//  ItemListViewController.h
//  deadbeef
//
//  Created by Oleksiy Yakovenko on 18/09/2018.
//  Copyright © 2018 Alexey Yakovenko. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "PluginConfigurationViewController.h"
#include "../../shared/pluginsettings.h"

NS_ASSUME_NONNULL_BEGIN

@interface ItemListViewController : NSViewController<NSTableViewDelegate, NSTableViewDataSource>

@property (strong) IBOutlet PluginConfigurationViewController *itemConfigViewController;
@property (strong) IBOutlet NSPanel *itemConfigurationPanel;

@property (weak) IBOutlet NSTableView *tableView;
- (IBAction)buttonBarAction:(id)sender;
- (id)initWithProp:(settings_property_t *)prop scriptable:(id)scriptable;

- (IBAction)closeAction:(id)sender;
- (IBAction)resetAction:(id)sender;

@end

NS_ASSUME_NONNULL_END
