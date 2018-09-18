#import "ItemListViewController.h"

@interface ItemListViewController ()

@end

@implementation ItemListViewController {
    settings_property_t *_prop;
}

- (void)viewDidLoad {
    [super viewDidLoad];
}

- (id)initWithProp:(settings_property_t *)prop {
    self = [super initWithNibName:@"ScriptableItemList" bundle:nil];
    _prop = prop;
    return self;
}

- (IBAction)buttonBarAction:(id)sender {
}

// NSTableViewDataSource
- (NSInteger)numberOfRowsInTableView:(NSTableView *)aTableView {
    return 5;
}

- (id)tableView:(NSTableView *)aTableView objectValueForTableColumn:(NSTableColumn *)aTableColumn row:(NSInteger)rowIndex {
    return @"stub";
}

@end
