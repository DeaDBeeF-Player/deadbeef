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
    char *bc = tf_compile("hello world");
    char buffer[20];
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    XCTAssert([@"hello world" isEqualToString:[NSString stringWithUTF8String:buffer]]);
}

- (void)test_AlbumArtistDash4DigitYearSpaceAlbum_ReturnsCorrectResult {
    pl_add_meta (it, "album artist", "TheAlbumArtist");
    pl_add_meta (it, "year", "12345678");
    pl_add_meta (it, "album", "TheNameOfAlbum");

    char *bc = tf_compile("%album artist% - ($left(%year%,4)) %album%");
    char buffer[200];
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    XCTAssert([@"TheAlbumArtist - (1234) TheNameOfAlbum" isEqualToString:[NSString stringWithUTF8String:buffer]], @"The actual output is: %s", buffer);
}

- (void)test_Unicode_AlbumArtistDash4DigitYearSpaceAlbum_ReturnsCorrectResult {
    pl_add_meta (it, "album artist", "ИсполнительДанногоАльбома");
    pl_add_meta (it, "year", "12345678");
    pl_add_meta (it, "album", "Альбом");

    char *bc = tf_compile("%album artist% - ($left(%year%,4)) %album%");
    char buffer[200];
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    XCTAssert([@"ИсполнительДанногоАльбома - (1234) Альбом" isEqualToString:[NSString stringWithUTF8String:buffer]], @"The actual output is: %s", buffer);
}

- (void)test_TotalDiscsGreaterThan1_ReturnsExpectedResult {
    pl_add_meta (it, "disctotal", "20");
    pl_add_meta (it, "disc", "18");

    char *bc = tf_compile("$if($greater(%totaldiscs%,1),- Disc: %discnumber%/%totaldiscs%)");
    char buffer[200];
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    XCTAssert([@"- Disc: 18/20" isEqualToString:[NSString stringWithUTF8String:buffer]], @"The actual output is: %s", buffer);
}

- (void)test_AlbumArtistSameAsArtist_ReturnsBlankTrackArtist {
    pl_add_meta (it, "artist", "Artist Name");
    pl_add_meta (it, "album artist", "Artist Name");

    char *bc = tf_compile("%track artist%");
    char buffer[200];
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    XCTAssert([@"" isEqualToString:[NSString stringWithUTF8String:buffer]], @"The actual output is: %s", buffer);
}

- (void)test_TrackArtistIsUndef_ReturnsBlankTrackArtist {
    pl_add_meta (it, "album artist", "Artist Name");

    char *bc = tf_compile("%track artist%");
    char buffer[200];
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    XCTAssert([@"" isEqualToString:[NSString stringWithUTF8String:buffer]], @"The actual output is: %s", buffer);
}

- (void)test_TrackArtistIsDefined_ReturnsTheTrackArtist {
    pl_add_meta (it, "artist", "Track Artist Name");
    pl_add_meta (it, "album artist", "Album Artist Name");

    char *bc = tf_compile("%track artist%");
    char buffer[200];
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    XCTAssert([@"Track Artist Name" isEqualToString:[NSString stringWithUTF8String:buffer]], @"The actual output is: %s", buffer);
}

- (void)test_Add10And2_Gives12 {
    char *bc = tf_compile("$add(10,2)");
    char buffer[200];
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    XCTAssert([@"12" isEqualToString:[NSString stringWithUTF8String:buffer]], @"The actual output is: %s", buffer);
}

- (void)test_StrcmpChannelsMono_GivesMo {
    char *bc = tf_compile("$if($strcmp(%channels%,mono),mo,st)");
    pl_replace_meta (it, ":CHANNELS", "1");
    char buffer[200];
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    XCTAssert([@"mo" isEqualToString:[NSString stringWithUTF8String:buffer]], @"The actual output is: %s", buffer);
}

- (void)test_StrcmpChannelsMono_GivesSt {
    char *bc = tf_compile("$if($strcmp(%channels%,mono),mo,st)");
    pl_replace_meta (it, ":CHANNELS", "2");
    char buffer[200];
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    XCTAssert([@"st" isEqualToString:[NSString stringWithUTF8String:buffer]], @"The actual output is: %s", buffer);
}

