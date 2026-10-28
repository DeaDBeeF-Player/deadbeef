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

/*
    WARNING: this API is experimental. It is not final, and it is subject to change at any time,
    including incompatible changes which can break existing plugins.
    Use it at your own risk. It is not recommended to publish cocoaui plugins yet.
*/

#ifndef cocoaui_api_h
#define cocoaui_api_h

#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>
#include <deadbeef/deadbeef.h>

NS_ASSUME_NONNULL_BEGIN

/// The handle that cocoaui gives to a plugin widget, to call back into the host.
/// Plugins should keep it in a weak reference.
@protocol DdbCocoaWidgetHostProtocol <NSObject>

/// Call this after changing anything that is saved by @c serializedSettingsDictionary,
/// so that the layout is saved.
- (void)layoutDidChange;

@end

/// Implemented by the plugin to provide a widget which can be placed in the design mode layout.
/// Only @c view is required. All the other methods are optional, and the defaults are provided by cocoaui.
/// The widget type and display name are taken from the registration.
@protocol DdbCocoaWidgetProtocol <NSObject>

/// The view to show in the layout. Requested once, right after the widget is created.
@property (nonatomic,readonly) NSView *view;

@optional

/// Called after the widget and its settings have been loaded as part of the layout.
- (void)didFinishLoading;

/// The settings to save in the layout. Property list types only.
- (nullable NSDictionary *)serializedSettingsDictionary;

/// Restore the settings saved by @c serializedSettingsDictionary.
- (BOOL)deserializeFromSettingsDictionary:(nullable NSDictionary *)dictionary;

/// Player events, same as @c DB_plugin_t message.
- (void)message:(uint32_t)_id ctx:(uintptr_t)ctx p1:(uint32_t)p1 p2:(uint32_t)p2;

/// Additional items for the widget's design mode menu.
@property (nonatomic,readonly) NSArray<NSMenuItem *> *menuItems;

/// Move the keyboard focus to the widget. Return YES if it was handled.
- (BOOL)makeFirstResponder;

/// Called before the widget is destroyed. Remove observers etc.
- (void)cleanup;

@end

/// Objective-C API exposed by cocoaui to other plugins.
/// This header only declares protocols, so plugins can use it without linking to the app.
@protocol DdbCocoaUIProtocol <NSObject>

/// Register a widget type, so that it can be added to the design mode layout.
/// Must be called on the main thread.
/// @param type The type name, which is saved in the layout. It must be unique, and must not change.
/// @param displayName The name to show in the UI.
/// @param factory Called each time a widget of this type needs to be created.
/// It receives the host handle, which should be kept in a weak reference.
- (void)registerWidgetType:(NSString *)type
               displayName:(NSString *)displayName
                   factory:(id<DdbCocoaWidgetProtocol> (^)(id<DdbCocoaWidgetHostProtocol> host))factory;

/// Unregister a widget type previously registered with @c registerWidgetType:displayName:factory: .
/// Plugins should call this when they stop. Must be called on the main thread.
/// Widgets of this type which already exist are not removed from the layout.
/// @param type The type name that was used for the registration.
- (void)unregisterWidgetType:(NSString *)type;

@end

typedef struct {
    DB_gui_t gui;

    /// Returns the object implementing @c DdbCocoaUIProtocol.
    id<DdbCocoaUIProtocol> _Nullable (* _Nullable get_api) (void);
} ddb_cocoaui_api_t;

NS_ASSUME_NONNULL_END

#endif /* cocoaui_api_h */
