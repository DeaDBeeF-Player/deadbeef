/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2018 Alexey Yakovenko and other contributors

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

#import <Cocoa/Cocoa.h>
#import <XCTest/XCTest.h>
#include "playlist.h"
#include "tf.h"
#include "playqueue.h"
#include "streamer.h"
#include "plugins.h"

static int fake_out_state_value = OUTPUT_STATE_STOPPED;

static int fake_out_state (void) {
    return fake_out_state_value;
}

static DB_output_t fake_out = {
    .plugin.id = "fake_out",
    .plugin.name = "fake_out",
    .state = fake_out_state,
};

@interface TitleFormatting : XCTestCase {
    playItem_t *it;
    ddb_tf_context_t ctx;
    char buffer[1000];
}
@end

@implementation TitleFormatting

- (void)setUp {
    [super setUp];

    it = pl_item_alloc_init ("testfile.flac", "stdflac");

    memset (&ctx, 0, sizeof (ctx));
    ctx._size = sizeof (ddb_tf_context_t);
    ctx.it = (DB_playItem_t *)it;
    ctx.plt = NULL;

    streamer_set_playing_track (NULL);

    fake_out_state_value = OUTPUT_STATE_STOPPED;
}

- (void)tearDown {
    streamer_set_playing_track (NULL);
    pl_item_unref (it);
    ctx.plt = NULL;

    [super tearDown];
}

- (void)test_Literal_ReturnsLiteral {
    char *bc = tf_compile("hello world");
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    XCTAssert(!strcmp ("hello world", buffer), @"The actual output is: %s", buffer);
}

- (void)test_AlbumArtistDash4DigitYearSpaceAlbum_ReturnsCorrectResult {
    pl_add_meta (it, "album artist", "TheAlbumArtist");
    pl_add_meta (it, "year", "12345678");
    pl_add_meta (it, "album", "TheNameOfAlbum");

    char *bc = tf_compile("%album artist% - ($left($meta(year),4)) %album%");
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    XCTAssert(!strcmp ("TheAlbumArtist - (1234) TheNameOfAlbum", buffer), @"The actual output is: %s", buffer);
}

- (void)test_Unicode_AlbumArtistDash4DigitYearSpaceAlbum_ReturnsCorrectResult {
    pl_add_meta (it, "album artist", "ИсполнительДанногоАльбома");
    pl_add_meta (it, "year", "12345678");
    pl_add_meta (it, "album", "Альбом");

    char *bc = tf_compile("%album artist% - ($left($meta(year),4)) %album%");
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    XCTAssert(!strcmp ("ИсполнительДанногоАльбома - (1234) Альбом", buffer), @"The actual output is: %s", buffer);
}

- (void)test_TotalDiscsGreaterThan1_ReturnsExpectedResult {
    pl_add_meta (it, "numdiscs", "20");
    pl_add_meta (it, "disc", "18");

    char *bc = tf_compile("$if($greater(%totaldiscs%,1),- Disc: %discnumber%/%totaldiscs%)");
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    XCTAssert(!strcmp ("- Disc: 18/20", buffer), @"The actual output is: %s", buffer);
}

- (void)test_AlbumArtistSameAsArtist_ReturnsBlankTrackArtist {
    pl_add_meta (it, "artist", "Artist Name");
    pl_add_meta (it, "album artist", "Artist Name");

    char *bc = tf_compile("%track artist%");
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    XCTAssert(!buffer[0], @"The actual output is: %s", buffer);
}

- (void)test_TrackArtistIsUndef_ReturnsBlankTrackArtist {
    pl_add_meta (it, "album artist", "Artist Name");

    char *bc = tf_compile("%track artist%");
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    XCTAssert(!buffer[0], @"The actual output is: %s", buffer);
}

- (void)test_TrackArtistIsDefined_ReturnsTheTrackArtist {
    pl_add_meta (it, "artist", "Track Artist Name");
    pl_add_meta (it, "album artist", "Album Artist Name");

    char *bc = tf_compile("%track artist%");
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    XCTAssert(!strcmp ("Track Artist Name", buffer), @"The actual output is: %s", buffer);
}

- (void)test_Add10And2_Gives12 {
    char *bc = tf_compile("$add(10,2)");
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    XCTAssert(!strcmp ("12", buffer), @"The actual output is: %s", buffer);
}

- (void)test_StrcmpChannelsMono_GivesMo {
    char *bc = tf_compile("$if($strcmp(%channels%,mono),mo,st)");
    pl_replace_meta (it, ":CHANNELS", "1");
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    XCTAssert(!strcmp ("mo", buffer), @"The actual output is: %s", buffer);
}

- (void)test_StrcmpChannelsMono_GivesSt {
    char *bc = tf_compile("$if($strcmp(%channels%,mono),mo,st)");
    pl_replace_meta (it, ":CHANNELS", "2");
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    XCTAssert(!strcmp ("st", buffer), @"The actual output is: %s", buffer);
}

- (void)test_SimpleExpr_Performance {
    char *bc = tf_compile("simple expr");

    [self measureBlock:^{
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

    char *bc = tf_compile("$meta(comment)");
    XCTAssertNoThrow(tf_eval (&ctx, bc, buffer, 200), @"Crashed!");
    tf_free (bc);
    XCTAssert(!memcmp (buffer, "abcdef", 6), @"The actual output is: %s", buffer);
}

- (void)test_ParticularLongExpressionDoesntAllocateZeroBytes {
    pl_replace_meta (it, "artist", "Frank Schätzing");
    pl_replace_meta (it, "year", "1999");
    pl_replace_meta (it, "album", "Tod und Teufel");
    pl_replace_meta (it, "disc", "1");
    pl_replace_meta (it, "disctotal", "4");
    pl_replace_meta (it, ":FILETYPE", "FLAC");
    char *bc = tf_compile("$if($strcmp(%genre%,Classical),%composer%,$if([%band%],%band%,%album artist%)) | ($left(%year%,4)) %album% $if($greater(%totaldiscs%,1),- Disc: %discnumber%/%totaldiscs%) - \\[%codec%\\]");
    XCTAssertNoThrow(tf_eval (&ctx, bc, buffer, 1000), @"Crashed!");
    tf_free (bc);
}

- (void)test_If2FirstArgIsTrue_EvalToFirstArg {
    pl_replace_meta (it, "title", "a title");
    char *bc = tf_compile("$if2(%title%,def)");
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    XCTAssert(!strcmp ("a title", buffer), @"The actual output is: %s", buffer);
}

- (void)test_If2FirstArgIsMixedStringTrue_EvalToFirstArg {
    pl_replace_meta (it, "title", "a title");
    pl_replace_meta (it, "artist", "an artist");
    char *bc = tf_compile("$if2(%title%%artist%,def)");
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    XCTAssert(!strcmp ("a titlean artist", buffer), @"The actual output is: %s", buffer);
}

- (void)test_If2FirstArgIsMissingField_EvalToLastArg {
    pl_replace_meta (it, "title", "a title");
    pl_replace_meta (it, "artist", "an artist");
    char *bc = tf_compile("$if2(%garbage%,def)");
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    XCTAssert(!strcmp ("def", buffer), @"The actual output is: %s", buffer);
}

- (void)test_If2FirstArgIsMixedWithGarbageTrue_EvalToFirstArg {
    pl_replace_meta (it, "title", "a title");
    pl_replace_meta (it, "artist", "an artist");
    char *bc = tf_compile("$if2(%garbage%xxx%title%,def)");
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    XCTAssert(!strcmp ("xxxa title", buffer), @"The actual output is: %s", buffer);
}

- (void)test_If2FirstArgIsMixedWithGarbageTailTrue_EvalToFirstArg {
    pl_replace_meta (it, "title", "a title");
    pl_replace_meta (it, "artist", "an artist");
    char *bc = tf_compile("$if2(%title%%garbage%xxx,def)");
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    XCTAssert(!strcmp ("a titlexxx", buffer), @"The actual output is: %s", buffer);
}

- (void)test_If2FirstArgIsFalse_EvalToSecondArg {
    char *bc = tf_compile("$if2(,ghi)");
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    XCTAssert(!strcmp ("ghi", buffer), @"The actual output is: %s", buffer);
}

- (void)test_If3FirstArgIsTrue_EvalToFirstArg {
    pl_replace_meta (it, "title", "a title");
    char *bc = tf_compile("$if3(%title%,def)");
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    XCTAssert(!strcmp ("a title", buffer), @"The actual output is: %s", buffer);
}

- (void)test_If3AllButLastAreFalse_EvalToLastArg {
    char *bc = tf_compile("$if3(,,,,,lastarg)");
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    XCTAssert(!strcmp ("lastarg", buffer), @"The actual output is: %s", buffer);
}

- (void)test_If3OneOfTheArgsBeforeLastIsTrue_EvalToFirstTrueArg {
    char *bc = tf_compile("$if3(,,firstarg,,secondarg,lastarg)");
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    XCTAssert(!strcmp ("lastarg", buffer), @"The actual output is: %s", buffer);
}

- (void)test_IfEqualTrue_EvalsToThen {
    char *bc = tf_compile("$ifequal(100,100,then,else)");
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    XCTAssert(!strcmp ("then", buffer), @"The actual output is: %s", buffer);
}

- (void)test_IfEqualFalse_EvalsToElse {
    char *bc = tf_compile("$ifequal(100,200,then,else)");
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    XCTAssert(!strcmp ("else", buffer), @"The actual output is: %s", buffer);
}

- (void)test_IfGreaterTrue_EvalsToThen {
    char *bc = tf_compile("$ifgreater(200,100,then,else)");
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    XCTAssert(!strcmp ("then", buffer), @"The actual output is: %s", buffer);
}

- (void)test_IfGreaterFalse_EvalsToElse {
    char *bc = tf_compile("$ifgreater(100,200,then,else)");
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    XCTAssert(!strcmp ("else", buffer), @"The actual output is: %s", buffer);
}

- (void)test_GreaterIsTrue_EvalsToTrue {
    char *bc = tf_compile("$if($greater(2,1),istrue,isfalse)");
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    XCTAssert(!strcmp ("istrue", buffer), @"The actual output is: %s", buffer);
}

- (void)test_GreaterIsFalse_EvalsToFalse {
    char *bc = tf_compile("$if($greater(1,2),istrue,isfalse)");
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    XCTAssert(!strcmp ("isfalse", buffer), @"The actual output is: %s", buffer);
}

- (void)test_GreaterIsFalseEmptyArguments_EvalsToFalse {
    char *bc = tf_compile("$if($greater(,),istrue,isfalse)");
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    XCTAssert(!strcmp ("isfalse", buffer), @"The actual output is: %s", buffer);
}

- (void)test_StrCmpEmptyArguments_EvalsToTrue {
    char *bc = tf_compile("$if($strcmp(,),istrue,isfalse)");
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    XCTAssert(!strcmp ("istrue", buffer), @"The actual output is: %s", buffer);
}

- (void)test_StrCmpSameArguments_EvalsToTrue {
    char *bc = tf_compile("$if($strcmp(abc,abc),istrue,isfalse)");
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    XCTAssert(!strcmp ("istrue", buffer), @"The actual output is: %s", buffer);
}

- (void)test_IfLongerTrue_EvalsToTrue {
    char *bc = tf_compile("$iflonger(abcd,2,istrue,isfalse)");
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    XCTAssert(!strcmp ("istrue", buffer), @"The actual output is: %s", buffer);
}

- (void)test_IfLongerFalse_EvalsToFalse {
    char *bc = tf_compile("$iflonger(ab,4,istrue,isfalse)");
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    XCTAssert(!strcmp ("isfalse", buffer), @"The actual output is: %s", buffer);
}

- (void)test_SelectMiddle_EvalsToSelectedValue {
    char *bc = tf_compile("$select(3,10,20,30,40,50)");
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    XCTAssert(!strcmp ("30", buffer), @"The actual output is: %s", buffer);
}

- (void)test_SelectLeftmost_EvalsToSelectedValue {
    char *bc = tf_compile("$select(1,10,20,30,40,50)");
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    XCTAssert(!strcmp ("10", buffer), @"The actual output is: %s", buffer);
}

- (void)test_SelectRightmost_EvalsToSelectedValue {
    char *bc = tf_compile("$select(5,10,20,30,40,50)");
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    XCTAssert(!strcmp ("50", buffer), @"The actual output is: %s", buffer);
}

- (void)test_SelectOutOfBoundsLeft_EvalsToFalse {
    char *bc = tf_compile("$select(0,10,20,30,40,50)");
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    XCTAssert(!buffer[0], @"The actual output is: %s", buffer);
}

- (void)test_SelectOutOfBoundsRight_EvalsToFalse {
    char *bc = tf_compile("$select(6,10,20,30,40,50)");
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    XCTAssert(!buffer[0], @"The actual output is: %s", buffer);
}

- (void)test_InvalidPercentExpression_WithNullTrack_NoCrash {
    char *bc = tf_compile("begin - %version% - end");

    ctx.it = NULL;
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp ("begin -  - end", buffer), @"The actual output is: %s", buffer);
}

- (void)test_InvalidPercentExpression_WithNullTrackAccessingMetadata_NoCrash {
    char *bc = tf_compile("begin - %title% - end");
    ctx.it = NULL;
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp ("begin -  - end", buffer), @"The actual output is: %s", buffer);
}

