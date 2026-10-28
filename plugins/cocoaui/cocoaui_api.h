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

#ifndef cocoaui_api_h
#define cocoaui_api_h

#import <Foundation/Foundation.h>
#include <deadbeef/deadbeef.h>

NS_ASSUME_NONNULL_BEGIN

/// Objective-C API exposed by cocoaui to other plugins.
/// This header only declares protocols, so plugins can use it without linking to the app.
@protocol DdbCocoaUIProtocol <NSObject>

@end

typedef struct {
    DB_gui_t gui;

    /// Returns the object implementing @c DdbCocoaUIProtocol.
    id<DdbCocoaUIProtocol> _Nullable (* _Nullable get_api) (void);
} ddb_cocoaui_api_t;

NS_ASSUME_NONNULL_END

#endif /* cocoaui_api_h */
