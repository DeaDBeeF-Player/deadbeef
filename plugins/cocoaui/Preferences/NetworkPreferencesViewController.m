//
//  NetworkPreferencesViewController.m
//  DeaDBeeF
//
//  Created by Oleksiy Yakovenko on 2/23/20.
//  Copyright © 2020 Oleksiy Yakovenko. All rights reserved.
//

#import "DdbShared.h"
#import "NetworkPreferencesViewController.h"
#include "ctmap.h"
#include <deadbeef/deadbeef.h>

extern DB_functions_t *deadbeef;

static NSString *kContentTypeMappingChangedNotification = @"ContentTypeMappingChanged";

@interface ContentTypeMap : NSObject

- (instancetype)init NS_UNAVAILABLE;
- (instancetype)initWithContentType:(NSString *)contentType plugins:(NSString *)plugins;

@property (nonatomic) NSString *contentType;
@property (nonatomic) NSString *plugins;

@end


@implementation ContentTypeMap

- (instancetype)initWithContentType:(NSString *)contentType plugins:(NSString *)plugins {
    self = [super init];

    _contentType = contentType;
    _plugins = plugins;

    return self;
}

- (void)setContentType:(NSString *)contentType {
    _contentType = contentType;
    [NSNotificationCenter.defaultCenter postNotificationName:kContentTypeMappingChangedNotification object:self];
}

- (void)setPlugins:(NSString *)plugins {
    _plugins = plugins;
    [NSNotificationCenter.defaultCenter postNotificationName:kContentTypeMappingChangedNotification object:self];
}

@end

#pragma mark -

@interface NetworkPreferencesViewController ()

@property (nonatomic) BOOL enableNetworkProxy;
@property (nonatomic) NSString *networkProxyAddress;
@property (nonatomic) NSUInteger networkProxyPort;
@property (nonatomic) NSUInteger networkProxyType;
@property (nonatomic) NSString *networkProxyUserName;
@property (nonatomic) NSString *networkProxyPassword;
@property (nonatomic) NSString *networkProxyUserAgent;

@property (weak) IBOutlet NSArrayController *contentTypeMappingArrayController;
@property (weak) IBOutlet NSTableView *contentTypeMappingTableView;


@end

@implementation NetworkPreferencesViewController

- (instancetype)initWithCoder:(NSCoder *)coder
{
    self = [super initWithCoder:coder];
    if (!self) {
        return nil;
    }
    // network
    _enableNetworkProxy = deadbeef->conf_get_int ("network.proxy", 0);

    _networkProxyAddress = conf_get_nsstr ("network.proxy.address", "");

    _networkProxyPort = deadbeef->conf_get_int ("network.proxy.port", 8080);

    NSString *t = conf_get_nsstr ("network.proxy.type", "HTTP");
    const char *type = t.UTF8String;
    if (!strcasecmp (type, "HTTP")) {
        _networkProxyType = 0;
    }
    else if (!strcasecmp (type, "HTTP_1_0")) {
        _networkProxyType = 1;
    }
    else if (!strcasecmp (type, "SOCKS4")) {
        _networkProxyType = 2;
    }
    else if (!strcasecmp (type, "SOCKS5")) {
        _networkProxyType = 3;
    }
    else if (!strcasecmp (type, "SOCKS4A")) {
        _networkProxyType = 4;
    }
    else if (!strcasecmp (type, "SOCKS5_HOSTNAME")) {
        _networkProxyType = 5;
    }

    _networkProxyUserName = conf_get_nsstr ("network.proxy.username", "");

    _networkProxyPassword = conf_get_nsstr ("network.proxy.password", "");

    _networkProxyUserAgent = conf_get_nsstr ("network.http_user_agent", "");

    return self;
}

- (void)viewDidLoad {
    [super viewDidLoad];

    // Content-type mapping
    [self initContentTypeMapping];

    [NSNotificationCenter.defaultCenter addObserver:self selector:@selector(contentTypeMappingChanged:) name:kContentTypeMappingChangedNotification object:nil];

}