- (void)test_Index_WithNullPlaylist_NoCrash {
    char *bc = tf_compile("begin - %list_index% - end");
    ctx.it = NULL;
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp ("begin - 0 - end", buffer), @"The actual output is: %s", buffer);
}

- (void)test_Div5by2_Gives3 {
    char *bc = tf_compile("$div(5,2)");
    ctx.it = NULL;
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp ("3", buffer), @"The actual output is: %s", buffer);
}

- (void)test_Div4pt9by1pt9_Gives4 {
    char *bc = tf_compile("$div(4.9,1.9)");
    ctx.it = NULL;
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp ("4", buffer), @"The actual output is: %s", buffer);
}

- (void)test_Div20by2by5_Gives2 {
    char *bc = tf_compile("$div(20,2,5)");
    ctx.it = NULL;
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp ("2", buffer), @"The actual output is: %s", buffer);
}

- (void)test_DivBy0_GivesEmpty {
    char *bc = tf_compile("$div(5,0)");
    ctx.it = NULL;
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!buffer[0], @"The actual output is: %s", buffer);
}

- (void)test_Max0Arguments_GivesEmpty {
    char *bc = tf_compile("$max()");
    ctx.it = NULL;
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!buffer[0], @"The actual output is: %s", buffer);
}

- (void)test_MaxOf1and2_Gives2 {
    char *bc = tf_compile("$max(1,2)");
    ctx.it = NULL;
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp ("2", buffer), @"The actual output is: %s", buffer);
}

- (void)test_MaxOf30and50and20_Gives50 {
    char *bc = tf_compile("$max(30,50,20)");
    ctx.it = NULL;
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp ("50", buffer), @"The actual output is: %s", buffer);
}

- (void)test_MinOf1and2_Gives1 {
    char *bc = tf_compile("$min(1,2)");
    ctx.it = NULL;
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp ("1", buffer), @"The actual output is: %s", buffer);
}

- (void)test_MinOf30and50and20_Gives20 {
    char *bc = tf_compile("$min(30,50,20)");
    ctx.it = NULL;
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp ("20", buffer), @"The actual output is: %s", buffer);
}

- (void)test_ModOf3and2_Gives1 {
    char *bc = tf_compile("$mod(3,2)");
    ctx.it = NULL;
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp ("1", buffer), @"The actual output is: %s", buffer);
}

- (void)test_ModOf6and3_Gives0 {
    char *bc = tf_compile("$mod(6,3)");
    ctx.it = NULL;
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp ("0", buffer), @"The actual output is: %s", buffer);
}

- (void)test_ModOf16and18and9_Gives7 {
    char *bc = tf_compile("$mod(16,18,9)");
    ctx.it = NULL;
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp ("7", buffer), @"The actual output is: %s", buffer);
}

- (void)test_Mul2and5_Gives10 {
    char *bc = tf_compile("$mul(2,5)");
    ctx.it = NULL;
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp ("10", buffer), @"The actual output is: %s", buffer);
}

- (void)test_MulOf2and3and4_Gives24 {
    char *bc = tf_compile("$mul(2,3,4)");
    ctx.it = NULL;
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp ("24", buffer), @"The actual output is: %s", buffer);
}

- (void)test_MulDiv2and10and4_Gives5 {
    char *bc = tf_compile("$muldiv(2,10,4)");
    ctx.it = NULL;
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp ("5", buffer), @"The actual output is: %s", buffer);
}

- (void)test_MulDiv2and10and0_GivesEmpty {
    char *bc = tf_compile("$muldiv(2,10,0)");
    ctx.it = NULL;
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!buffer[0], @"The actual output is: %s", buffer);
}

- (void)test_MulDiv2and3and4_Gives2 {
    char *bc = tf_compile("$muldiv(2,3,4)");
    ctx.it = NULL;
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp ("2", buffer), @"The actual output is: %s", buffer);
}

- (void)test_Rand_GivesANumber {
    char *bc = tf_compile("$rand()");
    ctx.it = NULL;
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    int num_digits = 0;
    for (int i = 0; buffer[i]; i++) {
        if (isdigit(buffer[i])) {
            num_digits++;
        }
    }
    XCTAssert(num_digits == strlen (buffer), @"The actual output is: %s", buffer);
}

- (void)test_RandWithArgs_GivesEmpty {
    char *bc = tf_compile("$rand(1)");
    ctx.it = NULL;
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    int num_digits = 0;
    for (int i = 0; buffer[i]; i++) {
        if (isdigit(buffer[i])) {
            num_digits++;
        }
    }
    XCTAssert(!strcmp (buffer, ""), @"The actual output is: %s", buffer);
}

- (void)test_SubWithoutArgs_GivesEmpty {
    char *bc = tf_compile("$sub()");
    ctx.it = NULL;
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    int num_digits = 0;
    for (int i = 0; buffer[i]; i++) {
        if (isdigit(buffer[i])) {
            num_digits++;
        }
    }
    XCTAssert(!strcmp (buffer, ""), @"The actual output is: %s", buffer);
}

- (void)test_SubWith1Arg_GivesEmpty {
    char *bc = tf_compile("$sub(2)");
    ctx.it = NULL;
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    int num_digits = 0;
    for (int i = 0; buffer[i]; i++) {
        if (isdigit(buffer[i])) {
            num_digits++;
        }
    }
    XCTAssert(!strcmp (buffer, ""), @"The actual output is: %s", buffer);
}

