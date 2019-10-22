//
//  PlaylistTest.m
//  Tests
//
//  Created by Oleksiy Yakovenko on 24/10/2018.
//  Copyright © 2018 Alexey Yakovenko. All rights reserved.
//

#import <XCTest/XCTest.h>
#include "deadbeef.h"
#include "../../common.h"
#include "playlist.h"
#include "plugins.h"

@interface PlaylistTest : XCTestCase

@end

@implementation PlaylistTest

- (void)setUp {
    // Put setup code here. This method is called before the invocation of each test method in the class.
}

- (void)tearDown {
    // Put teardown code here. This method is called after the invocation of each test method in the class.
}

- (void)test_SearchForValueInSingleValueItems_FindsTheItem {
    playlist_t *plt = plt_alloc("test");
    playItem_t *it = pl_item_alloc();

    plt_insert_item(plt, NULL, it);

    pl_add_meta(it, "title", "value");

    plt_search_process(plt, "value");

    XCTAssertTrue(plt->head[PL_SEARCH] != NULL);
    XCTAssertTrue(plt->head[PL_MAIN]->selected);

    plt_unref (plt);
}


- (void)test_SearchFor2ndValueInMultiValueItems_FindsTheItem {
    playlist_t *plt = plt_alloc("test");
    playItem_t *it = pl_item_alloc();

    plt_insert_item(plt, NULL, it);

    const char values[] = "value1\0value2\0";
    pl_add_meta_full(it, "title", values, sizeof(values));

    plt_search_process(plt, "value2");

    XCTAssertTrue(plt->head[PL_SEARCH] != NULL);
    XCTAssertTrue(plt->head[PL_MAIN]->selected);

    plt_unref (plt);
}

- (void)test_IsRelativePath_AbsolutePath_False {
    int res = plug_is_relative_path ("/path");
    XCTAssertFalse(res);
}

- (void)test_IsRelativePath_AbsolutePathWithUriScheme_False {
    int res = plug_is_relative_path ("file:///path");
    XCTAssertFalse(res);
}

- (void)test_IsRelativePath_RelativePathWithUriScheme_True {
    int res = plug_is_relative_path ("file://path");
    XCTAssertTrue(res);
}

- (void)test_IsRelativePath_VFSPath_False {
    int res = plug_is_relative_path ("zip://path");
    XCTAssertFalse(res);
}

- (void)test_IsRelativePath_HTTPPath_False {
    int res = plug_is_relative_path ("http://path");
    XCTAssertFalse(res);
}

- (void)test_IsRelativePath_RelativePath_True {
    int res = plug_is_relative_path ("path");
    XCTAssertTrue(res);
}

- (void)test_IsRelativePath_RelativeWithFoldersPath_True {
    int res = plug_is_relative_path ("path/filename");
    XCTAssertTrue(res);
}

- (void)test_IsRelativePath_WeirdPath_True {
    int res = plug_is_relative_path ("something:something");
    XCTAssertTrue(res);
}

@end
