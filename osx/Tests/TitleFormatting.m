#import <Cocoa/Cocoa.h>
#import <XCTest/XCTest.h>
#include "playlist.h"
#include "tf.h"

@interface TitleFormatting : XCTestCase {
    playItem_t *it;
    ddb_tf_context_t ctx;
}
@end

@implementation TitleFormatting

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
    pl_item_unref (it);
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
    pl_add_meta (it, "disctotal", "20");
    pl_add_meta (it, "disc", "18");

    char *bc;
    int sz = tf_compile("$if($greater(%totaldiscs%,1),- Disc: %discnumber%/%totaldiscs%)", &bc);
    char buffer[200];
    tf_eval (&ctx, bc, sz, buffer, sizeof (buffer));
    tf_free (bc);
    XCTAssert([@"- Disc: 18/20" isEqualToString:[NSString stringWithUTF8String:buffer]], @"The actual output is: %s", buffer);
}

- (void)test_AlbumArtistSameAsArtist_ReturnsBlankTrackArtist {
    pl_add_meta (it, "artist", "Artist Name");
    pl_add_meta (it, "album artist", "Artist Name");

    char *bc;
    int sz = tf_compile("%track artist%", &bc);
    char buffer[200];
    tf_eval (&ctx, bc, sz, buffer, sizeof (buffer));
    tf_free (bc);
    XCTAssert([@"" isEqualToString:[NSString stringWithUTF8String:buffer]], @"The actual output is: %s", buffer);
}

- (void)test_TrackArtistIsUndef_ReturnsBlankTrackArtist {
    pl_add_meta (it, "album artist", "Artist Name");

    char *bc;
    int sz = tf_compile("%track artist%", &bc);
    char buffer[200];
    tf_eval (&ctx, bc, sz, buffer, sizeof (buffer));
    tf_free (bc);
    XCTAssert([@"" isEqualToString:[NSString stringWithUTF8String:buffer]], @"The actual output is: %s", buffer);
}

- (void)test_TrackArtistIsDefined_ReturnsTheTrackArtist {
    pl_add_meta (it, "artist", "Track Artist Name");
    pl_add_meta (it, "album artist", "Album Artist Name");

    char *bc;
    int sz = tf_compile("%track artist%", &bc);
    char buffer[200];
    tf_eval (&ctx, bc, sz, buffer, sizeof (buffer));
    tf_free (bc);
    XCTAssert([@"Track Artist Name" isEqualToString:[NSString stringWithUTF8String:buffer]], @"The actual output is: %s", buffer);
}

- (void)test_Add10And2_Gives12 {
    char *bc;
    int sz = tf_compile("$add(10,2)", &bc);
    char buffer[200];
    tf_eval (&ctx, bc, sz, buffer, sizeof (buffer));
    tf_free (bc);
    XCTAssert([@"12" isEqualToString:[NSString stringWithUTF8String:buffer]], @"The actual output is: %s", buffer);
}

- (void)test_StrcmpChannelsMono_GivesMo {
    char *bc;
    int sz = tf_compile("$if($strcmp(%channels%,mono),mo,st)", &bc);
    pl_replace_meta (it, ":CHANNELS", "1");
    char buffer[200];
    tf_eval (&ctx, bc, sz, buffer, sizeof (buffer));
    tf_free (bc);
    XCTAssert([@"mo" isEqualToString:[NSString stringWithUTF8String:buffer]], @"The actual output is: %s", buffer);
}

- (void)test_StrcmpChannelsMono_GivesSt {
    char *bc;
    int sz = tf_compile("$if($strcmp(%channels%,mono),mo,st)", &bc);
    pl_replace_meta (it, ":CHANNELS", "2");
    char buffer[200];
    tf_eval (&ctx, bc, sz, buffer, sizeof (buffer));
    tf_free (bc);
    XCTAssert([@"st" isEqualToString:[NSString stringWithUTF8String:buffer]], @"The actual output is: %s", buffer);
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

- (void)test_LongCommentOverflowBuffer_DoesntCrash {
    char longcomment[2048];
    for (int i = 0; i < sizeof (longcomment) - 1; i++) {
        longcomment[i] = (i % 33) + 'a';
    }
    longcomment[sizeof (longcomment)-1] = 0;

    pl_add_meta (it, "comment", longcomment);

    char *bc;
    int sz = tf_compile("%comment%", &bc);
    char *buffer = malloc (200);
    XCTAssertNoThrow(tf_eval (&ctx, bc, sz, buffer, 200), @"Crashed!");
    tf_free (bc);
    XCTAssert(!memcmp (buffer, "abcdef", 6), @"The actual output is: %s", buffer);
    free (buffer);
}

@end
