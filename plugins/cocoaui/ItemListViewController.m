#import "ItemListViewController.h"
#import "deadbeef-Swift.h"

@interface ItemListViewController ()

@end

@implementation ItemListViewController {
    settings_property_t *_prop;
    id<Scriptable> _scriptable;
}

- (void)viewDidLoad {
    [super viewDidLoad];
}

- (id)initWithProp:(settings_property_t *)prop scriptable:(id<Scriptable>)scriptable {
    self = [super initWithNibName:@"ScriptableItemList" bundle:nil];
    _prop = prop;
    _scriptable = scriptable;

    return self;
}

- (IBAction)closeAction:(id)sender {
    [_itemConfigurationPanel orderOut:[[self view] window]];
}

- (IBAction)resetAction:(id)sender {
    [_itemConfigViewController resetPluginConfigToDefaults];
}

- (IBAction)buttonBarAction:(id)sender {
    NSInteger seg = [sender selectedSegment];
    switch (seg) {
        case 0: // add
        {
            NSArray<NSString *> *types = [_scriptable getItemTypes];
            NSMenu *menu = [[NSMenu alloc] initWithTitle:@"DspChainMenu"];
            [menu setDelegate:(id<NSMenuDelegate>)self];
            [menu setAutoenablesItems:NO];
            int i = 0;
            for (NSString *type in types) {
                NSString *title = [_scriptable getItemNameWithType:type];
                [[menu insertItemWithTitle:title action:@selector(addItemAction:) keyEquivalent:@"" atIndex:i++] setTarget:self];
            }
            [NSMenu popUpContextMenu:menu withEvent:[NSApp currentEvent] forView:sender];
            break;
        }
        case 1:
        {
            NSInteger index = [_tableView selectedRow];
            if (index >= 0) {
                [_scriptable removeItemWithIndex:index];
                [_tableView reloadData];
            }
            break;
        }
        case 2:
        {
            NSInteger index = [_tableView selectedRow];
            if (index >= 0) {
                [self configureAction:index];
            }
            break;
        }
    }
}

- (void)configureAction:(NSInteger)index {
    //@objc public func configure (presetIndex: Int, subItemIndex:Int, sheet:NSWindow, parentWindow:NSWindow, viewController:PluginConfigurationViewController) {
    id<Scriptable> item = [_scriptable getItems][index];
    PluginConfigurationScriptableBackend *accessor = [[PluginConfigurationScriptableBackend alloc] initWithScriptable:item];

    [_itemConfigViewController initPluginConfiguration:[[item getScript] UTF8String] accessor:accessor];

    NSWindow *window = [[self view] window];
    [window beginSheet:_itemConfigurationPanel completionHandler:^(NSModalResponse returnCode) {
        [_itemConfigurationPanel orderOut:window];
    }];
}

- (void)addItemAction:(id)sender {
    NSMenuItem *item = sender;
    NSInteger index = [[item menu] indexOfItem:item];
    NSArray<NSString *> *types = [_scriptable getItemTypes];

    (void)[_scriptable addItemWithType:types[index]];
    [_tableView reloadData];
}

// NSTableViewDataSource
- (NSInteger)numberOfRowsInTableView:(NSTableView *)aTableView {
    return [[_scriptable getItems] count];
}

- (id)tableView:(NSTableView *)aTableView objectValueForTableColumn:(NSTableColumn *)aTableColumn row:(NSInteger)rowIndex {
    return [[_scriptable getItems][rowIndex] displayName];
}

@end