- (void)test_SubWith3and2_Gives1 {
    char *bc = tf_compile("$sub(3,2)");
    ctx.it = NULL;
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    int num_digits = 0;
    for (int i = 0; buffer[i]; i++) {
        if (isdigit(buffer[i])) {
            num_digits++;
        }
    }
    XCTAssert(!strcmp (buffer, "1"), @"The actual output is: %s", buffer);
}

- (void)test_SubWith10and5and2_Gives3 {
    char *bc = tf_compile("$sub(10,5,2)");
    ctx.it = NULL;
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    int num_digits = 0;
    for (int i = 0; buffer[i]; i++) {
        if (isdigit(buffer[i])) {
            num_digits++;
        }
    }
    XCTAssert(!strcmp (buffer, "3"), @"The actual output is: %s", buffer);
}

- (void)test_ChannelsFor3ChTrack_Gives3 {
    pl_replace_meta (it, ":CHANNELS", "3");
    char *bc = tf_compile("%channels%");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "3"), @"The actual output is: %s", buffer);
}

- (void)test_ChannelsFuncForMonoTrack_GivesMono {
    pl_replace_meta (it, ":CHANNELS", "1");
    char *bc = tf_compile("$channels()");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "mono"), @"The actual output is: %s", buffer);
}

- (void)test_ChannelsFuncForUnsetChannels_GivesStereo {
    char *bc = tf_compile("$channels()");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "stereo"), @"The actual output is: %s", buffer);
}

- (void)test_AndTrueArgs_ReturnsTrue {
    pl_replace_meta (it, "artist", "artist");
    pl_replace_meta (it, "album", "album");

    char *bc = tf_compile("$if($and(%artist%,%album%),true,false)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "true"), @"The actual output is: %s", buffer);
}

- (void)test_AndTrueAndFalseArgs_ReturnsFalse {
    pl_replace_meta (it, "artist", "artist");

    char *bc = tf_compile("$if($and(%artist%,%album%),true,false)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "false"), @"The actual output is: %s", buffer);
}

- (void)test_AndFalseArgs_ReturnsFalse {
    char *bc = tf_compile("$if($and(%artist%,%album%),true,false)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "false"), @"The actual output is: %s", buffer);
}

- (void)test_OrTrueAndFalseArgs_ReturnsTrue {
    pl_replace_meta (it, "artist", "artist");

    char *bc = tf_compile("$if($or(%artist%,%album%),true,false)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "true"), @"The actual output is: %s", buffer);
}

- (void)test_OrTrueArgs_ReturnsTrue {
    pl_replace_meta (it, "artist", "artist");
    pl_replace_meta (it, "album", "album");

    char *bc = tf_compile("$if($or(%artist%,%album%),true,false)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "true"), @"The actual output is: %s", buffer);
}

- (void)test_OrFalseArgs_ReturnsFalse {
    char *bc = tf_compile("$if($or(%artist%,%album%),true,false)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "false"), @"The actual output is: %s", buffer);
}

- (void)test_NotTrueArg_ReturnsFalse {
    pl_replace_meta (it, "artist", "artist");
    char *bc = tf_compile("$if($not(%artist%),true,false)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "false"), @"The actual output is: %s", buffer);
}

- (void)test_NotFalseArg_ReturnsTrue {
    char *bc = tf_compile("$if($not(%artist%),true,false)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "true"), @"The actual output is: %s", buffer);
}

- (void)test_XorTrueAndTrue_ReturnsFalse {
    pl_replace_meta (it, "artist", "artist");
    pl_replace_meta (it, "album", "album");
    char *bc = tf_compile("$if($xor(%artist%,%album%),true,false)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "false"), @"The actual output is: %s", buffer);
}

- (void)test_XorTrueAndFalse_ReturnsTrue {
    pl_replace_meta (it, "artist", "artist");
    char *bc = tf_compile("$if($xor(%artist%,%album%),true,false)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "true"), @"The actual output is: %s", buffer);
}

- (void)test_XorFalseTrueFalse_ReturnsTrue {
    pl_replace_meta (it, "artist", "artist");
    char *bc = tf_compile("$if($xor(%album%,%artist%,%album%),true,false)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "true"), @"The actual output is: %s", buffer);
}

- (void)test_PlayingColumnWithEmptyFormat_GivesQueueIndexes {
    playqueue_push (it);
    ctx.id = DB_COLUMN_PLAYING;
    ctx.flags |= DDB_TF_CONTEXT_HAS_ID;
    tf_eval (&ctx, NULL, buffer, 1000);
    XCTAssert(!strcmp (buffer, "(1)"), @"The actual output is: %s", buffer);
    playqueue_pop ();
}

- (void)test_LengthSamplesOf100Start300End_Returns200 {
    pl_item_set_startsample (it, 100);
    pl_item_set_endsample (it, 300);
    char *bc = tf_compile("%length_samples%");
    tf_eval (&ctx, bc, buffer, 1000);
    XCTAssert(!strcmp (buffer, "200"), @"The actual output is: %s", buffer);
}

- (void)test_AbbrTestString_ReturnsAbbreviatedString {
    pl_item_set_startsample (it, 100);
    pl_item_set_endsample (it, 300);
    char *bc = tf_compile("$abbr('This is a Long Title (12-inch version) [needs tags]')");
    tf_eval (&ctx, bc, buffer, 1000);
    XCTAssert(!strcmp (buffer, "TiaLT1v[needst"), @"The actual output is: %s", buffer);
}

- (void)test_AbbrTestUnicodeString_ReturnsAbbreviatedString {
    pl_item_set_startsample (it, 100);
    pl_item_set_endsample (it, 300);
    char *bc = tf_compile("$abbr('This ɀHİJ a русский Title (12-inch version) [needs tags]')");
    tf_eval (&ctx, bc, buffer, 1000);
    XCTAssert(!strcmp (buffer, "TɀaрT1v[needst"), @"The actual output is: %s", buffer);
}

- (void)test_AnsiTestString_ReturnsTheSameString {
    char *bc = tf_compile("$ansi(ABCDабвг)");
    tf_eval (&ctx, bc, buffer, 1000);
    XCTAssert(!strcmp (buffer, "ABCDабвг"), @"The actual output is: %s", buffer);
}

- (void)test_AsciiTestString_ReturnsAsciiSameStringWithInvalidCharsStripped {
    char *bc = tf_compile("$ascii(олдABCDабвг)");
    tf_eval (&ctx, bc, buffer, 1000);
    XCTAssert(!strcmp (buffer, "ABCD"), @"The actual output is: %s", buffer);
}


- (void)test_CapsTestAsciiString_ReturnsCapitalizeEachWordString {
    char *bc = tf_compile("$caps(MY TEST STRING)");
    tf_eval (&ctx, bc, buffer, 1000);
    XCTAssert(!strcmp (buffer, "My Test String"), @"The actual output is: %s", buffer);
}


- (void)test_CapsTestAsciiRandomizedString_ReturnsCapitalizeEachWordString {
    char *bc = tf_compile("$caps(MY TesT STriNG)");
    tf_eval (&ctx, bc, buffer, 1000);
    XCTAssert(!strcmp (buffer, "My Test String"), @"The actual output is: %s", buffer);
}

- (void)test_CapsTestUnicodeRandomizedString_ReturnsCapitalizeEachWordString {
    char *bc = tf_compile("$caps(AsciiAlbumName РуССкоЕНазВАние ΠΥΘΑΓΌΡΑΣ)");
    tf_eval (&ctx, bc, buffer, 1000);
    XCTAssert(!strcmp (buffer, "Asciialbumname Русскоеназвание Πυθαγόρασ"), @"The actual output is: %s", buffer);
}

- (void)test_CapsTestUnicodeStringWithNonMatchinByteLengthsForLowerUpperCaseChars_ReturnsCapitalizeEachWordString {
    char *bc = tf_compile("$caps(ɑBCD ɀHİJ)");
    tf_eval (&ctx, bc, buffer, 1000);
    XCTAssert(!strcmp (buffer, "Ɑbcd Ɀhij"), @"The actual output is: %s", buffer);
}

- (void)test_Caps2TestUnicodeRandomizedString_ReturnsCapitalizeEachWordString {
    char *bc = tf_compile("$caps2(ɑBCD ɀHİJ)");
    tf_eval (&ctx, bc, buffer, 1000);
    XCTAssert(!strcmp (buffer, "ⱭBCD ⱿHİJ"), @"The actual output is: %s", buffer);
}

- (void)test_Char1055And88And38899_ReturnsCorrespondingUTF8Chars {
    char *bc = tf_compile("$char(1055)$char(88)$char(38899)");
    tf_eval (&ctx, bc, buffer, 1000);
    XCTAssert(!strcmp (buffer, "ПX音"), @"The actual output is: %s", buffer);
}

- (void)test_Crc32Of123456789_Returns3421780262 {
    char *bc = tf_compile("$crc32(123456789)");
    tf_eval (&ctx, bc, buffer, 1000);
    XCTAssert(!strcmp (buffer, "3421780262"), @"The actual output is: %s", buffer);
}

- (void)test_CrLf_InsertsLinebreak {
    ctx.flags |= DDB_TF_CONTEXT_MULTILINE;
    char *bc = tf_compile("$crlf()");
    tf_eval (&ctx, bc, buffer, 1000);
    XCTAssert(!strcmp (buffer, "\n"), @"The actual output is: %s", buffer);
}

- (void)test_DirectoryOnFilePath_ReturnsDirectory {
    char *bc = tf_compile("$directory(/directory/file.path)");
    tf_eval (&ctx, bc, buffer, 1000);
    XCTAssert(!strcmp (buffer, "directory"), @"The actual output is: %s", buffer);
}

- (void)test_DirectoryOnFilePathWithMultipleSlashes_ReturnsDirectory {
    char *bc = tf_compile("$directory(/directory///file.path)");
    tf_eval (&ctx, bc, buffer, 1000);
    XCTAssert(!strcmp (buffer, "directory"), @"The actual output is: %s", buffer);
}

- (void)test_DirectoryOnFilePathWithoutFrontSlash_ReturnsDirectory {
    char *bc = tf_compile("$directory(directory/file.path)");
    tf_eval (&ctx, bc, buffer, 1000);
    XCTAssert(!strcmp (buffer, "directory"), @"The actual output is: %s", buffer);
}

- (void)test_DirectoryOnFilePathWithMoreNestedness_ReturnsDirectory {
    char *bc = tf_compile("$directory(/path/directory/file.path)");
    tf_eval (&ctx, bc, buffer, 1000);
    XCTAssert(!strcmp (buffer, "directory"), @"The actual output is: %s", buffer);
}

- (void)test_DirectoryOnFilePathWithoutDirectory_ReturnsEmpty {
    char *bc = tf_compile("$directory(file.path)");
    tf_eval (&ctx, bc, buffer, 1000);
    XCTAssert(!strcmp (buffer, ""), @"The actual output is: %s", buffer);
}

- (void)test_DirectoryOnFilePathAtRoot_ReturnsEmpty {
    char *bc = tf_compile("$directory(/file.path)");
    tf_eval (&ctx, bc, buffer, 1000);
    XCTAssert(!strcmp (buffer, ""), @"The actual output is: %s", buffer);
}

- (void)test_DirectoryOnEmptyAtRoot_ReturnsEmpty {
    char *bc = tf_compile("$directory(/)");
    tf_eval (&ctx, bc, buffer, 1000);
    XCTAssert(!strcmp (buffer, ""), @"The actual output is: %s", buffer);
}

- (void)test_DirectoryOnEmpty_ReturnsEmpty {
    char *bc = tf_compile("$directory()");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, ""), @"The actual output is: %s", buffer);
}

- (void)test_DirectoryOnFilePathLevel0_ReturnsEmpty {
    char *bc = tf_compile("$directory(/directory/file.path,0)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, ""), @"The actual output is: %s", buffer);
}

- (void)test_DirectoryOnFilePathLevel1_ReturnsDirectory1 {
    char *bc = tf_compile("$directory(/directory3/directory2/directory1/file.path,1)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "directory1"), @"The actual output is: %s", buffer);
}

- (void)test_DirectoryOnFilePathLevel2_ReturnsDirectory2 {
    char *bc = tf_compile("$directory(/directory3/directory2/directory1/file.path,2)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "directory2"), @"The actual output is: %s", buffer);
}

- (void)test_DirectoryOnFilePathLevel4_ReturnsEmpty {
    char *bc = tf_compile("$directory(/directory3/directory2/directory/file.path,4)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, ""), @"The actual output is: %s", buffer);
}

- (void)test_DirectoryOnFilePathLevel2MultipleSlashes_ReturnsDirectory2 {
    char *bc = tf_compile("$directory(////directory3////directory2////directory////file.path,2)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "directory2"), @"The actual output is: %s", buffer);
}

- (void)test_MultiLine_LineBreaksIgnored {
    char *bc = tf_compile("hello\nworld");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "helloworld"), @"The actual output is: %s", buffer);
}

- (void)test_MultiLineWithComments_LineBreaksAndCommentedLinesIgnored {
    char *bc = tf_compile("// this is a comment\nhello\nworld\n//another comment\nmore text");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "helloworldmore text"), @"The actual output is: %s", buffer);
}

- (void)test_QuotedSpecialChars_TreatedLiterally {
    char *bc = tf_compile("'blah$blah%blah[][]'");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "blah$blah%blah[][]"), @"The actual output is: %s", buffer);
}

- (void)test_FunctionArgumentsOnMultipleLinesWithComments_LinebreaksAndCommentsIgnored {
    char *bc = tf_compile("$add(1,\n2,\n3,//4,\n5)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "11"), @"The actual output is: %s", buffer);
}

- (void)test_DirectoryPathOnFilePath_ReturnsDirectoryPath {
    char *bc = tf_compile("$directory_path('/a/b/c/d.mp3')");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "/a/b/c"), @"The actual output is: %s", buffer);
}

- (void)test_DirectoryPathOnPathWithoutFile_ReturnsDirectoryPath {
    char *bc = tf_compile("$directory_path('/a/b/c/d/')");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "/a/b/c/d"), @"The actual output is: %s", buffer);
}

- (void)test_ExtOnFilePath_ReturnsExt {
    char *bc = tf_compile("$ext('/a/b/c/d/file.mp3')");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "mp3"), @"The actual output is: %s", buffer);
}

- (void)test_ExtOnFileWithoutExtPath_ReturnsEmpty {
    char *bc = tf_compile("$ext('/a/b/c/d/file')");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, ""), @"The actual output is: %s", buffer);
}

- (void)test_ExtOnFilePathWithoutFilename_ReturnsEmpty {
    char *bc = tf_compile("$ext('/a/b/c/d/')");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, ""), @"The actual output is: %s", buffer);
}

