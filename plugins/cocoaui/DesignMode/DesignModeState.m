//
//  DesignModeState.m
//  DeaDBeeF
//
//  Created by Oleksiy Yakovenko on 21/02/2021.
//  Copyright © 2021 Oleksiy Yakovenko. All rights reserved.
//

#import "DesignModeState.h"
#import "DesignModeDeps.h"
#include <deadbeef/deadbeef.h>

extern DB_functions_t *deadbeef;

static DesignModeState *_sharedInstance;

@interface DesignModeState()

@property (nonatomic,weak) id<DesignModeDepsProtocol> deps;
@property (nonatomic,readwrite) id<WidgetProtocol> rootWidget;
@property (nonatomic) BOOL isLoading;
@property (nonatomic,copy) NSDictionary *previousLayout;
@property (nonatomic) NSInteger coalescingUndoDepth;
@property (nonatomic) BOOL coalescedUndoRegistered;

- (BOOL)restoreLayoutDictionary:(NSDictionary *)layout actionName:(NSString *)actionName;
- (void)layoutDidChange:(nullable NSString *)actionName registerUndo:(BOOL)registerUndo;

@end

@implementation DesignModeState

+ (DesignModeState *)sharedInstance {
    if (_sharedInstance == nil) {
        _sharedInstance = [[DesignModeState alloc] initWithDeps:DesignModeDeps.sharedInstance];
    }

    return _sharedInstance;
}

+ (void)freeSharedInstance {
    _sharedInstance = nil;
}

- (void)cleanupWidget:(id<WidgetProtocol>)widget {
    for (id<WidgetProtocol> childWidget in widget.childWidgets) {
        [self cleanupWidget:childWidget];
    }
    if ([widget respondsToSelector:@selector(cleanup)]) {
        [widget cleanup];
    }
}

- (void)dealloc
{
    [self cleanupWidget:_rootWidget];
}

- (instancetype)initWithDeps:(id<DesignModeDepsProtocol>)deps {
    self = [super init];

    if (self == nil) {
        return nil;
    }

    _deps = deps;
    return self;
}

- (void)load {
    self.isLoading = YES;
    self.rootWidget = [self.deps.factory createWidgetWithType:@"Placeholder"];

    char *layout = malloc (100000);
    deadbeef->conf_get_str ("cocoaui.layout", "", layout, 100000);
    NSString *strLayout = @(layout);
    free (layout);

    NSData *data = [strLayout dataUsingEncoding:NSUTF8StringEncoding];

    NSError *err = nil;
    NSDictionary *json = [NSJSONSerialization JSONObjectWithData:data options:0 error:&err];

    id<WidgetProtocol> layoutWidget;
    if (json && [json isKindOfClass:NSDictionary.class]) {
        layoutWidget = [self.deps.serializer loadFromDictionary:json];
    }

    if (!layoutWidget) {
        layoutWidget = [self.deps.serializer loadFromDictionary:@{
            @"type":@"PlaylistWithTabs"
        }];
    }


    if (!layoutWidget) {
        // bummer...
    }

    [self.rootWidget appendChild:layoutWidget];

    // Always have a valid previous layout, so that the first change is undoable.
    self.previousLayout = json ?: [self currentLayoutDictionary];

    self.isLoading = NO;
}

- (void)layoutDidChange {
    [self layoutDidChangeWithActionName:nil];
}

- (void)beginCoalescingUndo {
    if (self.coalescingUndoDepth++ == 0) {
        self.coalescedUndoRegistered = NO;
    }
}

- (void)endCoalescingUndo {
    if (self.coalescingUndoDepth > 0) {
        self.coalescingUndoDepth--;
    }
}

- (void)registerUndoForLayout:(NSDictionary *)oldLayout actionName:(NSString *)actionName {
    NSUndoManager *undoManager = self.deps.undoManager;
    if (undoManager == nil || oldLayout == nil) {
        return;
    }

    // While coalescing, the first registered operation already holds the layout from before the first change.
    // Undo and redo are never coalesced.
    BOOL isUndoOrRedo = undoManager.isUndoing || undoManager.isRedoing;
    if (self.coalescingUndoDepth > 0 && !isUndoOrRedo) {
        if (self.coalescedUndoRegistered) {
            return;
        }
        self.coalescedUndoRegistered = YES;
    }

    NSString *name = [actionName copy] ?: @"Change Layout";
    [undoManager registerUndoWithTarget:self handler:^(DesignModeState *target) {
        // Registers the inverse operation (redo) via layoutDidChange.
        [target restoreLayoutDictionary:oldLayout actionName:name];
    }];
    [undoManager setActionName:name];
}

- (void)layoutDidChangeWithoutUndo {
    [self layoutDidChange:nil registerUndo:NO];
}

- (void)layoutDidChangeWithActionName:(NSString *)actionName {
    [self layoutDidChange:actionName registerUndo:YES];
}

- (void)layoutDidChange:(NSString *)actionName registerUndo:(BOOL)registerUndo {
    if (self.isLoading) {
        return;
    }
    NSDictionary *dict = [self.deps.serializer saveWidgetToDictionary:self.rootWidget.childWidgets.firstObject];

    if ([dict isEqual:self.previousLayout]) {
        return;
    }

    NSDictionary *oldLayout = self.previousLayout;
    self.previousLayout = dict;
    if (registerUndo) {
        [self registerUndoForLayout:oldLayout actionName:actionName];
    }

    NSError *err;
    NSData *dt = [NSJSONSerialization dataWithJSONObject:dict options:0 error:&err];
    NSString *json = [[NSString alloc] initWithData:dt encoding:NSUTF8StringEncoding];

    deadbeef->conf_set_str ("cocoaui.layout", json.UTF8String);
    deadbeef->conf_save ();
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

- (NSDictionary *)currentLayoutDictionary {
    return [self.deps.serializer saveWidgetToDictionary:self.rootWidget.childWidgets.firstObject];
}

- (BOOL)applyLayoutDictionary:(NSDictionary *)layout {
    return [self restoreLayoutDictionary:layout actionName:@"Apply Layout"];
}

- (BOOL)restoreLayoutDictionary:(NSDictionary *)layout actionName:(NSString *)actionName {
    id<WidgetProtocol> newWidget = [self.deps.serializer loadFromDictionary:layout];
    if (newWidget == nil) {
        return NO;
    }

    id<WidgetProtocol> oldWidget = self.rootWidget.childWidgets.firstObject;
    if (oldWidget != nil) {
        [self.rootWidget removeChild:oldWidget];
        [self cleanupWidget:oldWidget];
    }
    [self.rootWidget appendChild:newWidget];

    [self layoutDidChangeWithActionName:actionName];
    return YES;
}

@end
