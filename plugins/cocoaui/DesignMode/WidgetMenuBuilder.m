//
//  WidgetMenuBuilder.m
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 21/02/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//

#import "DesignModeDeps.h"
#import "WidgetMenuBuilder.h"

static NSPasteboardType const ddbWidgetUTIType = @"org.deadbeef.widget";

@interface WidgetMenuBuilder()

@property (nonatomic,weak) id<DesignModeDepsProtocol> deps;
@property (nonatomic,weak) id<WidgetProtocol> activeWidget;

@end

@implementation WidgetMenuBuilder

+ (WidgetMenuBuilder *)sharedInstance {
    static WidgetMenuBuilder *instance;
    if (instance == nil) {
        instance = [[WidgetMenuBuilder alloc] initWithDeps:DesignModeDeps.sharedInstance];
    }
    return instance;
}

- (instancetype)init {
    return [self initWithDeps:nil];
}

- (instancetype)initWithDeps:(id<DesignModeDepsProtocol>)deps {
    self = [super init];

    if (self == nil) {
        return nil;
    }

    _deps = deps;

    return self;
}

- (NSMenu *)menuForWidget:(id<WidgetProtocol>)widget {
    self.activeWidget = nil;
    NSString *widgetType = widget.widgetType;
    if (widgetType == nil) {
        return nil;
    }

    NSString *displayName = [self.deps.factory displayNameForType:widgetType];
    if (displayName == nil) {
        return nil;
    }

    self.activeWidget = widget;

    NSMenu *menu = [NSMenu new];
    menu.autoenablesItems = NO;

    BOOL isPlaceholder = widget.isPlaceholder;

    // Title
    NSMenuItem *itemTitle = [[NSMenuItem alloc] initWithTitle:displayName action:nil keyEquivalent:@""];
    itemTitle.enabled = NO;
    [menu addItem:itemTitle];
    [menu addItem: NSMenuItem.separatorItem];

    // placeholder: always replace, unless root
    NSMenuItem *itemCreate;
    if (!isPlaceholder) {
        itemCreate = [[NSMenuItem alloc] initWithTitle:@"Replace with…" action:nil keyEquivalent:@""];
    }
    else {
        itemCreate = [[NSMenuItem alloc] initWithTitle:@"Insert…" action:nil keyEquivalent:@""];
    }
    [menu addItem:itemCreate];

    NSMenu *menuCreate = [NSMenu new];
    menuCreate.autoenablesItems = NO;
    itemCreate.submenu = menuCreate;

    NSArray<NSString *> *types = self.deps.factory.types;

    for (NSString *type in types) {
        if ([type isEqualToString:@"Placeholder"]) {
            continue;
        }
        NSString *typeDisplayName = [self.deps.factory displayNameForType:type];
        NSMenuItem *item = [[NSMenuItem alloc] initWithTitle:typeDisplayName action:nil keyEquivalent:@""];
        item.target = self;
        item.action = @selector(createWidget:);
        item.representedObject = type;
        [menuCreate addItem:item];
    }

    [menu addItem: NSMenuItem.separatorItem];

    NSMenuItem *itemDelete = [[NSMenuItem alloc] initWithTitle:@"Delete" action:@selector(deleteWidget:) keyEquivalent:@""];
    itemDelete.target = self;
    [menu addItem: itemDelete];

    NSMenuItem *itemCut = [[NSMenuItem alloc] initWithTitle:@"Cut" action:@selector(cutWidget:) keyEquivalent:@""];
    itemCut.enabled = !isPlaceholder;
    itemCut.target = self;
    [menu addItem: itemCut];

    NSMenuItem *itemCopy = [[NSMenuItem alloc] initWithTitle:@"Copy" action:@selector(copyWidget:) keyEquivalent:@""];
    itemCut.enabled = !isPlaceholder;
    itemCopy.target = self;
    [menu addItem: itemCopy];

    NSMenuItem *itemPaste = [[NSMenuItem alloc] initWithTitle:@"Paste" action:@selector(pasteWidget:) keyEquivalent:@""];
    itemPaste.target = self;

    NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
    BOOL hasClipboard = [pasteboard canReadItemWithDataConformingToTypes:@[ddbWidgetUTIType]];
    if (!hasClipboard) {
        itemPaste.enabled = NO;
    }

    [menu addItem: itemPaste];

    // TODO: Widget custom menu (options)
    // TODO: Parent menu for the child (options)

    return menu;
}

- (void)replaceActiveWidgetWith:(id<WidgetProtocol>)widget {
    id<WidgetProtocol> activeWidget = self.activeWidget;
    if (activeWidget.parentWidget == nil) {
        [activeWidget removeChild:activeWidget.childWidgets.firstObject];
        [activeWidget appendChild:widget];
    }
    else {
        id<WidgetProtocol> parentWidget = activeWidget.parentWidget;
        if (activeWidget != nil && widget != nil) {
            [parentWidget replaceChild:activeWidget withChild:widget];
        }
    }

    [self.deps.state layoutDidChange];
}

- (void)createWidget:(NSMenuItem *)sender {
    NSString *type = sender.representedObject;
    id<WidgetProtocol> widget = [self.deps.factory createWidgetWithType:type];


    [self replaceActiveWidgetWith:widget];
}

- (void)deleteWidget:(NSMenuItem *)sender {
    id<WidgetProtocol> activeWidget = self.activeWidget;
    id<WidgetProtocol> parentWidget = activeWidget.parentWidget;

    if (activeWidget != nil && parentWidget != nil) {
        [parentWidget removeChild:activeWidget];
        [self.deps.state layoutDidChange];
    }
}

- (void)cutWidget:(NSMenuItem *)sender {
    [self copyWidget:sender];
    [self deleteWidget:sender];
}

- (void)copyWidget:(NSMenuItem *)sender {
    id<WidgetProtocol> activeWidget = self.activeWidget;

    NSDictionary *widgetDict = [self.deps.serializer saveWidgetToDictionary:activeWidget];
    NSPasteboard *pasteboard = NSPasteboard.generalPasteboard;
    [pasteboard clearContents];
    NSData *data = [NSKeyedArchiver archivedDataWithRootObject:widgetDict];
    [pasteboard setData:data forType:ddbWidgetUTIType];
}

- (void)pasteWidget:(NSMenuItem *)sender {
    NSPasteboard *pasteboard = NSPasteboard.generalPasteboard;
    if ([pasteboard canReadItemWithDataConformingToTypes:@[ddbWidgetUTIType]]) {
        NSData *data = [pasteboard dataForType:ddbWidgetUTIType];
        if (!data) {
            return;
        }
        NSDictionary *widgetDict = (NSDictionary *)[NSKeyedUnarchiver unarchiveObjectWithData: data];
        if (!widgetDict) {
            return;
        }

        id<WidgetProtocol> widget = [self.deps.serializer loadFromDictionary:widgetDict];
        if (!widget) {
            return;
        }

        [self replaceActiveWidgetWith:widget];
    }
}

@end