- (void)test_ExtOnFilePathEndingWithDot_ReturnsEmpty {
    char *bc = tf_compile("$ext('/a/b/c/d/file.')");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, ""), @"The actual output is: %s", buffer);
}

- (void)test_ExtOnFilePathDotFile_ReturnsExt {
    char *bc = tf_compile("$ext('/a/b/c/d/.ext')");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "ext"), @"The actual output is: %s", buffer);
}

- (void)test_ExtOnFileExtWithMultiplePeriod_ReturnsExt {
    char *bc = tf_compile("$ext('/a/b/c/d/file.iso.wv')");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "wv"), @"The actual output is: %s", buffer);
}

- (void)test_FilenameOnFilePath_ReturnsFilename {
    char *bc = tf_compile("$filename('/a/b/c/d/file.mp3')");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "file.mp3"), @"The actual output is: %s", buffer);
}

- (void)test_FilenameOnFilePathWithoutFile_ReturnsEmpty {
    char *bc = tf_compile("$filename('/a/b/c/d/')");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, ""), @"The actual output is: %s", buffer);
}

- (void)test_FilenameOnFilenameWithoutPath_ReturnsFilename {
    char *bc = tf_compile("$filename('file.iso.wv')");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "file.iso.wv"), @"The actual output is: %s", buffer);
}

- (void)test_FilenameMeta_ReturnsFilename {
    pl_replace_meta (it, ":URI", "/path/file.mp3");
    char *bc = tf_compile("%filename%");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "file"), @"The actual output is: %s", buffer);
}

- (void)test_Date_ReturnsYearValue {
    pl_replace_meta (it, "year", "1980");
    char *bc = tf_compile("%date%");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "1980"), @"The actual output is: %s", buffer);
}

- (void)test_CustomField_ReturnsTheFieldValue {
    pl_replace_meta (it, "random_name", "random value");
    char *bc = tf_compile("%random_name%");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "random value"), @"The actual output is: %s", buffer);
}

- (void)test_MultipleArtists_ReturnsArtistsSeparatedByCommas {
    pl_append_meta (it, "artist", "Artist1");
    pl_append_meta (it, "artist", "Artist2");
    char *bc = tf_compile("%artist%");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "Artist1, Artist2"), @"The actual output is: %s", buffer);
}

- (void)test_EmptyTitle_YieldsFilename {
    pl_replace_meta (it, ":URI", "/home/user/filename.mp3");
    char *bc = tf_compile("%title%");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "filename"), @"The actual output is: %s", buffer);
}

- (void)test_DoublingPercentDollarApostrophe_OutputsSinglePercentDollarApostrophe {
    char *bc = tf_compile("''$$%%");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "'$%"), @"The actual output is: %s", buffer);
}

- (void)test_PlaybackTime_OutputsPlaybackTime {
    streamer_set_playing_track (it);
    char *bc = tf_compile("%playback_time%");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "0:00"), @"The actual output is: %s", buffer);
}

- (void)test_PlaybackTime_OutputsPlaybackTimeMs {
    streamer_set_playing_track (it);
    char *bc = tf_compile("%playback_time_ms%");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "0:00.000"), @"The actual output is: %s", buffer);
}

- (void)test_NoDynamicFlag_SkipsDynamicFields {
    char *bc = tf_compile("header|%playback_time%|footer");
    ctx.flags |= DDB_TF_CONTEXT_NO_DYNAMIC;
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "header||footer"), @"The actual output is: %s", buffer);
}

- (void)test_Track_Number_SingleDigit_ReturnsNonZeroPaddedTrackNumber {
    pl_replace_meta (it, "track", "5");
    char *bc = tf_compile("%track number%");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "5"), @"The actual output is: %s", buffer);
}

- (void)test_Track_Number_NonNumerical_ReturnsUnmodified {
    pl_replace_meta (it, "track", "A01");
    char *bc = tf_compile("%track number%");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "A01"), @"The actual output is: %s", buffer);
}

- (void)test_Track_Number_PaddingZero_ReturnsUnmodified {
    pl_replace_meta (it, "track", "001");
    char *bc = tf_compile("%track number%");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "001"), @"The actual output is: %s", buffer);
}

