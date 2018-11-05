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

@end
