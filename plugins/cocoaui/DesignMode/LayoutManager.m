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

#import "LayoutManager.h"

NSErrorDomain const LayoutManagerErrorDomain = @"LayoutManagerErrorDomain";

static NSString *const kLayoutMagic = @"ddblayout";
static const NSInteger kLayoutVersionMajor = 1;
static const NSInteger kLayoutVersionMinor = 0;
static NSString *const kLayoutExtension = @"ddblayout";
static NSString *const kLayoutsFolderName = @"UILayouts";

static LayoutManager *_sharedInstance;

@interface LayoutManagerEntry ()

- (instancetype)initWithName:(NSString *)name url:(NSURL *)url;

@end

@implementation LayoutManagerEntry

- (instancetype)initWithName:(NSString *)name url:(NSURL *)url {
    self = [super init];
    if (!self) {
        return nil;
    }
    _name = [name copy];
    _url = [url copy];
    return self;
}

@end

@interface LayoutManager ()

@property (nonatomic) NSArray<LayoutManagerEntry *> *builtInLayouts;

@end

@implementation LayoutManager

+ (LayoutManager *)sharedInstance {
    if (_sharedInstance == nil) {
        _sharedInstance = [LayoutManager new];
    }
    return _sharedInstance;
}

+ (void)freeSharedInstance {
    _sharedInstance = nil;
}

+ (NSString *)fileExtension {
    return kLayoutExtension;
}

- (instancetype)initWithBuiltInLayoutsDirectoryURL:(NSURL *)builtInURL userLayoutsDirectoryURL:(NSURL *)userURL {
    self = [super init];
    if (!self) {
        return nil;
    }
    _builtInLayoutsDirectoryURL = [builtInURL copy];
    _userLayoutsDirectoryURL = [userURL copy];
    _builtInLayouts = @[];
    return self;
}

- (instancetype)init {
    return [self initWithBuiltInLayoutsDirectoryURL:[NSBundle.mainBundle.resourceURL URLByAppendingPathComponent:kLayoutsFolderName isDirectory:YES]
                            userLayoutsDirectoryURL:nil];
}

- (NSArray<LayoutManagerEntry *> *)scanDirectory:(NSURL *)directoryURL {
    if (directoryURL == nil) {
        return @[];
    }
    // A missing folder is normal, e.g. no layouts were saved yet.
    NSArray<NSURL *> *urls = [NSFileManager.defaultManager contentsOfDirectoryAtURL:directoryURL
                                                         includingPropertiesForKeys:nil
                                                                            options:NSDirectoryEnumerationSkipsHiddenFiles
                                                                              error:nil];
    NSMutableArray<LayoutManagerEntry *> *entries = [NSMutableArray new];
    for (NSURL *url in urls) {
        if ([url.pathExtension caseInsensitiveCompare:kLayoutExtension] != NSOrderedSame) {
            continue;
        }
        NSString *name = url.lastPathComponent.stringByDeletingPathExtension;
        [entries addObject:[[LayoutManagerEntry alloc] initWithName:name url:url]];
    }
    [entries sortUsingComparator:^NSComparisonResult(LayoutManagerEntry *a, LayoutManagerEntry *b) {
        return [a.name localizedStandardCompare:b.name];
    }];
    return entries;
}

- (void)reloadBuiltInLayouts {
    self.builtInLayouts = [self scanDirectory:self.builtInLayoutsDirectoryURL];
}

- (NSArray<LayoutManagerEntry *> *)userLayouts {
    return [self scanDirectory:self.userLayoutsDirectoryURL];
}

- (NSURL *)ensureUserLayoutsDirectory {
    NSURL *url = self.userLayoutsDirectoryURL;
    if (url == nil) {
        return nil;
    }
    NSError *error;
    if (![NSFileManager.defaultManager createDirectoryAtURL:url withIntermediateDirectories:YES attributes:nil error:&error]) {
        NSLog (@"Failed to create %@: %@", url.path, error);
        return nil;
    }
    return url;
}

static NSError *
makeError (LayoutManagerError code, NSString *description) {
    return [NSError errorWithDomain:LayoutManagerErrorDomain code:code userInfo:@{NSLocalizedDescriptionKey: description}];
}

/// Parses "major.minor" into components. Returns NO if malformed.
static BOOL
parseVersion (id value, NSInteger *major, NSInteger *minor) {
    if (![value isKindOfClass:NSString.class]) {
        return NO;
    }
    NSArray<NSString *> *parts = [(NSString *)value componentsSeparatedByString:@"."];
    if (parts.count != 2) {
        return NO;
    }
    NSScanner *majorScanner = [NSScanner scannerWithString:parts[0]];
    NSScanner *minorScanner = [NSScanner scannerWithString:parts[1]];
    if (![majorScanner scanInteger:major] || !majorScanner.isAtEnd || ![minorScanner scanInteger:minor] || !minorScanner.isAtEnd) {
        return NO;
    }
    return *major >= 0 && *minor >= 0;
}

- (NSDictionary *)layoutFromURL:(NSURL *)url error:(NSError **)error {
    NSData *data = [NSData dataWithContentsOfURL:url options:0 error:error];
    if (data == nil) {
        return nil;
    }

    id json = [NSJSONSerialization JSONObjectWithData:data options:0 error:nil];
    if (![json isKindOfClass:NSDictionary.class]) {
        if (error) {
            *error = makeError (LayoutManagerErrorInvalidFile, @"The file is not a valid layout file.");
        }
        return nil;
    }

    NSDictionary *root = json;
    if (![root[@"magic"] isEqual:kLayoutMagic]) {
        if (error) {
            *error = makeError (LayoutManagerErrorWrongMagic, @"The file is not a DeaDBeeF layout file.");
        }
        return nil;
    }

    NSInteger major, minor;
    if (!parseVersion (root[@"version"], &major, &minor)) {
        if (error) {
            *error = makeError (LayoutManagerErrorInvalidFile, @"The layout file has an invalid version.");
        }
        return nil;
    }
    if (major > kLayoutVersionMajor || (major == kLayoutVersionMajor && minor > kLayoutVersionMinor)) {
        if (error) {
            *error = makeError (LayoutManagerErrorUnsupportedVersion,
                                [NSString stringWithFormat:@"The layout file version %@ is newer than the supported version %ld.%ld. Please update DeaDBeeF.",
                                 root[@"version"], (long)kLayoutVersionMajor, (long)kLayoutVersionMinor]);
        }
        return nil;
    }

    NSDictionary *layout = root[@"layout_cocoaui"];
    if (![layout isKindOfClass:NSDictionary.class]) {
        if (error) {
            *error = makeError (LayoutManagerErrorInvalidFile, @"The layout file doesn't contain a layout.");
        }
        return nil;
    }
    return layout;
}

- (BOOL)writeLayout:(NSDictionary *)layout toURL:(NSURL *)url error:(NSError **)error {
    NSDictionary *root = @{
        @"magic": kLayoutMagic,
        @"version": [NSString stringWithFormat:@"%ld.%ld", (long)kLayoutVersionMajor, (long)kLayoutVersionMinor],
        @"layout_cocoaui": layout,
    };
    NSData *data = [NSJSONSerialization dataWithJSONObject:root options:NSJSONWritingPrettyPrinted|NSJSONWritingSortedKeys error:error];
    if (data == nil) {
        return NO;
    }
    return [data writeToURL:url options:NSDataWritingAtomic error:error];
}

@end
