#import "ItemListViewController.h"
#import "deadbeef-Swift.h"

@interface ItemListViewController ()

@end

@implementation ItemListViewController {
    settings_property_t *_prop;
    NSMutableArray< id<Scriptable> > *_items; //
}

- (void)viewDidLoad {
    [super viewDidLoad];
}

- (id)initWithProp:(settings_property_t *)prop accessor:(NSObject<PluginConfigurationValueAccessor> *)accessor {
    self = [super initWithNibName:@"ScriptableItemList" bundle:nil];
    _prop = prop;

    int cnt = [accessor count];
    _items = [[NSMutableArray alloc] initWithCapacity:cnt];
    for (int i = 0; i < cnt; i++) {
        NSString *key = [accessor keyForIndex:i];
        NSString *val = [accessor getValueForKey:key def:nil];

        // value is JSON: {type:string, items:list}
        NSData *data = [val dataUsingEncoding:NSUTF8StringEncoding];

        NSError *err = nil;
        NSDictionary *json = [NSJSONSerialization JSONObjectWithData:data options:0 error:&err];

        if (!json) {
            NSLog (@"Failed to parse plugin list item json, error: %@\n", [err localizedDescription]);
            return nil;
        }

        __typeof([NSObject<Scriptable> class]) c = NSClassFromString([NSString stringWithUTF8String:prop->itemlist_type]);

        if (c) {
            NSString *type = json[@"type"];
            id<Scriptable> item = [c createWithType:type];
            [item loadWithData:json];
            [_items addObject:item];
        }
    }

    return self;
}

- (IBAction)buttonBarAction:(id)sender {
}

// NSTableViewDataSource
- (NSInteger)numberOfRowsInTableView:(NSTableView *)aTableView {
    return [_items count];
}

- (id)tableView:(NSTableView *)aTableView objectValueForTableColumn:(NSTableColumn *)aTableColumn row:(NSInteger)rowIndex {
    return [_items[rowIndex] displayName];
}

@end
