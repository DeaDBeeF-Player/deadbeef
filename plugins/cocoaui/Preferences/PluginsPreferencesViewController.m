//
//  PluginsPreferencesViewController.m
//  DeaDBeeF
//
//  Created by Oleksiy Yakovenko on 2/24/20.
//  Copyright © 2020 Oleksiy Yakovenko. All rights reserved.
//

#import "PluginsPreferencesViewController.h"
#import "PreferencesPluginEntry.h"
#import "PropertySheetViewController.h"
#include <deadbeef/deadbeef.h>

extern DB_functions_t *deadbeef;

@interface PluginConfigPropertySheetDataSource : NSObject<PropertySheetDataSource> {
    DB_plugin_t *_plugin;
    BOOL _multipleChanges;
}
@end

@implementation PluginConfigPropertySheetDataSource
- (instancetype)initWithPlugin:(DB_plugin_t *)plugin {
    self = [super init];
    _plugin = plugin;
    return self;
}

- (NSString *)propertySheet:(PropertySheetViewController *)vc configForItem:(id)item {
    return _plugin->configdialog ? [NSString stringWithUTF8String:_plugin->configdialog] : nil;
}

- (NSString *)propertySheet:(PropertySheetViewController *)vc valueForKey:(NSString *)key def:(NSString *)def item:(id)item {
    char str[1000];
    deadbeef->conf_get_str ([key UTF8String], [def UTF8String], str, sizeof (str));
    return [NSString stringWithUTF8String:str];
}

- (void)propertySheet:(PropertySheetViewController *)vc setValue:(NSString *)value forKey:(NSString *)key item:(id)item {
    deadbeef->conf_set_str ([key UTF8String], [value UTF8String]);
    if (!_multipleChanges) {
        deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
    }
}

- (void)propertySheetBeginChanges {
    _multipleChanges = YES;
}

- (void)propertySheetCommitChanges {
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
    _multipleChanges = NO;
}

@end

#pragma mark -

@interface PluginsPreferencesViewController () <NSTableViewDataSource, NSTableViewDelegate>

// Plugins properties
@property (nonatomic,weak) IBOutlet NSTextField *pluginUnselectedText;
@property (nonatomic,weak) IBOutlet NSTabView *pluginTabView;

@property (nonatomic) NSArray<PreferencesPluginEntry *> *pluginList;
@property (nonatomic) BOOL isShowingConfigurablePlugins;

@property (nonatomic,weak) IBOutlet NSTableView *pluginsTableView;
@property (nonatomic,weak) IBOutlet NSTextField *pluginVersion;
@property (nonatomic,weak) IBOutlet NSTextView *pluginDescription;
@property (nonatomic,weak) IBOutlet NSTextView *pluginLicense;

@property (nonatomic,weak) IBOutlet PropertySheetViewController *pluginConfViewController;

@property (nonatomic,weak) IBOutlet NSButton *wwwButton;

- (IBAction)pluginOpenWebsite:(id)sender;
- (IBAction)pluginConfResetDefaults:(id)sender;

@property (nonatomic) PluginConfigPropertySheetDataSource *pluginPropertySheetDataSource;
@property (nonatomic) NSURL *website;


@end

@implementation PluginsPreferencesViewController

- (void)viewDidLoad {
    [super viewDidLoad];

    // Do view setup here.
    [self initPluginList];

    // plugin list context menu
    NSMenu *menu = [NSMenu new];

    [menu addItemWithTitle:@"Copy report" action:@selector(copyReport:) keyEquivalent:@""];
    [menu addItemWithTitle:@"Only show plugins with configuration" action:@selector(toggleShowConfigurablePlugins:) keyEquivalent:@""];

    menu.autoenablesItems = NO;

    self.pluginsTableView.menu = menu;
}

- (void)initPluginList {
    NSMutableArray *pluginsArray = [NSMutableArray new];

    DB_plugin_t **plugins = deadbeef->plug_get_list();

    for (int i = 0; plugins[i]; i++) {
        if (!plugins[i]->configdialog && self.isShowingConfigurablePlugins) {
            continue;
        }
        PreferencesPluginEntry *entry = [PreferencesPluginEntry new];
        entry.plugin = plugins[i];
        [pluginsArray addObject:entry];
    }

    self.pluginList = [pluginsArray sortedArrayUsingComparator:^NSComparisonResult(PreferencesPluginEntry * _Nonnull obj1, PreferencesPluginEntry * _Nonnull obj2) {
        return strcasecmp (obj1.plugin->name, obj2.plugin->name);
    }];

    self.pluginsTableView.dataSource = self;
    self.pluginsTableView.delegate = self;
    self.pluginInfo = -1;
}

