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

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

extern NSErrorDomain const LayoutManagerErrorDomain;

typedef NS_ERROR_ENUM (LayoutManagerErrorDomain, LayoutManagerError) {
    LayoutManagerErrorInvalidFile = 1,
    LayoutManagerErrorWrongMagic = 2,
    LayoutManagerErrorUnsupportedVersion = 3,
    LayoutManagerErrorWriteFailed = 4,
};

/// A named layout file, found in one of the layout folders.
@interface LayoutManagerEntry : NSObject

/// File name without the extension.
@property (nonatomic,readonly) NSString *name;
@property (nonatomic,readonly) NSURL *url;

@end

@interface LayoutManager : NSObject

@property (nonatomic,class,readonly) LayoutManager *sharedInstance;
+ (void)freeSharedInstance;

@property (nonatomic,class,readonly) NSString *fileExtension;

/// Layouts shipped inside the app bundle (Resources/UILayouts). Loaded on startup, see -reloadBuiltInLayouts.
@property (nonatomic,readonly) NSArray<LayoutManagerEntry *> *builtInLayouts;

/// Layouts saved by the user. Rescanned on every access.
@property (nonatomic,readonly) NSArray<LayoutManagerEntry *> *userLayouts;

/// Folder for saved user layouts; may not exist yet.
@property (nonatomic,nullable,copy) NSURL *userLayoutsDirectoryURL;

/// Folder with the built-in layouts.
@property (nonatomic,nullable,copy) NSURL *builtInLayoutsDirectoryURL;

- (instancetype)initWithBuiltInLayoutsDirectoryURL:(nullable NSURL *)builtInURL
                           userLayoutsDirectoryURL:(nullable NSURL *)userURL NS_DESIGNATED_INITIALIZER;
- (instancetype)init;

- (void)reloadBuiltInLayouts;

/// Creates the user layouts folder if needed. Returns nil on failure.
- (nullable NSURL *)ensureUserLayoutsDirectory;

/// Reads a .ddblayout file, verifies magic and version, returns the "layout" dictionary.
- (nullable NSDictionary *)layoutFromURL:(NSURL *)url error:(NSError **)error;

/// Writes the layout wrapped into the .ddblayout envelope.
- (BOOL)writeLayout:(NSDictionary *)layout toURL:(NSURL *)url error:(NSError **)error;

@end

NS_ASSUME_NONNULL_END
