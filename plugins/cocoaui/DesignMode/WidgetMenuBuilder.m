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

#pragma mark -

// To be used as representedObject
@interface MenuItemData : NSObject

@property (nonatomic,weak) id<WidgetProtocol> targetWidget;
@property (nonatomic,weak) NSString *createType;

@end

@implementation MenuItemData
@end

#pragma mark -

@interface WidgetMenuBuilder()

@property (nonatomic,weak) id<DesignModeDepsProtocol> deps;

@end

@implementation WidgetMenuBuilder

+ (WidgetMenuBuilder *)sharedInstance {
    static WidgetMenuBuilder *instance;
    if (instance == nil) {
        instance = [[WidgetMenuBuilder alloc] initWithDeps:DesignModeDeps.sharedInstance];
    }
    return instance;
}

- (instancetype)initWithDeps:(id<DesignModeDepsProtocol>)deps {
    self = [super init];

    if (self == nil) {
        return nil;
    }

    _deps = deps;

    return self;
}

- (NSMenu *)menuForWidget:(id<WidgetProtocol>)widget includeParentMenu:(BOOL)includeParentMenu {
    NSString *widgetType = widget.widgetType;
    if (widgetType == nil) {
        return nil;
    }

    NSString *displayName = [self.deps.factory displayNameForType:widgetType];
    if (displayName == nil) {
        if ([widget respondsToSelector:@selector(displayName)]) {
            displayName = widget.displayName;
        }
        if (displayName == nil) {
            return nil;
        }
    }

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

    MenuItemData *sharedMenuItemData = [MenuItemData new];
    sharedMenuItemData.targetWidget = widget;

    for (NSString *type in types) {
        if ([type isEqualToString:@"Placeholder"]) {
            continue;
        }
        NSString *typeDisplayName = [self.deps.factory displayNameForType:type];
        NSMenuItem *item = [[NSMenuItem alloc] initWithTitle:typeDisplayName action:nil keyEquivalent:@""];
        item.target = self;
        item.action = @selector(createWidget:);
        MenuItemData *data = [MenuItemData new];
        data.targetWidget = widget;
        data.createType = type;
        item.representedObject = data;
        [menuCreate addItem:item];
    }

    [menu addItem: NSMenuItem.separatorItem];

    NSMenuItem *itemDelete = [[NSMenuItem alloc] initWithTitle:@"Delete" action:@selector(deleteWidget:) keyEquivalent:@""];
    itemDelete.representedObject = sharedMenuItemData;
    itemDelete.enabled = !isPlaceholder;
    itemDelete.target = self;
    [menu addItem: itemDelete];

    NSMenuItem *itemCut = [[NSMenuItem alloc] initWithTitle:@"Cut" action:@selector(cutWidget:) keyEquivalent:@""];
    itemCut.representedObject = sharedMenuItemData;
    itemCut.enabled = !isPlaceholder;
    itemCut.target = self;
    [menu addItem: itemCut];

    NSMenuItem *itemCopy = [[NSMenuItem alloc] initWithTitle:@"Copy" action:@selector(copyWidget:) keyEquivalent:@""];
    itemCopy.representedObject = sharedMenuItemData;
    itemCopy.enabled = !isPlaceholder;
    itemCopy.target = self;
    [menu addItem: itemCopy];

    NSMenuItem *itemPaste = [[NSMenuItem alloc] initWithTitle:@"Paste" action:@selector(pasteWidget:) keyEquivalent:@""];
    itemPaste.representedObject = sharedMenuItemData;
    itemPaste.target = self;

    NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
    BOOL hasClipboard = [pasteboard canReadItemWithDataConformingToTypes:@[ddbWidgetUTIType]];
    if (!hasClipboard) {
        itemPaste.enabled = NO;
    }

    [menu addItem: itemPaste];

    if (includeParentMenu) {
        id<WidgetProtocol> parentWidget = widget.parentWidget;
        if (parentWidget != nil && parentWidget != self.deps.state.rootWidget) {
            [menu addItem: NSMenuItem.separatorItem];

            NSMenuItem *itemParent = [[NSMenuItem alloc] initWithTitle:@"Parent…" action:nil keyEquivalent:@""];
            NSMenu *menuParent = [self menuForWidget:parentWidget includeParentMenu:NO];
            itemParent.submenu = menuParent;

            [menu addItem:itemParent];
        }
    }

    NSArray<NSMenuItem *> *additionalMenuItems;
    if ([widget respondsToSelector:@selector(menuItems)]) {
        additionalMenuItems = widget.menuItems;
    }

    if (additionalMenuItems.count) {
        [menu addItem: NSMenuItem.separatorItem];

        for (NSMenuItem *menuItem in additionalMenuItems) {
            [menu addItem:menuItem];
        }
    }

    return menu;
}

- (void)replaceWidget:(id<WidgetProtocol>)widget withNewWidget:(id<WidgetProtocol>)newWidget {
    if (widget.parentWidget == nil) {
        [widget removeChild:widget.childWidgets.firstObject];
        [widget appendChild:newWidget];
    }
    else {
        id<WidgetProtocol> parentWidget = widget.parentWidget;
        if (widget != nil && newWidget != nil) {
            [parentWidget replaceChild:widget withChild:newWidget];
        }
    }

    [self.deps.state layoutDidChange];
}

- (void)createWidget:(NSMenuItem *)sender {
    MenuItemData *data = sender.representedObject;
    id<WidgetProtocol> widget = [self.deps.factory createWidgetWithType:data.createType];
    [self replaceWidget:data.targetWidget withNewWidget:widget];
}

- (void)deleteWidget:(NSMenuItem *)sender {
    MenuItemData *data = sender.representedObject;
    id<WidgetProtocol> parentWidget = data.targetWidget.parentWidget;

    if (data.targetWidget != nil && parentWidget != nil) {
        [parentWidget removeChild:data.targetWidget];
        [self.deps.state layoutDidChange];
    }
}

- (void)cutWidget:(NSMenuItem *)sender {
    [self copyWidget:sender];
    [self deleteWidget:sender];
}

- (void)copyWidget:(NSMenuItem *)sender {
    MenuItemData *menuItemdata = sender.representedObject;

    NSDictionary *widgetDict = [self.deps.serializer saveWidgetToDictionary:menuItemdata.targetWidget];
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

        MenuItemData *menuItemdata = sender.representedObject;
        [self replaceWidget:menuItemdata.targetWidget withNewWidget:widget];
    }
}

@end