- (void)test_TrackNumber_SingleDigit_PaddingZeroAdded {
    pl_replace_meta (it, "track", "1");
    char *bc = tf_compile("%tracknumber%");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "01"), @"The actual output is: %s", buffer);
}

- (void)test_TrackNumber_PaddingZero_ReturnsUnmodified {
    pl_replace_meta (it, "track", "001");
    char *bc = tf_compile("%tracknumber%");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "001"), @"The actual output is: %s", buffer);
}

- (void)test_Length_DoesntGetPaddedWithSpace {
    plt_set_item_duration(NULL, it, 130);
    char *bc = tf_compile("%length%");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "2:10"), @"The actual output is: %s", buffer);
}

- (void)test_ImportLegacyDirectAccess_ProducesExpectedData {
    const char *old = "%@disc@";
    char new[100];
    tf_import_legacy (old, new, sizeof (new));
    XCTAssert(!strcmp (new, "%disc%"), @"The actual output is: %s", buffer);
}

- (void)test_NestedSquareBracketsWithUndefVarsAndLiteralData_ReturnEmpty {
    pl_replace_meta (it, "title", "title");
    char *bc = tf_compile("[[%discnumber%]a] normaltext [%title%] [[%title%]a]");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, " normaltext title titlea"), @"The actual output is: %s", buffer);
}

- (void)test_FixEof_PutsIndicatorAfterLineBreak {
    pl_replace_meta (it, "title", "line1\nline2\n");
    char *bc = tf_compile("$fix_eol(%title%)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "line1 (...)"), @"The actual output is: %s", buffer);
}

- (void)test_FixEofTwoArgs_PutsCustomIndicatorAfterLineBreak {
    pl_replace_meta (it, "title", "line1\nline2\n");
    char *bc = tf_compile("$fix_eol(%title%, _..._)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "line1 _..._"), @"The actual output is: %s", buffer);
}

- (void)test_FixEofTwoArgsWithSmallBuffer_DoesntOverflowOffByOne {
    pl_replace_meta (it, "title", "hello\n");
    char *bc = tf_compile("$fix_eol(%title%, _..._)");
    tf_eval (&ctx, bc, buffer, 12);
    XCTAssert(!strcmp (buffer, "hello _..._"), @"The actual output is: %s", buffer);
    tf_eval (&ctx, bc, buffer, 11);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, ""), @"The actual output is: %s", buffer);
}

- (void)test_Hex_ReturnsHexConvertedNumber {
    char *bc = tf_compile("$hex(11259375)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "abcdef"), @"The actual output is: %s", buffer);
}

- (void)test_HexPadded_ReturnsHexConvertedNumberWithPadding {
    char *bc = tf_compile("$hex(11259375,10)");
    tf_eval (&ctx, bc, buffer, 10);
    XCTAssert(!strcmp (buffer, ""), @"The actual output is: %s", buffer);
    tf_eval (&ctx, bc, buffer, 11);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "0000abcdef"), @"The actual output is: %s", buffer);
}

- (void)test_HexZero_ReturnsZero {
    char *bc = tf_compile("$hex(0)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "0"), @"The actual output is: %s", buffer);
}

- (void)test_QuotedSquareBrackets_ReturnsSquareBrackets {
    char *bc = tf_compile("'['']'");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "[]"), @"The actual output is: %s", buffer);
}

- (void)test_ImportLegacySquareBrackets_ProducesQuotedSquareBrackets {
    const char *old = "[%y]";
    char new[100];
    tf_import_legacy (old, new, sizeof (new));
    XCTAssert(!strcmp (new, "'['%date%']'"), @"The actual output is: %s", buffer);
}

- (void)test_Num_123_5_Returns_00123 {
    char *bc = tf_compile("$num(123,5)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "00123"), @"The actual output is: %s", buffer);
}

- (void)test_Num_Minus123_5_Returns__0123 {
    char *bc = tf_compile("$num(-123,5)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "-0123"), @"The actual output is: %s", buffer);
}

- (void)test_NumFractional_ReturnsIntegerFloor {
    char *bc = tf_compile("$num(4.8,5)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "00004"), @"The actual output is: %s", buffer);
}

- (void)test_NumNonNumber_ReturnsZero {
    char *bc = tf_compile("$num(A1,5)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "00000"), @"The actual output is: %s", buffer);
}

- (void)test_NumLargeNumber_DoesntTruncate {
    char *bc = tf_compile("$num(1234,3)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "1234"), @"The actual output is: %s", buffer);
}

- (void)test_NumNegativePadding_GivesZeroPadding {
    char *bc = tf_compile("$num(1,-3)");
    tf_eval (&ctx, bc, buffer, 1000);
    XCTAssert(!strcmp (buffer, "1"), @"The actual output is: %s", buffer);
}

- (void)test_IsPlayingReturnValueTrue_CorrespondsToStringValue {
    streamer_set_playing_track (it);
    plug_set_output (&fake_out);
    fake_out_state_value = OUTPUT_STATE_PLAYING;
    char *bc = tf_compile("$if(%isplaying%,YES,NO) %isplaying%");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "YES 1"), @"The actual output is: %s", buffer);
}

- (void)test_IsPlayingReturnValueFalse_CorrespondsToStringValue {
    plug_set_output (&fake_out);
    fake_out_state_value = OUTPUT_STATE_STOPPED;
    char *bc = tf_compile("$if(%isplaying%,YES,NO) %isplaying%");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "NO "), @"The actual output is: %s", buffer);
}

- (void)test_IsPausedReturnValueTrue_CorrespondsToStringValue {
    streamer_set_playing_track (it);
    plug_set_output (&fake_out);
    fake_out_state_value = OUTPUT_STATE_PAUSED;
    char *bc = tf_compile("$if(%ispaused%,YES,NO) %ispaused%");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "YES 1"), @"The actual output is: %s", buffer);
}

- (void)test_MultiValueField_OutputAsCommaSeparated {
    pl_append_meta(it, "artist", "Value1");
    pl_append_meta(it, "artist", "Value2");
    pl_append_meta(it, "artist", "Value3");
    char *bc = tf_compile("%artist%");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "Value1, Value2, Value3"), @"The actual output is: %s", buffer);
}

- (void)test_LinebreaksAndTabs_OutputAsUnderscores {
    pl_append_meta(it, "artist", "Text1\r\nText2\tText3");
    char *bc = tf_compile("%artist%");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "Text1__Text2_Text3"), @"The actual output is: %s", buffer);
}

- (void)test_NestedSquareBracketsWithDefinedAndUndefinedVars_ReturnNonEmpty {
    pl_replace_meta (it, "title", "title");
    char *bc = tf_compile("header [[%discnumber%][%title%] a] footer");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "header title a footer"), @"The actual output is: %s", buffer);
}

- (void)test_PathStripFileUriScheme_ReturnStripped {
    pl_replace_meta (it, ":URI", "file:///home/user/filename.mp3");
    char *bc = tf_compile("%path%");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "/home/user/filename.mp3"), @"The actual output is: %s", buffer);
}

- (void)test_PathStripHTTPUriScheme_ReturnUnStripped {
    pl_replace_meta (it, ":URI", "http://example.com/filename.mp3");
    char *bc = tf_compile("%path%");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "http://example.com/filename.mp3"), @"The actual output is: %s", buffer);
}

- (void)test_RawPathWithFileUriScheme_ReturnUnStripped {
    pl_replace_meta (it, ":URI", "file:///home/user/filename.mp3");
    char *bc = tf_compile("%_path_raw%");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "file:///home/user/filename.mp3"), @"The actual output is: %s", buffer);
}

- (void)test_RawPathWithHttpUriScheme_ReturnUnStripped {
    pl_replace_meta (it, ":URI", "http://example.com/filename.mp3");
    char *bc = tf_compile("%_path_raw%");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "http://example.com/filename.mp3"), @"The actual output is: %s", buffer);
}

- (void)test_RawPathAbsolutePath_ReturnsExpected {
    pl_replace_meta (it, ":URI", "/path/to/filename.mp3");
    char *bc = tf_compile("%_path_raw%");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "file:///path/to/filename.mp3"), @"The actual output is: %s", buffer);
}

- (void)test_RawPathRelativePath_ReturnsEmpty {
    pl_replace_meta (it, ":URI", "relative/path/to/filename.mp3");
    char *bc = tf_compile("%_path_raw%");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, ""), @"The actual output is: %s", buffer);
}

- (void)test_PlaylistName_ReturnsPlaylistName {
    char *bc = tf_compile("%_playlist_name%");
    playlist_t plt = {
        .title = "Test Playlist",
    };
    ctx.plt = (ddb_playlist_t *)&plt;
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "Test Playlist"), @"The actual output is: %s", buffer);
}