- (void)test_SimpleExpr_Performance {
    char *bc = tf_compile("simple expr");

    [self measureBlock:^{
        char buffer[20];
        tf_eval (&ctx, bc, buffer, sizeof (buffer));
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

    char *bc = tf_compile("%comment%");
    char *buffer = malloc (200);
    XCTAssertNoThrow(tf_eval (&ctx, bc, buffer, 200), @"Crashed!");
    tf_free (bc);
    XCTAssert(!memcmp (buffer, "abcdef", 6), @"The actual output is: %s", buffer);
    free (buffer);
}

- (void)test_ParticularLongExpressionDoesntAllocateZeroBytes {
    pl_replace_meta (it, "artist", "Frank Schätzing");
    pl_replace_meta (it, "year", "1999");
    pl_replace_meta (it, "album", "Tod und Teufel");
    pl_replace_meta (it, "disc", "1");
    pl_replace_meta (it, "disctotal", "4");
    pl_replace_meta (it, ":FILETYPE", "FLAC");
    char *bc = tf_compile("$if($strcmp(%genre%,Classical),%composer%,$if([%band%],%band%,%album artist%)) | ($left(%year%,4)) %album% $if($greater(%totaldiscs%,1),- Disc: %discnumber%/%totaldiscs%) - \\[%codec%\\]");
    char *buffer = malloc (1000);
    XCTAssertNoThrow(tf_eval (&ctx, bc, buffer, 1000), @"Crashed!");
    tf_free (bc);
    free (buffer);
}

- (void)test_If2FirstArgIsTrue_EvalToFirstArg {
    char *bc = tf_compile("$if2(abc,def)");
    char buffer[200];
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    XCTAssert([@"abc" isEqualToString:[NSString stringWithUTF8String:buffer]], @"The actual output is: %s", buffer);
}

- (void)test_If2FirstArgIsFalse_EvalToSecondArg {
    char *bc = tf_compile("$if2(,ghi)");
    char buffer[200];
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    XCTAssert([@"ghi" isEqualToString:[NSString stringWithUTF8String:buffer]], @"The actual output is: %s", buffer);
}

- (void)test_If3FirstArgIsTrue_EvalToFirstArg {
    char *bc = tf_compile("$if3(abc,def)");
    char buffer[200];
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    XCTAssert([@"abc" isEqualToString:[NSString stringWithUTF8String:buffer]], @"The actual output is: %s", buffer);
}

- (void)test_If3AllButLastAreFalse_EvalToLastArg {
    char *bc = tf_compile("$if3(,,,,,lastarg)");
    char buffer[200];
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    XCTAssert([@"lastarg" isEqualToString:[NSString stringWithUTF8String:buffer]], @"The actual output is: %s", buffer);
}

- (void)test_If3OneOfTheArgsBeforeLastIsTrue_EvalToFirstTrueArg {
    char *bc = tf_compile("$if3(,,firstarg,,secondarg,lastarg)");
    char buffer[200];
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    XCTAssert([@"firstarg" isEqualToString:[NSString stringWithUTF8String:buffer]], @"The actual output is: %s", buffer);
}

- (void)test_IfEqualTrue_EvalsToThen {
    char *bc = tf_compile("$ifequal(100,100,then,else)");
    char buffer[200];
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    XCTAssert([@"then" isEqualToString:[NSString stringWithUTF8String:buffer]], @"The actual output is: %s", buffer);
}

- (void)test_IfEqualFalse_EvalsToElse {
    char *bc = tf_compile("$ifequal(100,200,then,else)");
    char buffer[200];
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    XCTAssert([@"else" isEqualToString:[NSString stringWithUTF8String:buffer]], @"The actual output is: %s", buffer);
}

- (void)test_IfGreaterTrue_EvalsToThen {
    char *bc = tf_compile("$ifgreater(200,100,then,else)");
    char buffer[200];
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    XCTAssert([@"then" isEqualToString:[NSString stringWithUTF8String:buffer]], @"The actual output is: %s", buffer);
}

- (void)test_IfGreaterFalse_EvalsToElse {
    char *bc = tf_compile("$ifgreater(100,200,then,else)");
    char buffer[200];
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    XCTAssert([@"else" isEqualToString:[NSString stringWithUTF8String:buffer]], @"The actual output is: %s", buffer);
}

- (void)test_GreaterIsTrue_EvalsToTrue {
    char *bc = tf_compile("$if($greater(2,1),istrue,isfalse)");
    char buffer[200];
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    XCTAssert([@"istrue" isEqualToString:[NSString stringWithUTF8String:buffer]], @"The actual output is: %s", buffer);
}

- (void)test_GreaterIsFalse_EvalsToFalse {
    char *bc = tf_compile("$if($greater(1,2),istrue,isfalse)");
    char buffer[200];
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    XCTAssert([@"isfalse" isEqualToString:[NSString stringWithUTF8String:buffer]], @"The actual output is: %s", buffer);
}

- (void)test_GreaterIsFalseEmptyArguments_EvalsToFalse {
    char *bc = tf_compile("$if($greater(,),istrue,isfalse)");
    char buffer[200];
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    XCTAssert([@"isfalse" isEqualToString:[NSString stringWithUTF8String:buffer]], @"The actual output is: %s", buffer);
}

- (void)test_StrCmpEmptyArguments_EvalsToTrue {
    char *bc = tf_compile("$if($strcmp(,),istrue,isfalse)");
    char buffer[200];
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    XCTAssert([@"istrue" isEqualToString:[NSString stringWithUTF8String:buffer]], @"The actual output is: %s", buffer);
}

- (void)test_StrCmpSameArguments_EvalsToTrue {
    char *bc = tf_compile("$if($strcmp(abc,abc),istrue,isfalse)");
    char buffer[200];
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    XCTAssert([@"istrue" isEqualToString:[NSString stringWithUTF8String:buffer]], @"The actual output is: %s", buffer);
}

- (void)test_IfLongerTrue_EvalsToTrue {
    char *bc = tf_compile("$iflonger(abcd,ef,istrue,isfalse)");
    char buffer[200];
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    XCTAssert([@"istrue" isEqualToString:[NSString stringWithUTF8String:buffer]], @"The actual output is: %s", buffer);
}

- (void)test_IfLongerFalse_EvalsToFalse {
    char *bc = tf_compile("$iflonger(ab,cdef,istrue,isfalse)");
    char buffer[200];
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    XCTAssert([@"isfalse" isEqualToString:[NSString stringWithUTF8String:buffer]], @"The actual output is: %s", buffer);
}

- (void)test_SelectMiddle_EvalsToSelectedValue {
    char *bc = tf_compile("$select(3,10,20,30,40,50)");
    char buffer[200];
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    XCTAssert([@"30" isEqualToString:[NSString stringWithUTF8String:buffer]], @"The actual output is: %s", buffer);
}

- (void)test_SelectLeftmost_EvalsToSelectedValue {
    char *bc = tf_compile("$select(1,10,20,30,40,50)");
    char buffer[200];
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    XCTAssert([@"10" isEqualToString:[NSString stringWithUTF8String:buffer]], @"The actual output is: %s", buffer);
}

- (void)test_SelectRightmost_EvalsToSelectedValue {
    char *bc = tf_compile("$select(5,10,20,30,40,50)");
    char buffer[200];
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    XCTAssert([@"50" isEqualToString:[NSString stringWithUTF8String:buffer]], @"The actual output is: %s", buffer);
}

- (void)test_SelectOutOfBoundsLeft_EvalsToFalse {
    char *bc = tf_compile("$select(0,10,20,30,40,50)");
    char buffer[200];
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    XCTAssert([@"" isEqualToString:[NSString stringWithUTF8String:buffer]], @"The actual output is: %s", buffer);
}

- (void)test_SelectOutOfBoundsRight_EvalsToFalse {
    char *bc = tf_compile("$select(6,10,20,30,40,50)");
    char buffer[200];
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    XCTAssert([@"" isEqualToString:[NSString stringWithUTF8String:buffer]], @"The actual output is: %s", buffer);
}
@end
