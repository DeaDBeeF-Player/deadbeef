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
        }
            
    }
}

- (void)addItemAction:(id)sender {
    NSMenuItem *item = sender;
    NSInteger index = [[item menu] indexOfItem:item];
    NSArray<NSString *> *types = [_scriptable getItemTypes];

    [_scriptable addItemWithType:types[index]];
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
    return [[_scriptable getItems] count];
}

- (id)tableView:(NSTableView *)aTableView objectValueForTableColumn:(NSTableColumn *)aTableColumn row:(NSInteger)rowIndex {
    return [[_scriptable getItems][rowIndex] displayName];
}

@end
