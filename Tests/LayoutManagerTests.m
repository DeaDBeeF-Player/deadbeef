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

#import <XCTest/XCTest.h>
#import "LayoutManager.h"

@interface LayoutManagerTests : XCTestCase

@property (nonatomic) NSURL *tempURL;
@property (nonatomic) NSURL *builtInURL;
@property (nonatomic) NSURL *userURL;
@property (nonatomic) LayoutManager *manager;

@end

@implementation LayoutManagerTests

- (void)setUp {
    self.tempURL = [NSURL fileURLWithPath:[NSTemporaryDirectory() stringByAppendingPathComponent:NSUUID.UUID.UUIDString] isDirectory:YES];
    self.builtInURL = [self.tempURL URLByAppendingPathComponent:@"BuiltIn" isDirectory:YES];
    self.userURL = [self.tempURL URLByAppendingPathComponent:@"User" isDirectory:YES];
    [NSFileManager.defaultManager createDirectoryAtURL:self.builtInURL withIntermediateDirectories:YES attributes:nil error:nil];
    self.manager = [[LayoutManager alloc] initWithBuiltInLayoutsDirectoryURL:self.builtInURL userLayoutsDirectoryURL:self.userURL];
}

- (void)tearDown {
    [NSFileManager.defaultManager removeItemAtURL:self.tempURL error:nil];
}

- (NSURL *)writeFileNamed:(NSString *)name contents:(NSString *)contents inDirectory:(NSURL *)directory {
    [NSFileManager.defaultManager createDirectoryAtURL:directory withIntermediateDirectories:YES attributes:nil error:nil];
    NSURL *url = [directory URLByAppendingPathComponent:name];
    [contents writeToURL:url atomically:YES encoding:NSUTF8StringEncoding error:nil];
    return url;
}

- (NSURL *)writeEnvelopeWithMagic:(NSString *)magic version:(NSString *)version layout:(NSString *)layout {
    NSString *json = [NSString stringWithFormat:@"{\"magic\":\"%@\",\"version\":\"%@\",\"layout_cocoaui\":%@}", magic, version, layout];
    return [self writeFileNamed:@"test.ddblayout" contents:json inDirectory:self.tempURL];
}

#pragma mark - Reading and writing

- (void)test_writeThenRead_roundTripsLayout {
    NSDictionary *layout = @{@"type": @"Splitter", @"children": @[@{@"type": @"PlaylistWithTabs"}]};
    NSURL *url = [self.tempURL URLByAppendingPathComponent:@"roundtrip.ddblayout"];

    NSError *error;
    XCTAssertTrue([self.manager writeLayout:layout toURL:url error:&error], @"%@", error);
    NSDictionary *loaded = [self.manager layoutFromURL:url error:&error];

    XCTAssertEqualObjects(loaded, layout, @"%@", error);
}

- (void)test_write_savesMagicVersionAndLayout {
    NSDictionary *layout = @{@"type": @"PlaylistWithTabs"};
    NSURL *url = [self.tempURL URLByAppendingPathComponent:@"envelope.ddblayout"];
    [self.manager writeLayout:layout toURL:url error:nil];

    NSDictionary *json = [NSJSONSerialization JSONObjectWithData:[NSData dataWithContentsOfURL:url] options:0 error:nil];

    XCTAssertEqualObjects(json[@"magic"], @"ddblayout");
    XCTAssertEqualObjects(json[@"version"], @"1.0");
    XCTAssertEqualObjects(json[@"layout_cocoaui"], layout);
}

- (void)test_read_currentVersion_succeeds {
    NSURL *url = [self writeEnvelopeWithMagic:@"ddblayout" version:@"1.0" layout:@"{\"type\":\"PlaylistWithTabs\"}"];

    NSDictionary *layout = [self.manager layoutFromURL:url error:nil];

    XCTAssertEqualObjects(layout[@"type"], @"PlaylistWithTabs");
}

- (void)test_read_wrongMagic_fails {
    NSURL *url = [self writeEnvelopeWithMagic:@"other" version:@"1.0" layout:@"{}"];

    NSError *error;
    XCTAssertNil([self.manager layoutFromURL:url error:&error]);
    XCTAssertEqual(error.code, LayoutManagerErrorWrongMagic);
}

- (void)test_read_higherMinorVersion_fails {
    NSURL *url = [self writeEnvelopeWithMagic:@"ddblayout" version:@"1.1" layout:@"{}"];

    NSError *error;
    XCTAssertNil([self.manager layoutFromURL:url error:&error]);
    XCTAssertEqual(error.code, LayoutManagerErrorUnsupportedVersion);
}

- (void)test_read_higherMajorVersion_fails {
    NSURL *url = [self writeEnvelopeWithMagic:@"ddblayout" version:@"2.0" layout:@"{}"];

    NSError *error;
    XCTAssertNil([self.manager layoutFromURL:url error:&error]);
    XCTAssertEqual(error.code, LayoutManagerErrorUnsupportedVersion);
}