- (void)contentTypeMappingChanged:(ContentTypeMap *)sender {
    NSArray<ContentTypeMap *> *objects = self.contentTypeMappingArrayController.arrangedObjects;

    char mapstr[2048] = "";
    int s = sizeof (mapstr);
    char *p = mapstr;

    for (ContentTypeMap *m in objects) {
        const char *ct = m.contentType ? m.contentType.UTF8String : "";
        const char *plugins = m.plugins ? m.plugins.UTF8String : "";
        int l = snprintf (p, s, "\"%s\" {%s} ", ct, plugins);
        p += l;
        s -= l;
        if (s <= 0) {
            break;
        }
    }

    deadbeef->conf_set_str ("network.ctmapping", mapstr);
    deadbeef->conf_save ();

    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

- (IBAction)resetContentTypeMapping:(id)sender {
    deadbeef->conf_set_str ("network.ctmapping", DDB_DEFAULT_CTMAPPING);
    [self initContentTypeMapping];
}


- (IBAction)segmentedControlAction:(NSSegmentedControl *)sender {
    NSInteger selectedSegment = [sender selectedSegment];

    switch (selectedSegment) {
    case 0:
        [self.contentTypeMappingArrayController add:sender];
        break;
    case 1:
        [self.contentTypeMappingArrayController remove:sender];
        break;
    }
}

- (void)initContentTypeMapping {
    [self.contentTypeMappingArrayController setContent:nil];

    char ctmap_str[2048];
    deadbeef->conf_get_str ("network.ctmapping", DDB_DEFAULT_CTMAPPING, ctmap_str, sizeof (ctmap_str));
    ddb_ctmap_t *ctmap = ddb_ctmap_init_from_string (ctmap_str);
    if (ctmap) {

        ddb_ctmap_t *m = ctmap;
        while (m) {
            NSString *plugins = @"";
            for (int i = 0; m->plugins[i]; i++) {
                if (i != 0) {
                    plugins = [plugins stringByAppendingString:@" "];
                }
                plugins = [plugins stringByAppendingString:[NSString stringWithUTF8String:m->plugins[i]]];
            }


            ContentTypeMap *map = [[ContentTypeMap alloc] initWithContentType:[NSString stringWithUTF8String:m->ct] plugins:plugins];
            [self.contentTypeMappingArrayController addObject:map];

            m = m->next;
        }

        ddb_ctmap_free (ctmap);
    }

    [self.contentTypeMappingTableView reloadData];
}

- (void)setEnableNetworkProxy:(BOOL)enableNetworkProxy {
    _enableNetworkProxy = enableNetworkProxy;
    deadbeef->conf_set_int ("network.proxy", enableNetworkProxy);
    deadbeef->sendmessage(DB_EV_CONFIGCHANGED, 0, 0, 0);
}

- (void)setNetworkProxyAddress:(NSString *)networkProxyAddress {
    _networkProxyAddress = networkProxyAddress;
    deadbeef->conf_set_str ("network.proxy.address", (networkProxyAddress?:@"").UTF8String);
    deadbeef->sendmessage(DB_EV_CONFIGCHANGED, 0, 0, 0);
}

- (void)setNetworkProxyPort:(NSUInteger)networkProxyPort {
    _networkProxyPort = networkProxyPort;
    deadbeef->conf_set_int ("network.proxy.port", (int)(networkProxyPort?:8080));
    deadbeef->sendmessage(DB_EV_CONFIGCHANGED, 0, 0, 0);
}

- (void)setNetworkProxyType:(NSUInteger)networkProxyType {
    _networkProxyType = networkProxyType;
    const char *type = NULL;
    switch (networkProxyType) {
    case 0:
        type = "HTTP";
        break;
    case 1:
        type = "HTTP_1_0";
        break;
    case 2:
        type = "SOCKS4";
        break;
    case 3:
        type = "SOCKS5";
        break;
    case 4:
        type = "SOCKS4A";
        break;
    case 5:
        type = "SOCKS5_HOSTNAME";
        break;
    default:
        type = "HTTP";
        break;
    }

    deadbeef->conf_set_str ("network.proxy.type", type);
    deadbeef->sendmessage(DB_EV_CONFIGCHANGED, 0, 0, 0);
}

- (void)setNetworkProxyUserName:(NSString *)networkProxyUserName {
    _networkProxyUserName = networkProxyUserName;
    deadbeef->conf_set_str ("network.proxy.username", (networkProxyUserName?:@"").UTF8String);
    deadbeef->sendmessage(DB_EV_CONFIGCHANGED, 0, 0, 0);
}

- (void)setNetworkProxyPassword:(NSString *)networkProxyPassword {
    _networkProxyPassword = networkProxyPassword;
    deadbeef->conf_set_str ("network.proxy.password", (networkProxyPassword?:@"").UTF8String);
    deadbeef->sendmessage(DB_EV_CONFIGCHANGED, 0, 0, 0);
}

- (void)setNetworkProxyUserAgent:(NSString *)networkProxyUserAgent {
    _networkProxyUserAgent = networkProxyUserAgent;
    deadbeef->conf_set_str ("network.http_user_agent", (networkProxyUserAgent?:@"").UTF8String);
    deadbeef->sendmessage(DB_EV_CONFIGCHANGED, 0, 0, 0);
}

@end
