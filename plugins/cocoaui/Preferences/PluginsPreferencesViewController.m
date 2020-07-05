//
//  PluginsPreferencesViewController.m
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 2/24/20.
//  Copyright © 2020 Alexey Yakovenko. All rights reserved.
//

#import "PluginsPreferencesViewController.h"
#import "PropertySheetViewController.h"
#include "deadbeef.h"

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
@property (unsafe_unretained) IBOutlet NSTextField *pluginUnselectedText;
@property (unsafe_unretained) IBOutlet NSTabView *pluginTabView;

@property (unsafe_unretained) IBOutlet NSTableView *pluginList;
@property (unsafe_unretained) IBOutlet NSTextField *pluginVersion;
@property (unsafe_unretained) IBOutlet NSTextView *pluginDescription;
@property (unsafe_unretained) IBOutlet NSTextView *pluginLicense;

@property (strong) IBOutlet PropertySheetViewController *pluginConfViewController;

@property (weak) IBOutlet NSButton *wwwButton;
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
}

- (void)initPluginList {
    self.pluginList.dataSource = self;
    self.pluginList.delegate = self;
    self.pluginInfo = -1;
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
    DB_plugin_t *p = deadbeef->plug_get_list()[idx];

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
        DB_plugin_t **p = deadbeef->plug_get_list();
        version = [NSString stringWithFormat:@"%d.%d", p[idx]->version_major, p[idx]->version_minor];
        if (p[idx]->descr) {
            description = [NSString stringWithUTF8String:p[idx]->descr];
        }
        if (p[idx]->copyright) {
            license = [NSString stringWithUTF8String:p[idx]->copyright];
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

// data source for plugin list
- (NSInteger)numberOfRowsInTableView:(NSTableView *)aTableView {
    DB_plugin_t **p = deadbeef->plug_get_list();
    int cnt;
    for (cnt = 0; p[cnt]; cnt++);

    return cnt;
}

- (id)tableView:(NSTableView *)aTableView objectValueForTableColumn:(NSTableColumn *)aTableColumn row:(NSInteger)rowIndex {
    DB_plugin_t **p = deadbeef->plug_get_list();

    return [NSString stringWithUTF8String: p[rowIndex]->name];
}

- (void)tableViewSelectionDidChange:(NSNotification *)aNotification {
    self.pluginInfo = [_pluginList selectedRow];
}


@end
