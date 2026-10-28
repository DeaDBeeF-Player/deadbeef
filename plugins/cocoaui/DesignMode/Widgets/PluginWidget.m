/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2026 Oleksiy Yakovenko and other contributors

    This software is provided 'as-is', without any express or implied
    warranty.  In no event will the authors be held liable for any damages
    arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.

    3. This notice may not be removed or altered from any source distribution.
*/

#import "PluginWidget.h"

@interface PluginWidget()

@property (nonatomic,weak) id<DesignModeDepsProtocol> deps;
@property (nonatomic,copy) NSString *type;
@property (nonatomic) id<DdbCocoaWidgetProtocol> content;

@end

@implementation PluginWidget

- (instancetype)initWithDeps:(id<DesignModeDepsProtocol>)deps type:(NSString *)type factory:(PluginWidgetFactoryBlock)factory {
    self = [super initWithDeps:deps];
    if (self == nil) {
        return nil;
    }

    _deps = deps;
    _type = [type copy];

    // The boilerplate is done at this point, so the plugin gets a fully functional host.
    // The host is passed as a protocol, the plugin is not supposed to know the class.
    _content = factory(self);

    NSView *view = _content.view;
    view.translatesAutoresizingMaskIntoConstraints = NO;
    [self.topLevelView addSubview:view];
    [view.leadingAnchor constraintEqualToAnchor:self.topLevelView.leadingAnchor].active = YES;
    [view.trailingAnchor constraintEqualToAnchor:self.topLevelView.trailingAnchor].active = YES;
    [view.topAnchor constraintEqualToAnchor:self.topLevelView.topAnchor].active = YES;
    [view.bottomAnchor constraintEqualToAnchor:self.topLevelView.bottomAnchor].active = YES;

    return self;
}

#pragma mark - DdbCocoaWidgetHostProtocol

- (void)layoutDidChange {
    [self.deps.state layoutDidChange];
}

#pragma mark - WidgetProtocol

// The type is per instance here, because the same class serves all the plugin widget types.
- (NSString *)widgetType {
    return self.type;
}

- (NSDictionary *)serializedSettingsDictionary {
    if ([self.content respondsToSelector:@selector(serializedSettingsDictionary)]) {
        return [self.content serializedSettingsDictionary];
    }
    return [super serializedSettingsDictionary];
}

- (BOOL)deserializeFromSettingsDictionary:(NSDictionary *)dictionary {
    if ([self.content respondsToSelector:@selector(deserializeFromSettingsDictionary:)]) {
        return [self.content deserializeFromSettingsDictionary:dictionary];
    }
    return [super deserializeFromSettingsDictionary:dictionary];
}

- (void)message:(uint32_t)_id ctx:(uintptr_t)ctx p1:(uint32_t)p1 p2:(uint32_t)p2 {
    if ([self.content respondsToSelector:@selector(message:ctx:p1:p2:)]) {
        [self.content message:_id ctx:ctx p1:p1 p2:p2];
        return;
    }
    [super message:_id ctx:ctx p1:p1 p2:p2];
}

#pragma mark - Optional WidgetProtocol methods

// The callers check these with respondsToSelector, so they're only reported
// as implemented if the plugin's object implements them.
- (BOOL)respondsToSelector:(SEL)aSelector {
    if (aSelector == @selector(didFinishLoading)
        || aSelector == @selector(cleanup)
        || aSelector == @selector(makeFirstResponder)
        || aSelector == @selector(menuItems)) {
        return [self.content respondsToSelector:aSelector];
    }
    return [super respondsToSelector:aSelector];
}

- (void)didFinishLoading {
    [self.content didFinishLoading];
}

- (void)cleanup {
    [self.content cleanup];
}

- (BOOL)makeFirstResponder {
    return [self.content makeFirstResponder];
}

- (NSArray<NSMenuItem *> *)menuItems {
    return self.content.menuItems;
}

@end