- (void)test_ReplaceWith3Arguments_ReturnsExpectedValue {
    char *bc = tf_compile("$replace(ab,a,b,b,c)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "bc"), @"The actual output is: %s", buffer);
}

- (void)test_ReplaceWith3ArgumentsNested_ReturnsExpectedValue {
    char *bc = tf_compile("$replace($replace(ab,a,b),b,c)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "cc"), @"The actual output is: %s", buffer);
}

- (void)test_ReplaceWith1Argument_ReturnsEmpty {
    char *bc = tf_compile("$replace(ab)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, ""), @"The actual output is: %s", buffer);
}

- (void)test_ReplaceWith2Arguments_ReturnsEmpty {
    char *bc = tf_compile("$replace(ab,a)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, ""), @"The actual output is: %s", buffer);
}

- (void)test_ReplaceWith4Arguments_ReturnsEmpty {
    char *bc = tf_compile("$replace(ab,a,c,d)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, ""), @"The actual output is: %s", buffer);
}

- (void)test_ReplaceLongerSubstring_ReturnsExpected {
    char *bc = tf_compile("$replace(foobar,foo,DeaD,bar,BeeF)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "DeaDBeeF"), @"The actual output is: %s", buffer);
}

- (void)test_FilenameExt_ReturnsFilenameWithExt {
    pl_replace_meta (it, ":URI", "/Users/User/MyFile.mod");
    char *bc = tf_compile("%filename_ext%");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "MyFile.mod"), @"The actual output is: %s", buffer);
}

- (void)test_UpperForAllLowerCase_ReturnsUppercase {
    char *bc = tf_compile("$upper(abcd)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "ABCD"), @"The actual output is: %s", buffer);
}

- (void)test_UpperForAllUpperCase_ReturnsUppercase {
    char *bc = tf_compile("$upper(ABCD)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "ABCD"), @"The actual output is: %s", buffer);
}

- (void)test_UpperForMixed_ReturnsUppercase {
    char *bc = tf_compile("$upper(aBcD)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "ABCD"), @"The actual output is: %s", buffer);
}

- (void)test_UpperForAllLowerCaseNonAscii_ReturnsUppercase {
    char *bc = tf_compile("$upper(абвгд)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "АБВГД"), @"The actual output is: %s", buffer);
}

- (void)test_LowerForAllUpperCase_ReturnsLowercase {
    char *bc = tf_compile("$lower(ABCD)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "abcd"), @"The actual output is: %s", buffer);
}

- (void)test_LowerForAllLowerCase_ReturnsLowercase {
    char *bc = tf_compile("$lower(abcd)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "abcd"), @"The actual output is: %s", buffer);
}

- (void)test_LowerForMixed_ReturnsLowercase {
    char *bc = tf_compile("$lower(aBcD)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "abcd"), @"The actual output is: %s", buffer);
}

- (void)test_LowerForAllUpperCaseNonAscii_ReturnsLowercase {
    char *bc = tf_compile("$lower(АБВГД)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "абвгд"), @"The actual output is: %s", buffer);
}

- (void)test_PathWithNullUri_ReturnsEmpty {
    pl_delete_meta (it, ":URI");
    char *bc = tf_compile("%path%");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, ""), @"The actual output is: %s", buffer);
}

- (void)test_RawPathWithNullUri_ReturnsEmpty {
    pl_delete_meta (it, ":URI");
    char *bc = tf_compile("%_path_raw%");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, ""), @"The actual output is: %s", buffer);
}

- (void)test_VfsPathTitle_GivesCorrectTitle {
    pl_replace_meta (it, ":URI", "/path/file/myfile.zip:mytrack.mp3");
    char *bc = tf_compile("%title%");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "mytrack"), @"The actual output is: %s", buffer);
}

- (void)test_RepeatSingleChar11Times_Gives11Chars {
    char *bc = tf_compile("$repeat(x,11)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "xxxxxxxxxxx"), @"The actual output is: %s", buffer);
}

- (void)test_RepeatTwoChars3Times_Gives3DoubleChars {
    char *bc = tf_compile("$repeat(xy,3)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "xyxyxy"), @"The actual output is: %s", buffer);
}

- (void)test_RepeatCalculatedExpr2Times_Gives2Exprs {
    pl_replace_meta (it, "title", "abc");
    char *bc = tf_compile("$repeat(%title%,2)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "abcabc"), @"The actual output is: %s", buffer);
}

- (void)test_RepeatCalculatedExprNTimes_GivesNExprs {
    pl_replace_meta (it, "title", "abc");
    pl_replace_meta (it, "count", "3");
    char *bc = tf_compile("$repeat(%title%,%count%)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "abcabcabc"), @"The actual output is: %s", buffer);
}

- (void)test_RepeatSingleCharZeroTimes_GivesZeroChars {
    char *bc = tf_compile("pre$repeat(x,0)post");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "prepost"), @"The actual output is: %s", buffer);
}

- (void)test_InsertStrMiddle_GivesInsertedStr {
    pl_replace_meta (it, "title", "Insert [] Here");
    pl_replace_meta (it, "album", "Value");
    char *bc = tf_compile("$insert(%title%,%album%,8)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "Insert [Value] Here"), @"The actual output is: %s", buffer);
}

- (void)test_InsertStrMiddleUnicode_GivesInsertedStr {
    pl_replace_meta (it, "title", "Вставить [] сюда");
    pl_replace_meta (it, "album", "Значение");
    char *bc = tf_compile("$insert(%title%,%album%,10)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "Вставить [Значение] сюда"), @"The actual output is: %s", buffer);
}

- (void)test_InsertStrEnd_GivesAppendedStr {
    pl_replace_meta (it, "title", "Insert Here:");
    pl_replace_meta (it, "album", "Value");
    char *bc = tf_compile("$insert(%title%,%album%,12)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "Insert Here:Value"), @"The actual output is: %s", buffer);
}

- (void)test_InsertStrOutOfBounds_GivesAppendedStr {
    pl_replace_meta (it, "title", "Insert Here:");
    pl_replace_meta (it, "album", "Value");
    char *bc = tf_compile("$insert(%title%,%album%,13)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "Insert Here:Value"), @"The actual output is: %s", buffer);
}

- (void)test_InsertStrBufferTooSmallUnicode_GivesTruncatedAtBeforeStr {
    pl_replace_meta (it, "title", "Вставить [] сюда");
    pl_replace_meta (it, "album", "Значение");
    char *bc = tf_compile("$insert(%title%,%album%,10)");
    tf_eval (&ctx, bc, buffer, 5);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "Вс"), @"The actual output is: %s", buffer);
}

- (void)test_InsertStrBufferTooSmallUnicode_GivesTruncatedAtMiddleStr {
    pl_replace_meta (it, "title", "Вставить [] сюда");
    pl_replace_meta (it, "album", "Значение");
    char *bc = tf_compile("$insert(%title%,%album%,10)");
    tf_eval (&ctx, bc, buffer, 27);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "Вставить [Знач"), @"The actual output is: %s", buffer);
}

- (void)test_InsertStrBufferTooSmallUnicode_GivesTruncatedAtAfterStr {
    pl_replace_meta (it, "title", "Вставить [] сюда");
    pl_replace_meta (it, "album", "Значение");
    char *bc = tf_compile("$insert(%title%,%album%,10)");
    tf_eval (&ctx, bc, buffer, 41);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "Вставить [Значение] сю"), @"The actual output is: %s", buffer);
}

- (void)test_InsertStrBegin_GivesPrependedStr {
    pl_replace_meta (it, "title", ":Insert Before");
    pl_replace_meta (it, "album", "Value");
    char *bc = tf_compile("$insert(%title%,%album%,0)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "Value:Insert Before"), @"The actual output is: %s", buffer);
}

- (void)test_LeftOfUnicodeString_Takes2Chars {
    char *bc = tf_compile("$left(АБВГД,2)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "АБ"), @"The actual output is: %s", buffer);
}

- (void)test_Left2OfUnicodeStringBufFor1Char_Takes1Char {
    char *bc = tf_compile("$left(АБВГД,2)");
    tf_eval (&ctx, bc, buffer, 3);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "А"), @"The actual output is: %s", buffer);
}

- (void)test_LenOfUnicodeString_ReturnsLengthInChars {
    char *bc = tf_compile("$len(АБВГД)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "5"), @"The actual output is: %s", buffer);
}

- (void)test_DimTextExpression_ReturnsPlainText {
    char *bc = tf_compile("<<<dim this text>>>");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!ctx.dimmed);
    XCTAssert(!strcmp (buffer, "dim this text"), @"The actual output is: %s", buffer);    ctx.flags &= ~DDB_TF_CONTEXT_TEXT_DIM;
}

- (void)test_DimTextExpression_ReturnsTextWithDimEscSequence {
    char *bc = tf_compile("<<<dim this text>>>");
    ctx.flags |= DDB_TF_CONTEXT_TEXT_DIM;
    tf_eval (&ctx, bc, buffer, 1000);
    ctx.flags &= ~DDB_TF_CONTEXT_TEXT_DIM;
    tf_free (bc);
    XCTAssert(ctx.dimmed);
    XCTAssert(!strcmp (buffer, "\0331;-3mdim this text\0331;3m"), @"The actual output is: %s", buffer);
}

- (void)test_BrightenTextExpression_ReturnsTextWithBrightenEscSequence {
    char *bc = tf_compile(">>>brighten this text<<<");
    ctx.flags |= DDB_TF_CONTEXT_TEXT_DIM;
    tf_eval (&ctx, bc, buffer, 1000);
    ctx.flags &= ~DDB_TF_CONTEXT_TEXT_DIM;
    tf_free (bc);
    XCTAssert(ctx.dimmed);
    XCTAssert(!strcmp (buffer, "\0331;3mbrighten this text\0331;-3m"), @"The actual output is: %s", buffer);
}

- (void)test_BrightenInfiniteLengthTextExpression_ReturnsTextWithBrightenEscSequence {
    plt_set_item_duration(NULL, it, -1);
    char *bc = tf_compile("xxx>>>aaa%length%bbb<<<yyy");
    ctx.flags |= DDB_TF_CONTEXT_TEXT_DIM;
    tf_eval (&ctx, bc, buffer, 1000);
    ctx.flags &= ~DDB_TF_CONTEXT_TEXT_DIM;
    tf_free (bc);
    XCTAssert(ctx.dimmed);
    XCTAssert(!strcmp (buffer, "xxx\0331;3maaabbb\0331;-3myyy"), @"The actual output is: %s", buffer);
}

- (void)test_0_7_2_ContextSizeCheck_ReturnsResult {
    char *bc = tf_compile("test");
    ctx._size = (int)((char *)&ctx.dimmed - (char *)&ctx);
    tf_eval (&ctx, bc, buffer, 1000);
    ctx._size = sizeof (ctx);
    XCTAssert(!strcmp (buffer, "test"), @"The actual output is: %s", buffer);
}

- (void)test_InvalidContextSizeCheck_ReturnsEmpty {
    char *bc = tf_compile("test");
    ctx._size = (int)((char *)&ctx.dimmed - (char *)&ctx - 1);
    tf_eval (&ctx, bc, buffer, 1000);
    ctx._size = sizeof (ctx);
    XCTAssert(!strcmp (buffer, ""), @"The actual output is: %s", buffer);
}

- (void)test_PadHelloWith5_GivesHello {
    char *bc = tf_compile("$pad(Hello,5)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "Hello"), @"The actual output is: %s", buffer);
}

- (void)test_PadHelloWith5Xs_GivesHello {
    char *bc = tf_compile("$pad(Hello,5,X)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "Hello"), @"The actual output is: %s", buffer);
}

- (void)test_PadHelloWith10_Gives_Hello_____ {
    char *bc = tf_compile("$pad(Hello,10)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "Hello     "), @"The actual output is: %s", buffer);
}

- (void)test_PadHelloWith10Xs_GivesHelloXXXXX {
    char *bc = tf_compile("$pad(Hello,10,X)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "HelloXXXXX"), @"The actual output is: %s", buffer);
}

- (void)test_PadHelloWith10XYs_GivesHelloXXXXX {
    char *bc = tf_compile("$pad(Hello,10,XY)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "HelloXXXXX"), @"The actual output is: %s", buffer);
}

- (void)test_PadUnicodeStringWith10UnicodeChars_GivesExpectedOutput {
    char *bc = tf_compile("$pad(АБВГД,10,Ё)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "АБВГДЁЁЁЁЁ"), @"The actual output is: %s", buffer);
}

- (void)test_PadRightHelloWith5_GivesHello {
    char *bc = tf_compile("$pad_right(Hello,5)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "Hello"), @"The actual output is: %s", buffer);
}

- (void)test_PadRightHelloWith5Xs_GivesHello {
    char *bc = tf_compile("$pad_right(Hello,5,X)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "Hello"), @"The actual output is: %s", buffer);
}

- (void)test_PadRightHelloWith10_Gives______Hello {
    char *bc = tf_compile("$pad_right(Hello,10)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "     Hello"), @"The actual output is: %s", buffer);
}

- (void)test_PadRightHelloWith10Xs_GivesXXXXXHello {
    char *bc = tf_compile("$pad_right(Hello,10,X)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "XXXXXHello"), @"The actual output is: %s", buffer);
}

- (void)test_PadRightHelloWith10XYs_GivesXXXXXHello {
    char *bc = tf_compile("$pad_right(Hello,10,XY)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "XXXXXHello"), @"The actual output is: %s", buffer);
}

- (void)test_PadRightUnicodeStringWith10UnicodeChars_GivesExpectedOutput {
    char *bc = tf_compile("$pad_right(АБВГД,10,Ё)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "ЁЁЁЁЁАБВГД"), @"The actual output is: %s", buffer);
}

- (void)test_StripPrefix_NoArgs_Fail {
    char *bc = tf_compile("$stripprefix()");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, ""), @"The actual output is: %s", buffer);
}

- (void)test_StripPrefix_NoArticle_PassThrough {
    char *bc = tf_compile("$stripprefix(Hello)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "Hello"), @"The actual output is: %s", buffer);
}

- (void)test_StripPrefix_JoinedA_PassThrough {
    char *bc = tf_compile("$stripprefix(AA)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "AA"), @"The actual output is: %s", buffer);
}

- (void)test_StripPrefix_JoinedThe_PassThrough {
    char *bc = tf_compile("$stripprefix(TheThe)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "TheThe"), @"The actual output is: %s", buffer);
}

- (void)test_StripPrefix_MultipleA_OneStripped {
    char *bc = tf_compile("$stripprefix(A A)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "A"), @"The actual output is: %s", buffer);
}

- (void)test_StripPrefix_MultipleThe_OneStripped {
    char *bc = tf_compile("$stripprefix(The The)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "The"), @"The actual output is: %s", buffer);
}

- (void)test_StripPrefix_MultipleSpaces_AllSpacesSkipped {
    char *bc = tf_compile("$stripprefix(A  Word)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, " Word"), @"The actual output is: %s", buffer);
}

- (void)test_StripPrefix_CustomPrefixList_PrefixStripped {
    char *bc = tf_compile("$stripprefix(Some Word,The,A,Some,Word)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "Word"), @"The actual output is: %s", buffer);
}

- (void)test_SwapPrefix_NoArticle_PassThrough {
    char *bc = tf_compile("$swapprefix(Hello)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "Hello"), @"The actual output is: %s", buffer);
}

- (void)test_SwapPrefix_JoinedA_PassThrough {
    char *bc = tf_compile("$swapprefix(AA)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "AA"), @"The actual output is: %s", buffer);
}

- (void)test_SwapPrefix_JoinedThe_PassThrough {
    char *bc = tf_compile("$swapprefix(TheThe)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "TheThe"), @"The actual output is: %s", buffer);
}

- (void)test_SwapPrefix_MultipleA_CommaAdded {
    char *bc = tf_compile("$swapprefix(A A)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "A, A"), @"The actual output is: %s", buffer);
}

- (void)test_SwapPrefix_MultipleThe_CommaAdded {
    char *bc = tf_compile("$swapprefix(The The)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "The, The"), @"The actual output is: %s", buffer);
}

- (void)test_SwapPrefix_MultipleSpaces_SpacePreservedWithComma {
    char *bc = tf_compile("$swapprefix(A  Word)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, " Word, A"), @"The actual output is: %s", buffer);
}

- (void)test_SwapPrefix_CustomPrefixList_PrefixSwapped {
    char *bc = tf_compile("$swapprefix(Some Word,The,A,Some,Word)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "Word, Some"), @"The actual output is: %s", buffer);
}

- (void)test_StricmpEqual_ReturnsYes {
    char *bc = tf_compile("$if($stricmp(AbCd,AbCd),YES)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "YES"), @"The actual output is: %s", buffer);
}

- (void)test_StricmpUnequal_ReturnsEmpty {
    char *bc = tf_compile("$if($stricmp(AbCd,EfGh),YES)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, ""), @"The actual output is: %s", buffer);
}

- (void)test_StricmpEqualWithDifferentCase_ReturnsYES {
    char *bc = tf_compile("$if($stricmp(ABCD,abcd),YES)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "YES"), @"The actual output is: %s", buffer);
}

- (void)test_Len2AsciiChars_ReturnsNumberOfChars {
    char *bc = tf_compile("$len2(ABCDE)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "5"), @"The actual output is: %s", buffer);
}

- (void)test_Len2UnicodeSingleWidthChars_ReturnsNumberOfChars {
    char *bc = tf_compile("$len2(АБВГД)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "5"), @"The actual output is: %s", buffer);
}

- (void)test_Len2UnicodeDoubleWidthChars_ReturnsNumberOfCharsDoubled {
    char *bc = tf_compile("$len2(全形)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "4"), @"The actual output is: %s", buffer);
}

- (void)test_ShortestFirst_ReturnsFirst {
    char *bc = tf_compile("$shortest(1,22,333)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "1"), @"The actual output is: %s", buffer);
}

- (void)test_ShortestLast_ReturnsLast {
    char *bc = tf_compile("$shortest(333,22,1)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "1"), @"The actual output is: %s", buffer);
}

- (void)test_ShortestMid_ReturnsMid {
    char *bc = tf_compile("$shortest(333,1,22)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "1"), @"The actual output is: %s", buffer);
}

- (void)test_LongestFirst_ReturnsFirst {
    char *bc = tf_compile("$longest(333,22,1)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "333"), @"The actual output is: %s", buffer);
}

- (void)test_LongestLast_ReturnsLast {
    char *bc = tf_compile("$longest(1,22,333)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "333"), @"The actual output is: %s", buffer);
}

- (void)test_LongestMid_ReturnsMid {
    char *bc = tf_compile("$longest(1,333,22)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "333"), @"The actual output is: %s", buffer);
}

- (void)test_LongerFirst_ReturnsFirst {
    char *bc = tf_compile("$longer(22,1)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "22"), @"The actual output is: %s", buffer);
}

- (void)test_LongerSecond_ReturnsSecond {
    char *bc = tf_compile("$longer(1,22)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "22"), @"The actual output is: %s", buffer);
}

- (void)test_PadcutStrLonger_ReturnsHeadOfStr {
    char *bc = tf_compile("$padcut(Hello,3)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "Hel"), @"The actual output is: %s", buffer);
}


- (void)test_PadcutStrShorter_ReturnsPaddedStr {
    char *bc = tf_compile("$padcut(Hello,8)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "Hello   "), @"The actual output is: %s", buffer);
}


- (void)test_PadcutStrCharLonger_ReturnsHeadOfStr {
    char *bc = tf_compile("$padcut(Hello,3,x)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "Hel"), @"The actual output is: %s", buffer);
}

- (void)test_PadcutStrCharShorter_ReturnsPaddedStr {
    char *bc = tf_compile("$padcut(Hello,8,x)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "Helloxxx"), @"The actual output is: %s", buffer);
}

- (void)test_PadcutRightStrLonger_ReturnsHeadOfStr {
    char *bc = tf_compile("$padcut_right(Hello,3)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "Hel"), @"The actual output is: %s", buffer);
}


- (void)test_PadcutRightStrShorter_ReturnsPaddedStr {
    char *bc = tf_compile("$padcut_right(Hello,8)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "   Hello"), @"The actual output is: %s", buffer);
}


- (void)test_PadcutRightStrCharLonger_ReturnsHeadOfStr {
    char *bc = tf_compile("$padcut_right(Hello,3,x)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "Hel"), @"The actual output is: %s", buffer);
}

- (void)test_PadcutRightStrCharShorter_ReturnsPaddedStr {
    char *bc = tf_compile("$padcut_right(Hello,8,x)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "xxxHello"), @"The actual output is: %s", buffer);
}

- (void)test_ProgressPos0Range100Len10_ReturnsBar10CharsWithKnobAt0 {
    char *bc = tf_compile("$progress(0,100,10,x,=)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "x========="), @"The actual output is: %s", buffer);
}

- (void)test_ProgressPos100Range100Len10_ReturnsBar10CharsWithKnobAt9 {
    char *bc = tf_compile("$progress(100,100,10,x,=)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "=========x"), @"The actual output is: %s", buffer);
}

- (void)test_ProgressPos50Range100Len10_ReturnsBar10CharsWithKnobAt5 {
    char *bc = tf_compile("$progress(50,100,10,x,=)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "=====x===="), @"The actual output is: %s", buffer);
}

- (void)test_ProgressRange0_ReturnsEmpty {
    char *bc = tf_compile("$progress(5,0,10,x,=)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "=========x"), @"The actual output is: %s", buffer);
}

- (void)test_ProgressLen0_ReturnsEmpty {
    char *bc = tf_compile("$progress(5,100,0,x,=)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, ""), @"The actual output is: %s", buffer);
}

- (void)test_ProgressCharEmpty_ReturnsEmpty {
    char *bc = tf_compile("$progress(5,100,0,x,)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, ""), @"The actual output is: %s", buffer);
}

- (void)test_ProgressKnobEmpty_ReturnsEmpty {
    char *bc = tf_compile("$progress(5,100,0,,=)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, ""), @"The actual output is: %s", buffer);
}

- (void)test_ProgressRangeEmpty_ReturnsEmpty {
    char *bc = tf_compile("$progress(5,,0,x,=)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, ""), @"The actual output is: %s", buffer);
}

- (void)test_ProgressAllArgsEmpty_ReturnsEmpty {
    char *bc = tf_compile("$progress(,,,,)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, ""), @"The actual output is: %s", buffer);
}

- (void)test_Progress2Pos0Range100Len10_ReturnsBar10CharsWithKnobAt0 {
    char *bc = tf_compile("$progress2(0,100,10,x,=)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "=========="), @"The actual output is: %s", buffer);
}

- (void)test_Progress2Pos100Range100Len10_ReturnsBar10CharsWithKnobAt9 {
    char *bc = tf_compile("$progress2(100,100,10,x,=)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "xxxxxxxxxx"), @"The actual output is: %s", buffer);
}

- (void)test_Progress2Pos50Range100Len10_ReturnsBar10CharsWithKnobAt5 {
    char *bc = tf_compile("$progress2(50,100,10,x,=)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "xxxxx====="), @"The actual output is: %s", buffer);
}

- (void)test_Progress2Range0_ReturnsEmpty {
    char *bc = tf_compile("$progress2(5,0,10,x,=)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "xxxxxxxxxx"), @"The actual output is: %s", buffer);
}

- (void)test_Progress2Len0_ReturnsEmpty {
    char *bc = tf_compile("$progress2(5,100,0,x,=)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, ""), @"The actual output is: %s", buffer);
}

- (void)test_Progress2CharEmpty_ReturnsEmpty {
    char *bc = tf_compile("$progress2(5,100,0,x,)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, ""), @"The actual output is: %s", buffer);
}

- (void)test_Progress2KnobEmpty_ReturnsEmpty {
    char *bc = tf_compile("$progress2(5,100,0,,=)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, ""), @"The actual output is: %s", buffer);
}

- (void)test_Progress2RangeEmpty_ReturnsEmpty {
    char *bc = tf_compile("$progress2(5,,0,x,=)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, ""), @"The actual output is: %s", buffer);
}

- (void)test_Progress2AllArgsEmpty_ReturnsEmpty {
    char *bc = tf_compile("$progress2(,,,,)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, ""), @"The actual output is: %s", buffer);
}

- (void)test_Right5Chars_ReturnsLast5Chars {
    char *bc = tf_compile("$right(ABCDE12345,5)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "12345"), @"The actual output is: %s", buffer);
}

- (void)test_Right5CharsShortStr_ReturnsWholeStr {
    char *bc = tf_compile("$right(ABC,5)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "ABC"), @"The actual output is: %s", buffer);
}

- (void)test_Roman1_ReturnsI {
    char *bc = tf_compile("$roman(1)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "I"), @"The actual output is: %s", buffer);
}

- (void)test_Roman5_ReturnsV {
    char *bc = tf_compile("$roman(5)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "V"), @"The actual output is: %s", buffer);
}

- (void)test_Roman10_ReturnsX {
    char *bc = tf_compile("$roman(10)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "X"), @"The actual output is: %s", buffer);
}


- (void)test_Roman100500_ReturnsEmpty {
    char *bc = tf_compile("$roman(100500)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, ""), @"The actual output is: %s", buffer);
}

- (void)test_Roman51880_Returns_MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMDCCCLXXX {
    char *bc = tf_compile("$roman(51880)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMDCCCLXXX"), @"The actual output is: %s", buffer);
}

- (void)test_Rot13DeaDBeeF12345_ReturnsQrnQOrrS12345 {
    char *bc = tf_compile("$rot13(DeaDBeeF12345)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "QrnQOrrS12345"), @"The actual output is: %s", buffer);
}

- (void)test_StrchrDeaDBeeF_B_Returns5 {
    char *bc = tf_compile("$strchr(DeaDBeeF,B)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "5"), @"The actual output is: %s", buffer);
}

- (void)test_StrchrDeaDBeeF_R_Returns0 {
    char *bc = tf_compile("$strchr(DeaDBeeF,R)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "0"), @"The actual output is: %s", buffer);
}

- (void)test_StrrchrDeaDBeeF_B_Returns4 {
    char *bc = tf_compile("$strrchr(DeaDBeeF,D)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "4"), @"The actual output is: %s", buffer);
}

- (void)test_StrrchrDeaDBeeF_R_Returns0 {
    char *bc = tf_compile("$strrchr(DeaDBeeF,R)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "0"), @"The actual output is: %s", buffer);
}

- (void)test_NestingDirectoryInSubstr_HasNoIntermediateTruncation {
    pl_replace_meta (it, ":URI", "/media/Icy/Music/Long/Folder/Structure/2019.01.01 [Meta1] Meta2 [Meta3] Meta4 [Meta5] Meta6 [Meta7] Meta8 [Meta9]/some_reasonably_long_path.flac");
    char *bc = tf_compile("$substr($directory(%path%,1),1,$sub($strstr($directory(%path%,1),' ['),1))");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free(bc);
    XCTAssert(!strcmp (buffer, "2019.01.01"), @"The actual output is: %s", buffer);
}

- (void)test_Tab_ProducesTabChar {
    ctx.flags = DDB_TF_CONTEXT_MULTILINE;
    char *bc = tf_compile("$tab()");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "\t"), @"The actual output is: %s", buffer);
}

- (void)test_Tab5_Produces5TabChars {
    ctx.flags = DDB_TF_CONTEXT_MULTILINE;
    char *bc = tf_compile("$tab(5)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "\t\t\t\t\t"), @"The actual output is: %s", buffer);
}

- (void)test_TrimNoLeadingTrailingSpaces_ReturnsOriginal {
    char *bc = tf_compile("$trim(hello)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "hello"), @"The actual output is: %s", buffer);
}

- (void)test_TrimLeadingSpaces_ReturnsTrimmedString {
    char *bc = tf_compile("$trim(   hello)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "hello"), @"The actual output is: %s", buffer);
}

- (void)test_TrimTrailingSpaces_ReturnsTrimmedString {
    char *bc = tf_compile("$trim(hello   )");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "hello"), @"The actual output is: %s", buffer);
}

- (void)test_TrimLeadingAndTrailingSpaces_ReturnsTrimmedString {
    char *bc = tf_compile("$trim(    hello   )");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "hello"), @"The actual output is: %s", buffer);
}

- (void)test_TrimLeadingAndTrailingSpacesWithTabs_ReturnsTrimmedToTabsString {
    ctx.flags = DDB_TF_CONTEXT_MULTILINE;
    char *bc = tf_compile("$trim( \t   hello  \t )");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    XCTAssert(!strcmp (buffer, "\t   hello  \t"), @"The actual output is: %s", buffer);
}

@end