- (void)copyReport:(NSMenuItem *)sender {
    NSString *report = @"";

    DB_plugin_t **plugins = deadbeef->plug_get_list ();
    int i;
    for (i = 0; plugins[i]; i++) {
        const char *path = deadbeef->plug_get_path_for_plugin_ptr (plugins[i]);
        NSString *strPath = path ? [NSString stringWithUTF8String:path] : @"(builtin)";
        NSString *name = [NSString stringWithUTF8String:plugins[i]->name];

        report = [report stringByAppendingFormat:@"%@: %@ (%d.%d)\n", strPath, name, plugins[i]->version_major, plugins[i]->version_minor];
    }

    [NSPasteboard.generalPasteboard clearContents];
    [NSPasteboard.generalPasteboard setString:report forType:NSStringPboardType];
}

- (void)toggleShowConfigurablePlugins:(NSMenuItem *)sender {
    self.isShowingConfigurablePlugins = !self.isShowingConfigurablePlugins;
    sender.state = self.isShowingConfigurablePlugins ? NSControlStateValueOn : NSControlStateValueOff;
    [self initPluginList];
    [self.pluginsTableView reloadData];
}

- (IBAction)pluginOpenWebsite:(id)sender {
    if (self.website) {
        [NSWorkspace.sharedWorkspace openURL:self.website];
    }
}

- (IBAction)pluginConfResetDefaults:(id)sender {
    [_pluginConfViewController reset];
}

- (void)initPluginConfiguration:(NSInteger)idx {
    DB_plugin_t *p = self.pluginList[idx].plugin;

    self.pluginPropertySheetDataSource = [[PluginConfigPropertySheetDataSource alloc] initWithPlugin:p];

    _pluginConfViewController.labelFontSize = 10;
    _pluginConfViewController.contentFontSize = 11;
    _pluginConfViewController.unitSpacing = 4;
    _pluginConfViewController.autoAlignLabels = NO;
    _pluginConfViewController.labelFixedWidth = 120;
    _pluginConfViewController.dataSource = self.pluginPropertySheetDataSource;

    self.wwwButton.toolTip = nil;
    if (p->website
        && (!strncmp (p->website, "http://", 7)
            || !strncmp (p->website, "https://", 8))) {
        NSString *urlString = [NSString stringWithUTF8String:p->website];
        self.wwwButton.toolTip = urlString;
        self.website = [NSURL URLWithString:urlString];
    }
}

- (void)setPluginInfo:(NSInteger)idx {
    NSString *version = @"";
    NSString *description = @"";
    NSString *license = @"";

    if (idx != -1) {
        _pluginUnselectedText.hidden = YES;
        _pluginTabView.hidden = NO;
        DB_plugin_t *p = self.pluginList[idx].plugin;
        version = [NSString stringWithFormat:@"%d.%d", p->version_major, p->version_minor];
        if (p->descr) {
            description = [NSString stringWithUTF8String:p->descr];
        }
        if (p->copyright) {
            license = [NSString stringWithUTF8String:p->copyright];
        }

        [self initPluginConfiguration:idx];
    }
    else {
        _pluginUnselectedText.hidden = NO;
        _pluginTabView.hidden = YES;
    }

    _pluginVersion.stringValue = version;
    NSAttributedString *str = [[NSAttributedString alloc] initWithString:description attributes:@{NSForegroundColorAttributeName:NSColor.controlTextColor}];
    [_pluginDescription textStorage].attributedString = str;
    _pluginDescription.string = description;
    _pluginLicense.string = license;
}

#pragma mark - NSTableViewDataSource
- (NSInteger)numberOfRowsInTableView:(NSTableView *)aTableView {
    return self.pluginList.count;
}

- (NSView *)tableView:(NSTableView *)tableView viewForTableColumn:(NSTableColumn *)tableColumn row:(NSInteger)row {
    NSTableCellView *view = [tableView makeViewWithIdentifier:tableColumn.identifier owner:self];
    NSString *pluginName = [NSString stringWithUTF8String: self.pluginList[row].plugin->name];

    view.textField.stringValue = pluginName;

    const char *plugindir = deadbeef->get_system_dir (DDB_SYS_DIR_PLUGIN);
    const char *pluginpath = deadbeef->plug_get_path_for_plugin_ptr (self.pluginList[row].plugin);
    if (!pluginpath) {
        pluginpath = plugindir;
    }
    BOOL isSystemPlugin = strstr(pluginpath, plugindir) != NULL;
    if (!isSystemPlugin) {
        view.textField.font = [NSFontManager.sharedFontManager convertFont:[NSFont systemFontOfSize:NSFont.systemFontSize] toHaveTrait:NSBoldFontMask];
    }
    else {
        view.textField.font = [NSFont systemFontOfSize:NSFont.systemFontSize];
    }

    return view;
}


#pragma mark - NSTableViewDelegate

- (void)tableViewSelectionDidChange:(NSNotification *)aNotification {
    self.pluginInfo = [_pluginsTableView selectedRow];
}


@end