- (void)test_read_versionTenIsHigherThanTwo {
    // Versions are numeric, not lexicographic.
    NSURL *url = [self writeEnvelopeWithMagic:@"ddblayout" version:@"1.10" layout:@"{}"];

    NSError *error;
    XCTAssertNil([self.manager layoutFromURL:url error:&error]);
    XCTAssertEqual(error.code, LayoutManagerErrorUnsupportedVersion);
}

- (void)test_read_malformedVersion_fails {
    for (NSString *version in @[@"", @"1", @"1.0.0", @"a.b", @"-1.0", @"1.x"]) {
        NSURL *url = [self writeEnvelopeWithMagic:@"ddblayout" version:version layout:@"{}"];

        NSError *error;
        XCTAssertNil([self.manager layoutFromURL:url error:&error], @"version %@", version);
        XCTAssertEqual(error.code, LayoutManagerErrorInvalidFile, @"version %@", version);
    }
}

- (void)test_read_missingLayout_fails {
    NSURL *url = [self writeFileNamed:@"test.ddblayout" contents:@"{\"magic\":\"ddblayout\",\"version\":\"1.0\"}" inDirectory:self.tempURL];

    NSError *error;
    XCTAssertNil([self.manager layoutFromURL:url error:&error]);
    XCTAssertEqual(error.code, LayoutManagerErrorInvalidFile);
}

- (void)test_read_layoutNotDictionary_fails {
    NSURL *url = [self writeEnvelopeWithMagic:@"ddblayout" version:@"1.0" layout:@"[1,2]"];

    NSError *error;
    XCTAssertNil([self.manager layoutFromURL:url error:&error]);
    XCTAssertEqual(error.code, LayoutManagerErrorInvalidFile);
}

- (void)test_read_invalidJSON_fails {
    NSURL *url = [self writeFileNamed:@"test.ddblayout" contents:@"not json" inDirectory:self.tempURL];

    NSError *error;
    XCTAssertNil([self.manager layoutFromURL:url error:&error]);
    XCTAssertEqual(error.code, LayoutManagerErrorInvalidFile);
}

- (void)test_read_missingFile_fails {
    NSError *error;
    XCTAssertNil([self.manager layoutFromURL:[self.tempURL URLByAppendingPathComponent:@"nonexistent.ddblayout"] error:&error]);
    XCTAssertNotNil(error);
}

#pragma mark - Folders

- (void)test_builtInLayouts_beforeReload_isEmpty {
    [self writeFileNamed:@"A.ddblayout" contents:@"{}" inDirectory:self.builtInURL];

    XCTAssertEqual(self.manager.builtInLayouts.count, 0);
}

- (void)test_reloadBuiltInLayouts_listsOnlyLayoutFilesSortedByName {
    [self writeFileNamed:@"Beta.ddblayout" contents:@"{}" inDirectory:self.builtInURL];
    [self writeFileNamed:@"alpha.ddblayout" contents:@"{}" inDirectory:self.builtInURL];
    [self writeFileNamed:@"notes.txt" contents:@"" inDirectory:self.builtInURL];

    [self.manager reloadBuiltInLayouts];

    NSArray<NSString *> *names = [self.manager.builtInLayouts valueForKey:@"name"];
    XCTAssertEqualObjects(names, (@[@"alpha", @"Beta"]));
}

- (void)test_reloadBuiltInLayouts_missingFolder_isEmpty {
    LayoutManager *manager = [[LayoutManager alloc] initWithBuiltInLayoutsDirectoryURL:[self.tempURL URLByAppendingPathComponent:@"Missing"]
                                                               userLayoutsDirectoryURL:nil];

    [manager reloadBuiltInLayouts];

    XCTAssertEqual(manager.builtInLayouts.count, 0);
}

- (void)test_userLayouts_missingFolder_isEmpty {
    XCTAssertEqual(self.manager.userLayouts.count, 0);
}

- (void)test_userLayouts_areRescannedOnEveryAccess {
    XCTAssertEqual(self.manager.userLayouts.count, 0);

    [self writeFileNamed:@"Mine.ddblayout" contents:@"{}" inDirectory:self.userURL];

    XCTAssertEqualObjects([self.manager.userLayouts valueForKey:@"name"], @[@"Mine"]);
}

- (void)test_ensureUserLayoutsDirectory_createsFolder {
    BOOL isDirectory = NO;
    XCTAssertFalse([NSFileManager.defaultManager fileExistsAtPath:self.userURL.path]);

    NSURL *url = [self.manager ensureUserLayoutsDirectory];

    XCTAssertEqualObjects(url, self.userURL);
    XCTAssertTrue([NSFileManager.defaultManager fileExistsAtPath:self.userURL.path isDirectory:&isDirectory]);
    XCTAssertTrue(isDirectory);
}

- (void)test_ensureUserLayoutsDirectory_withoutURL_returnsNil {
    LayoutManager *manager = [[LayoutManager alloc] initWithBuiltInLayoutsDirectoryURL:nil userLayoutsDirectoryURL:nil];

    XCTAssertNil([manager ensureUserLayoutsDirectory]);
}

@end
