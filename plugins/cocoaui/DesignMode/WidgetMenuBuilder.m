//
//  WidgetMenuBuilder.m
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 21/02/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//

#import "WidgetMenuBuilder.h"
#import "WidgetFactory.h"

@interface WidgetMenuBuilder()

@property (nonatomic) id<WidgetFactoryProtocol> widgetFactory;
@property (nonatomic,weak) id<WidgetProtocol> activeWidget;

@end

@implementation WidgetMenuBuilder

+ (WidgetMenuBuilder *)sharedInstance {
    static WidgetMenuBuilder *instance;
    if (instance == nil) {
        instance = [[WidgetMenuBuilder alloc] initWithWidgetFactory:WidgetFactory.sharedFactory];
    }
    return instance;
}

- (instancetype)init {
    return [self initWithWidgetFactory:nil];
}

- (instancetype)initWithWidgetFactory:(id<WidgetFactoryProtocol>)widgetFactory {
    self = [super init];

    if (self == nil) {
        return nil;
    }

    _widgetFactory = widgetFactory;

    return self;
}

- (NSMenu *)menuForWidget:(id<WidgetProtocol>)widget {
    self.activeWidget = widget;

    NSMenu *menu = [NSMenu new];
    menu.autoenablesItems = NO;

    BOOL canInsert = widget.canInsert;

    NSMenuItem *itemCreate;
    if (!canInsert) {
        itemCreate = [[NSMenuItem alloc] initWithTitle:@"Replace with…" action:nil keyEquivalent:@""];
    }
    else {
        itemCreate = [[NSMenuItem alloc] initWithTitle:@"Insert…" action:nil keyEquivalent:@""];
    }
    [menu addItem:itemCreate];

    NSMenu *menuCreate = [NSMenu new];
    menuCreate.autoenablesItems = NO;
    itemCreate.submenu = menuCreate;

    NSArray<NSString *> *types = self.widgetFactory.types;

    for (NSString *type in types) {
        if ([type isEqualToString:@"Placeholder"]) {
            continue;
        }
        NSMenuItem *item = [[NSMenuItem alloc] initWithTitle:type action:nil keyEquivalent:@""];
        item.target = self;
        item.action = @selector(createWidget:);
        item.representedObject = type;
        [menuCreate addItem:item];
    }

    [menu addItem: NSMenuItem.separatorItem];

    if (!canInsert) {
        NSMenuItem *itemDelete = [[NSMenuItem alloc] initWithTitle:@"Delete" action:nil keyEquivalent:@""];
        itemDelete.target = self;
        itemDelete.action = @selector(deleteWidget:);
        [menu addItem: itemDelete];
        NSMenuItem *itemCut = [[NSMenuItem alloc] initWithTitle:@"Cut" action:nil keyEquivalent:@""];
        [menu addItem: itemCut];
        NSMenuItem *itemCopy = [[NSMenuItem alloc] initWithTitle:@"Copy" action:nil keyEquivalent:@""];
        [menu addItem: itemCopy];
    }
    NSMenuItem *itemPaste = [[NSMenuItem alloc] initWithTitle:@"Paste" action:nil keyEquivalent:@""];
    [menu addItem: itemPaste];

    // TODO: Widget custom menu (options)
    // TODO: Parent menu for the child (options)

    return menu;
}

- (void)createWidget:(NSMenuItem *)sender {
    NSString *type = sender.representedObject;
    id<WidgetProtocol> widget = [self.widgetFactory createWidgetWithType:type];

    id<WidgetProtocol> activeWidget = self.activeWidget;

    if (activeWidget.canInsert) {
        [activeWidget appendChild:widget];
    }
    else {
        id<WidgetProtocol> parentWidget = activeWidget.parentWidget;
        if (activeWidget != nil && widget != nil) {
            [parentWidget replaceChild:activeWidget withChild:widget];
        }
    }
}

- (void)deleteWidget:(NSMenuItem *)sender {
    id<WidgetProtocol> activeWidget = self.activeWidget;
    id<WidgetProtocol> parentWidget = activeWidget.parentWidget;

    if (activeWidget != nil && parentWidget != nil) {
        [parentWidget removeChild:activeWidget];
    }
}

@end
