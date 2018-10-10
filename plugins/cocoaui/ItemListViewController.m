#import "ItemListViewController.h"
#import "deadbeef-Swift.h"

@interface ItemListViewController ()

@end

@implementation ItemListViewController {
    settings_property_t *_prop;
    NSMutableArray< id<Scriptable> > *_items;
    id<PluginConfigurationValueAccessor> _accessor;
}

- (void)viewDidLoad {
    [super viewDidLoad];
}

- (id)initWithProp:(settings_property_t *)prop accessor:(id<PluginConfigurationValueAccessor>)accessor {
    self = [super initWithNibName:@"ScriptableItemList" bundle:nil];
    _prop = prop;
    _accessor = accessor;

    [self fetchItems];

    return self;
}

- (void) fetchItems {
    int cnt = [_accessor count];
    _items = [[NSMutableArray alloc] initWithCapacity:cnt];
    for (int i = 0; i < cnt; i++) {
        NSString *key = [_accessor keyForIndex:i];
        NSString *val = [_accessor getValueForKey:key def:nil];

        // value is JSON: {type:string, items:list}
        NSData *data = [val dataUsingEncoding:NSUTF8StringEncoding];

        NSError *err = nil;
        NSDictionary *json = [NSJSONSerialization JSONObjectWithData:data options:0 error:&err];

        if (!json) {
            NSLog (@"Failed to parse plugin list item json, error: %@\n", [err localizedDescription]);
            return;
        }

        __typeof([NSObject<Scriptable> class]) c = NSClassFromString([NSString stringWithUTF8String:_prop->itemlist_type]);

        if (c) {
            NSString *type = json[@"type"];
            id<Scriptable> item = [c createWithType:type];
            [item loadWithData:json];
            [_items addObject:item];
        }
    }}

- (IBAction)buttonBarAction:(id)sender {
    NSInteger seg = [sender selectedSegment];
    switch (seg) {
        case 0: // add
        {
            NSArray<NSString *> *types = [_accessor getItemTypes];
            NSMenu *menu = [[NSMenu alloc] initWithTitle:@"DspChainMenu"];
            [menu setDelegate:(id<NSMenuDelegate>)self];
            [menu setAutoenablesItems:NO];
            int i = 0;
            for (NSString *type in types) {
                NSString *title = [_accessor getItemNameWithType:type];
                [[menu insertItemWithTitle:title action:@selector(addItemAction:) keyEquivalent:@"" atIndex:i++] setTarget:self];
            }
            [NSMenu popUpContextMenu:menu withEvent:[NSApp currentEvent] forView:sender];
            break;
        }
            
    }
}

- (void)addItemAction:(id)sender {
    NSMenuItem *item = sender;
    NSInteger index = [[item menu] indexOfItem:item];
    NSArray<NSString *> *types = [_accessor getItemTypes];

    [_accessor addItemWithType:types[index]];
    [self fetchItems];
    [_tableView reloadData];
}


#if 0
- (IBAction)dspAddAction:(id)sender {
    NSMenu *menu = [self getDSPMenu];
    [NSMenu popUpContextMenu:menu withEvent:[NSApp currentEvent] forView:sender];
}

- (IBAction)dspRemoveAction:(id)sender {
    NSInteger index = [_dspList selectedRow];
    if (index < 0) {
        return;
    }

    [_dspPresetController removeItemWithIndex:index];
    [_dspList reloadData];

    if (index >= [_dspList numberOfRows]) {
        index--;
    }
    if (index >= 0) {
        [_dspList selectRowIndexes:[NSIndexSet indexSetWithIndex:index] byExtendingSelection:NO];
    }
}


// needs to go to swift presetmgr
- (IBAction)dspConfigureAction:(id)sender {
    NSInteger index = [_dspList selectedRow];
    if (index < 0) {
        return;
    }
    [_dspPresetController.presetMgr configureWithPresetIndex:-1 subItemIndex:index sheet: _dspConfigPanel parentWindow:[self window] viewController:_dspConfigViewController];
}

// FIXME: where is this used?
- (IBAction)dspConfigResetAction:(id)sender {
    [_dspConfigViewController resetPluginConfigToDefaults];
    //    [_dspChainDataSource apply];
}

- (IBAction)dspChainAction:(id)sender {
    NSInteger selectedSegment = [sender selectedSegment];

    switch (selectedSegment) {
        case 0:
            [self dspAddAction:sender];
            break;
        case 1:
            [self dspRemoveAction:sender];
            break;
        case 2:
            [self dspConfigureAction:sender];
            break;
    }
}

- (IBAction)dspSaveAction:(id)sender {
}

- (IBAction)dspLoadAction:(id)sender {
}
#endif

// NSTableViewDataSource
- (NSInteger)numberOfRowsInTableView:(NSTableView *)aTableView {
    return [_items count];
}

- (id)tableView:(NSTableView *)aTableView objectValueForTableColumn:(NSTableColumn *)aTableColumn row:(NSInteger)rowIndex {
    return [_items[rowIndex] displayName];
}

@end
