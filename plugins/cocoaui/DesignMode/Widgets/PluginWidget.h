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

#import "WidgetBase.h"
#import "cocoaui_api.h"

NS_ASSUME_NONNULL_BEGIN

typedef id<DdbCocoaWidgetProtocol> _Nonnull (^PluginWidgetFactoryBlock)(id<DdbCocoaWidgetHostProtocol> host);

/// Hosts a widget implemented by a plugin.
/// All the boilerplate is inherited from WidgetBase, and the plugin's object is called
/// for the methods which it chooses to implement.
@interface PluginWidget : WidgetBase<DdbCocoaWidgetHostProtocol>

- (instancetype)initWithDeps:(id<DesignModeDepsProtocol>)deps NS_UNAVAILABLE;

/// @param type The type name that the widget is registered with. It's used for serialization.
/// @param factory Called once to create the plugin's widget object.
- (instancetype)initWithDeps:(id<DesignModeDepsProtocol>)deps type:(NSString *)type factory:(PluginWidgetFactoryBlock)factory NS_DESIGNATED_INITIALIZER;

@end

NS_ASSUME_NONNULL_END
