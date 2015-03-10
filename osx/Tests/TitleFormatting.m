#import <Cocoa/Cocoa.h>
#import <XCTest/XCTest.h>
#include "playlist.h"
#include "tf.h"

@interface TitleFormatting : XCTestCase

@end

@implementation TitleFormatting {
    playItem_t *it;
    ddb_tf_context_t ctx;
}

- (void)setUp {
    [super setUp];

    pl_init ();

    it = pl_item_alloc_init ("testfile.flac", "stdflac");

    memset (&ctx, 0, sizeof (ctx));
    ctx._size = sizeof (ddb_tf_context_t);
    ctx.it = (DB_playItem_t *)it;
    ctx.plt = NULL;
    ctx.idx = -1;
    ctx.id = -1;
}

- (void)tearDown {
    pl_free ();

    [super tearDown];
}

- (void)test_Literal_ReturnsLiteral {
    char *bc;
    int sz = tf_compile("hello world", &bc);
    char buffer[20];
    tf_eval (&ctx, bc, sz, buffer, sizeof (buffer));
    tf_free (bc);
    XCTAssert([@"hello world" isEqualToString:[NSString stringWithUTF8String:buffer]]);
}

- (void)test_AlbumArtistDash4DigitYearSpaceAlbum_ReturnsCorrectResult {
    pl_add_meta (it, "album artist", "TheAlbumArtist");
    pl_add_meta (it, "year", "12345678");
    pl_add_meta (it, "album", "TheNameOfAlbum");

    char *bc;
    int sz = tf_compile("%album artist% - ($left(%year%,4)) %album%", &bc);
    char buffer[200];
    tf_eval (&ctx, bc, sz, buffer, sizeof (buffer));
    tf_free (bc);
    XCTAssert([@"TheAlbumArtist - (1234) TheNameOfAlbum" isEqualToString:[NSString stringWithUTF8String:buffer]], @"The actual output is: %s", buffer);
}

- (void)test_Unicode_AlbumArtistDash4DigitYearSpaceAlbum_ReturnsCorrectResult {
    pl_add_meta (it, "album artist", "ИсполнительДанногоАльбома");
    pl_add_meta (it, "year", "12345678");
    pl_add_meta (it, "album", "Альбом");

    char *bc;
    int sz = tf_compile("%album artist% - ($left(%year%,4)) %album%", &bc);
    char buffer[200];
    tf_eval (&ctx, bc, sz, buffer, sizeof (buffer));
    tf_free (bc);
    XCTAssert([@"ИсполнительДанногоАльбома - (1234) Альбом" isEqualToString:[NSString stringWithUTF8String:buffer]], @"The actual output is: %s", buffer);
}

- (void)test_TotalDiscsGreaterThan1_ReturnsExpectedResult {
    pl_add_meta (it, "totaldiscs", "20");
    pl_add_meta (it, "disc", "18");

    char *bc;
    int sz = tf_compile("$if($greater(%totaldiscs%,1),- Disc: %discnumber%/%totaldiscs%)", &bc);
    char buffer[200];
    tf_eval (&ctx, bc, sz, buffer, sizeof (buffer));
    tf_free (bc);
    XCTAssert([@"- Disc: 18/20" isEqualToString:[NSString stringWithUTF8String:buffer]], @"The actual output is: %s", buffer);
}

- (void)test_SimpleExpr_Performance {
    char *bc;
    int sz = tf_compile("simple expr", &bc);

    [self measureBlock:^{
        char buffer[20];
        tf_eval (&ctx, bc, sz, buffer, sizeof (buffer));
    }];
    
    tf_free (bc);
}

@end
