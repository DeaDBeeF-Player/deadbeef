/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2018 Oleksiy Yakovenko and other contributors

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

#include "messagepump.h"
#include "plmeta.h"
#include "playqueue.h"
#include "plugins.h"
#include "streamer.h"
#include "tf.h"
#include "tftintutil.h"
#include <dispatch/dispatch.h>
#include <gtest/gtest.h>

static ddb_playback_state_t fake_out_state_value = DDB_PLAYBACK_STATE_STOPPED;

static ddb_playback_state_t fake_out_state (void) {
    return fake_out_state_value;
}

static DB_output_t fake_out = {
    .plugin.id = "fake_out",
    .plugin.name = "fake_out",
    .state = fake_out_state,
};

class TitleFormattingTests: public ::testing::Test {
protected:
    void SetUp() override {
        it = pl_item_alloc_init ("testfile.flac", "stdflac");

        memset (&ctx, 0, sizeof (ctx));
        ctx._size = sizeof (ddb_tf_context_t);
        ctx.it = (DB_playItem_t *)it;
        ctx.plt = NULL;

        messagepump_init();
        plug_set_output (&fake_out);
        streamer_init();

        streamer_set_playing_track (NULL);

        fake_out_state_value = DDB_PLAYBACK_STATE_STOPPED;
    }

    void TearDown() override {
        streamer_set_playing_track (NULL);
        pl_item_unref (it);
        ctx.it = NULL;
        ctx.plt = NULL;
        streamer_free();

        // flush any remaining events
        uint32_t _id;
        uintptr_t ctx;
        uint32_t p1;
        uint32_t p2;
        while (messagepump_pop(&_id, &ctx, &p1, &p2) != -1) {
            if (_id >= DB_EV_FIRST && ctx) {
                messagepump_event_free ((ddb_event_t *)ctx);
            }
        }

        messagepump_free();
    }

    playItem_t *it;
    ddb_tf_context_t ctx;
    char buffer[1000];
};

TEST_F(TitleFormattingTests, test_Literal_ReturnsLiteral) {
    char *bc = tf_compile("hello world");
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    EXPECT_TRUE(!strcmp ("hello world", buffer));
}

TEST_F(TitleFormattingTests, test_AlbumArtistDash4DigitYearSpaceAlbum_ReturnsCorrectResult) {
    pl_add_meta (it, "album artist", "TheAlbumArtist");
    pl_add_meta (it, "year", "12345678");
    pl_add_meta (it, "album", "TheNameOfAlbum");

    char *bc = tf_compile("%album artist% - ($left($meta(year),4)) %album%");
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    EXPECT_TRUE(!strcmp ("TheAlbumArtist - (1234) TheNameOfAlbum", buffer));
}

TEST_F(TitleFormattingTests, test_Unicode_AlbumArtistDash4DigitYearSpaceAlbum_ReturnsCorrectResult) {
    pl_add_meta (it, "album artist", "ИсполнительДанногоАльбома");
    pl_add_meta (it, "year", "12345678");
    pl_add_meta (it, "album", "Альбом");

    char *bc = tf_compile("%album artist% - ($left($meta(year),4)) %album%");
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    EXPECT_TRUE(!strcmp ("ИсполнительДанногоАльбома - (1234) Альбом", buffer));
}

TEST_F(TitleFormattingTests, test_TotalDiscsGreaterThan1_ReturnsExpectedResult) {
    pl_add_meta (it, "numdiscs", "20");
    pl_add_meta (it, "disc", "18");

    char *bc = tf_compile("$if($greater(%totaldiscs%,1),- Disc: %discnumber%/%totaldiscs%)");
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    EXPECT_TRUE(!strcmp ("- Disc: 18/20", buffer));
}

TEST_F(TitleFormattingTests, test_AlbumArtistSameAsArtist_ReturnsBlankTrackArtist) {
    pl_add_meta (it, "artist", "Artist Name");
    pl_add_meta (it, "album artist", "Artist Name");

    char *bc = tf_compile("%track artist%");
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    EXPECT_TRUE(!buffer[0]);
}

TEST_F(TitleFormattingTests, test_TrackArtistIsUndef_ReturnsBlankTrackArtist) {
    pl_add_meta (it, "album artist", "Artist Name");

    char *bc = tf_compile("%track artist%");
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    EXPECT_TRUE(!buffer[0]);
}

TEST_F(TitleFormattingTests, test_TrackArtistIsDefined_ReturnsTheTrackArtist) {
    pl_add_meta (it, "artist", "Track Artist Name");
    pl_add_meta (it, "album artist", "Album Artist Name");

    char *bc = tf_compile("%track artist%");
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    EXPECT_TRUE(!strcmp ("Track Artist Name", buffer));
}

TEST_F(TitleFormattingTests, test_Add10And2_Gives12) {
    char *bc = tf_compile("$add(10,2)");
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    EXPECT_TRUE(!strcmp ("12", buffer));
}

TEST_F(TitleFormattingTests, test_StrcmpChannelsMono_GivesMo) {
    char *bc = tf_compile("$if($strcmp(%channels%,mono),mo,st)");
    pl_replace_meta (it, ":CHANNELS", "1");
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    EXPECT_TRUE(!strcmp ("mo", buffer));
}

TEST_F(TitleFormattingTests, test_StrcmpChannelsMono_GivesSt) {
    char *bc = tf_compile("$if($strcmp(%channels%,mono),mo,st)");
    pl_replace_meta (it, ":CHANNELS", "2");
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    EXPECT_TRUE(!strcmp ("st", buffer));
}

// This test validates that overflowing the buffer wouldn't cause a crash
TEST_F(TitleFormattingTests, test_LongCommentOverflowBuffer_ExpectedResult) {
    char longcomment[2048];
    for (int i = 0; i < sizeof (longcomment) - 1; i++) {
        longcomment[i] = (i % 33) + 'a';
    }
    longcomment[sizeof (longcomment)-1] = 0;

    pl_add_meta (it, "comment", longcomment);

    char *bc = tf_compile("$meta(comment)");
    tf_eval (&ctx, bc, buffer, 200);
    tf_free (bc);
    EXPECT_TRUE(!memcmp (buffer, "abcdef", 6));
}

TEST_F(TitleFormattingTests, test_ParticularLongExpressionDoesntAllocateZeroBytes) {
    pl_replace_meta (it, "artist", "Frank Schätzing");
    pl_replace_meta (it, "year", "1999");
    pl_replace_meta (it, "album", "Tod und Teufel");
    pl_replace_meta (it, "disc", "1");
    pl_replace_meta (it, "disctotal", "4");
    pl_replace_meta (it, ":FILETYPE", "FLAC");
    char *bc = tf_compile("$if($strcmp(%genre%,Classical),%composer%,$if([%band%],%band%,%album artist%)) | ($left(%year%,4)) %album% $if($greater(%totaldiscs%,1),- Disc: %discnumber%/%totaldiscs%) - \\[%codec%\\]");

    EXPECT_TRUE(bc != NULL);

    tf_free (bc);
}

TEST_F(TitleFormattingTests, test_If2FirstArgIsTrue_EvalToFirstArg) {
    pl_replace_meta (it, "title", "a title");
    char *bc = tf_compile("$if2(%title%,def)");
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    EXPECT_TRUE(!strcmp ("a title", buffer));
}

TEST_F(TitleFormattingTests, test_If2FirstArgIsMixedStringTrue_EvalToFirstArg) {
    pl_replace_meta (it, "title", "a title");
    pl_replace_meta (it, "artist", "an artist");
    char *bc = tf_compile("$if2(%title%%artist%,def)");
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    EXPECT_TRUE(!strcmp ("a titlean artist", buffer));
}

TEST_F(TitleFormattingTests, test_If2FirstArgIsMissingField_EvalToLastArg) {
    pl_replace_meta (it, "title", "a title");
    pl_replace_meta (it, "artist", "an artist");
    char *bc = tf_compile("$if2(%garbage%,def)");
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    EXPECT_TRUE(!strcmp ("def", buffer));
}

TEST_F(TitleFormattingTests, test_If2FirstArgIsMixedWithGarbageTrue_EvalToFirstArg) {
    pl_replace_meta (it, "title", "a title");
    pl_replace_meta (it, "artist", "an artist");
    char *bc = tf_compile("$if2(%garbage%xxx%title%,def)");
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    EXPECT_TRUE(!strcmp ("xxxa title", buffer));
}

TEST_F(TitleFormattingTests, test_If2FirstArgIsMixedWithGarbageTailTrue_EvalToFirstArg) {
    pl_replace_meta (it, "title", "a title");
    pl_replace_meta (it, "artist", "an artist");
    char *bc = tf_compile("$if2(%title%%garbage%xxx,def)");
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    EXPECT_TRUE(!strcmp ("a titlexxx", buffer));
}

TEST_F(TitleFormattingTests, test_If2FirstArgIsFalse_EvalToSecondArg) {
    char *bc = tf_compile("$if2(,ghi)");
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    EXPECT_TRUE(!strcmp ("ghi", buffer));
}

TEST_F(TitleFormattingTests, test_If3FirstArgIsTrue_EvalToFirstArg) {
    pl_replace_meta (it, "title", "a title");
    char *bc = tf_compile("$if3(%title%,def)");
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    EXPECT_TRUE(!strcmp ("a title", buffer));
}

TEST_F(TitleFormattingTests, test_If3AllButLastAreFalse_EvalToLastArg) {
    char *bc = tf_compile("$if3(,,,,,lastarg)");
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    EXPECT_TRUE(!strcmp ("lastarg", buffer));
}

TEST_F(TitleFormattingTests, test_If3OneOfTheArgsBeforeLastIsTrue_EvalToFirstTrueArg) {
    char *bc = tf_compile("$if3(,,firstarg,,secondarg,lastarg)");
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    EXPECT_TRUE(!strcmp ("lastarg", buffer));
}

TEST_F(TitleFormattingTests, test_IfEqualTrue_EvalsToThen) {
    char *bc = tf_compile("$ifequal(100,100,then,else)");
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    EXPECT_TRUE(!strcmp ("then", buffer));
}

TEST_F(TitleFormattingTests, test_IfEqualFalse_EvalsToElse) {
    char *bc = tf_compile("$ifequal(100,200,then,else)");
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    EXPECT_TRUE(!strcmp ("else", buffer));
}

TEST_F(TitleFormattingTests, test_IfGreaterTrue_EvalsToThen) {
    char *bc = tf_compile("$ifgreater(200,100,then,else)");
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    EXPECT_TRUE(!strcmp ("then", buffer));
}

TEST_F(TitleFormattingTests, test_IfGreaterFalse_EvalsToElse) {
    char *bc = tf_compile("$ifgreater(100,200,then,else)");
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    EXPECT_TRUE(!strcmp ("else", buffer));
}

TEST_F(TitleFormattingTests, test_GreaterIsTrue_EvalsToTrue) {
    char *bc = tf_compile("$if($greater(2,1),istrue,isfalse)");
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    EXPECT_TRUE(!strcmp ("istrue", buffer));
}

TEST_F(TitleFormattingTests, test_GreaterIsFalse_EvalsToFalse) {
    char *bc = tf_compile("$if($greater(1,2),istrue,isfalse)");
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    EXPECT_TRUE(!strcmp ("isfalse", buffer));
}

TEST_F(TitleFormattingTests, test_GreaterIsFalseEmptyArguments_EvalsToFalse) {
    char *bc = tf_compile("$if($greater(,),istrue,isfalse)");
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    EXPECT_TRUE(!strcmp ("isfalse", buffer));
}

TEST_F(TitleFormattingTests, test_Greater_CalculatedValueTrue_EvalsToTrue) {
    pl_replace_meta (it, "album", "abcdefabcdefabcdefabcdef");
    char *bc = tf_compile("$if($greater($len(%album%),20),GREATER $len(%album%) %album%,NOT GREATER $len(%album%) %album%)");
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    EXPECT_TRUE(!strcmp ("GREATER 24 abcdefabcdefabcdefabcdef", buffer));
}

TEST_F(TitleFormattingTests, test_Greater_CalculatedValueFalse_EvalsToFalse) {
    pl_replace_meta (it, "album", "abcdef");
    char *bc = tf_compile("$if($greater($len(%album%),20),GREATER $len(%album%) %album%,NOT GREATER $len(%album%) %album%)");
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    EXPECT_TRUE(!strcmp ("NOT GREATER 6 abcdef", buffer));
}

TEST_F(TitleFormattingTests, test_Greater_Largenumber_EvalsToTrue) {
    char *bc = tf_compile("$if($greater(50,20),GREATER,NOT GREATER)");
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    EXPECT_TRUE(!strcmp ("GREATER", buffer));
}

TEST_F(TitleFormattingTests, test_StrCmpEmptyArguments_EvalsToTrue) {
    char *bc = tf_compile("$if($strcmp(,),istrue,isfalse)");
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    EXPECT_TRUE(!strcmp ("istrue", buffer));
}

TEST_F(TitleFormattingTests, test_StrCmpSameArguments_EvalsToTrue) {
    char *bc = tf_compile("$if($strcmp(abc,abc),istrue,isfalse)");
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    EXPECT_TRUE(!strcmp ("istrue", buffer));
}

TEST_F(TitleFormattingTests, test_IfLongerTrue_EvalsToTrue) {
    char *bc = tf_compile("$iflonger(abcd,2,istrue,isfalse)");
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    EXPECT_TRUE(!strcmp ("istrue", buffer));
}

TEST_F(TitleFormattingTests, test_IfLongerFalse_EvalsToFalse) {
    char *bc = tf_compile("$iflonger(ab,4,istrue,isfalse)");
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    EXPECT_TRUE(!strcmp ("isfalse", buffer));
}

TEST_F(TitleFormattingTests, test_SelectMiddle_EvalsToSelectedValue) {
    char *bc = tf_compile("$select(3,10,20,30,40,50)");
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    EXPECT_TRUE(!strcmp ("30", buffer));
}

TEST_F(TitleFormattingTests, test_SelectLeftmost_EvalsToSelectedValue) {
    char *bc = tf_compile("$select(1,10,20,30,40,50)");
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    EXPECT_TRUE(!strcmp ("10", buffer));
}

TEST_F(TitleFormattingTests, test_SelectRightmost_EvalsToSelectedValue) {
    char *bc = tf_compile("$select(5,10,20,30,40,50)");
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    EXPECT_TRUE(!strcmp ("50", buffer));
}

TEST_F(TitleFormattingTests, test_SelectOutOfBoundsLeft_EvalsToFalse) {
    char *bc = tf_compile("$select(0,10,20,30,40,50)");
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    EXPECT_TRUE(!buffer[0]);
}

TEST_F(TitleFormattingTests, test_SelectOutOfBoundsRight_EvalsToFalse) {
    char *bc = tf_compile("$select(6,10,20,30,40,50)");
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);
    EXPECT_TRUE(!buffer[0]);
}

TEST_F(TitleFormattingTests, test_InvalidPercentExpression_WithNullTrack_NoCrash) {
    char *bc = tf_compile("begin - %version% - end");

    ctx.it = NULL;
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp ("begin -  - end", buffer));
}

TEST_F(TitleFormattingTests, test_InvalidPercentExpression_WithNullTrackAccessingMetadata_NoCrash) {
    char *bc = tf_compile("begin - %title% - end");
    ctx.it = NULL;
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp ("begin -  - end", buffer));
}

TEST_F(TitleFormattingTests, test_Index_WithNullPlaylist_NoCrash) {
    char *bc = tf_compile("begin - %list_index% - end");
    ctx.it = NULL;
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp ("begin - 0 - end", buffer));
}

TEST_F(TitleFormattingTests, test_Div5by2_Gives3) {
    char *bc = tf_compile("$div(5,2)");
    ctx.it = NULL;
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp ("3", buffer));
}

TEST_F(TitleFormattingTests, test_Div4pt9by1pt9_Gives4) {
    char *bc = tf_compile("$div(4.9,1.9)");
    ctx.it = NULL;
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp ("4", buffer));
}

TEST_F(TitleFormattingTests, test_Div20by2by5_Gives2) {
    char *bc = tf_compile("$div(20,2,5)");
    ctx.it = NULL;
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp ("2", buffer));
}

TEST_F(TitleFormattingTests, test_DivBy0_GivesEmpty) {
    char *bc = tf_compile("$div(5,0)");
    ctx.it = NULL;
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!buffer[0]);
}

TEST_F(TitleFormattingTests, test_Max0Arguments_GivesEmpty) {
    char *bc = tf_compile("$max()");
    ctx.it = NULL;
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!buffer[0]);
}

TEST_F(TitleFormattingTests, test_MaxOf1and2_Gives2) {
    char *bc = tf_compile("$max(1,2)");
    ctx.it = NULL;
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp ("2", buffer));
}

TEST_F(TitleFormattingTests, test_MaxOf30and50and20_Gives50) {
    char *bc = tf_compile("$max(30,50,20)");
    ctx.it = NULL;
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp ("50", buffer));
}

TEST_F(TitleFormattingTests, test_MinOf1and2_Gives1) {
    char *bc = tf_compile("$min(1,2)");
    ctx.it = NULL;
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp ("1", buffer));
}

TEST_F(TitleFormattingTests, test_MinOf30and50and20_Gives20) {
    char *bc = tf_compile("$min(30,50,20)");
    ctx.it = NULL;
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp ("20", buffer));
}

TEST_F(TitleFormattingTests, test_ModOf3and2_Gives1) {
    char *bc = tf_compile("$mod(3,2)");
    ctx.it = NULL;
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp ("1", buffer));
}

TEST_F(TitleFormattingTests, test_ModOf6and3_Gives0) {
    char *bc = tf_compile("$mod(6,3)");
    ctx.it = NULL;
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp ("0", buffer));
}

TEST_F(TitleFormattingTests, test_ModOf16and18and9_Gives7) {
    char *bc = tf_compile("$mod(16,18,9)");
    ctx.it = NULL;
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp ("7", buffer));
}

TEST_F(TitleFormattingTests, test_Mul2and5_Gives10) {
    char *bc = tf_compile("$mul(2,5)");
    ctx.it = NULL;
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp ("10", buffer));
}

TEST_F(TitleFormattingTests, test_MulOf2and3and4_Gives24) {
    char *bc = tf_compile("$mul(2,3,4)");
    ctx.it = NULL;
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp ("24", buffer));
}

TEST_F(TitleFormattingTests, test_MulDiv2and10and4_Gives5) {
    char *bc = tf_compile("$muldiv(2,10,4)");
    ctx.it = NULL;
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp ("5", buffer));
}

TEST_F(TitleFormattingTests, test_MulDiv2and10and0_GivesEmpty) {
    char *bc = tf_compile("$muldiv(2,10,0)");
    ctx.it = NULL;
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!buffer[0]);
}

TEST_F(TitleFormattingTests, test_MulDiv2and3and4_Gives2) {
    char *bc = tf_compile("$muldiv(2,3,4)");
    ctx.it = NULL;
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp ("2", buffer));
}

TEST_F(TitleFormattingTests, test_Rand_GivesANumber) {
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
    EXPECT_TRUE(num_digits == strlen (buffer));
}

TEST_F(TitleFormattingTests, test_RandWithArgs_GivesEmpty) {
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
    EXPECT_TRUE(!strcmp (buffer, ""));
}

TEST_F(TitleFormattingTests, test_SubWithoutArgs_GivesEmpty) {
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
    EXPECT_TRUE(!strcmp (buffer, ""));
}

TEST_F(TitleFormattingTests, test_SubWith1Arg_GivesEmpty) {
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
    EXPECT_TRUE(!strcmp (buffer, ""));
}

TEST_F(TitleFormattingTests, test_SubWith3and2_Gives1) {
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
    EXPECT_TRUE(!strcmp (buffer, "1"));
}

TEST_F(TitleFormattingTests, test_SubWith10and5and2_Gives3) {
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
    EXPECT_TRUE(!strcmp (buffer, "3"));
}

TEST_F(TitleFormattingTests, test_ChannelsFor3ChTrack_Gives3) {
    pl_replace_meta (it, ":CHANNELS", "3");
    char *bc = tf_compile("%channels%");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "3"));
}

TEST_F(TitleFormattingTests, test_ChannelsFuncForMonoTrack_GivesMono) {
    pl_replace_meta (it, ":CHANNELS", "1");
    char *bc = tf_compile("$channels()");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "mono"));
}

TEST_F(TitleFormattingTests, test_ChannelsFuncForUnsetChannels_GivesStereo) {
    char *bc = tf_compile("$channels()");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "stereo"));
}

TEST_F(TitleFormattingTests, test_AndTrueArgsWithElse_ReturnsTrue) {
    pl_replace_meta (it, "artist", "artist");
    pl_replace_meta (it, "album", "album");

    char *bc = tf_compile("$if($and(%artist%,%album%),true,false)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "true"));
}

TEST_F(TitleFormattingTests, test_AndTrueArgs_ReturnsTrue) {
    pl_replace_meta (it, "artist", "artist");
    pl_replace_meta (it, "album", "album");

    char *bc = tf_compile("$if($and(%artist%,%album%),true)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "true"));
}

TEST_F(TitleFormattingTests, test_AndTrueArgs_ReturnsArtistAlbum) {
    pl_replace_meta (it, "artist", "artist");
    pl_replace_meta (it, "album", "album");

    char *bc = tf_compile("$if($and(%artist%,%album%),%artist% - %album%)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "artist - album"));
}

TEST_F(TitleFormattingTests, test_AndTrueAndFalseArgs_ReturnsFalse) {
    pl_replace_meta (it, "artist", "artist");

    char *bc = tf_compile("$if($and(%artist%,%album%),true,false)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "false"));
}

TEST_F(TitleFormattingTests, test_AndFalseArgs_ReturnsFalse) {
    char *bc = tf_compile("$if($and(%artist%,%album%),true,false)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "false"));
}

TEST_F(TitleFormattingTests, test_OrTrueAndFalseArgs_ReturnsTrue) {
    pl_replace_meta (it, "artist", "artist");

    char *bc = tf_compile("$if($or(%artist%,%album%),true,false)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "true"));
}

TEST_F(TitleFormattingTests, test_OrTrueArgs_ReturnsTrue) {
    pl_replace_meta (it, "artist", "artist");
    pl_replace_meta (it, "album", "album");

    char *bc = tf_compile("$if($or(%artist%,%album%),true,false)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "true"));
}

TEST_F(TitleFormattingTests, test_OrFalseArgs_ReturnsFalse) {
    char *bc = tf_compile("$if($or(%artist%,%album%),true,false)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "false"));
}

TEST_F(TitleFormattingTests, test_NotTrueArg_ReturnsFalse) {
    pl_replace_meta (it, "artist", "artist");
    char *bc = tf_compile("$if($not(%artist%),true,false)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "false"));
}

TEST_F(TitleFormattingTests, test_NotFalseArg_ReturnsTrue) {
    char *bc = tf_compile("$if($not(%artist%),true,false)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "true"));
}

TEST_F(TitleFormattingTests, test_XorTrueAndTrue_ReturnsFalse) {
    pl_replace_meta (it, "artist", "artist");
    pl_replace_meta (it, "album", "album");
    char *bc = tf_compile("$if($xor(%artist%,%album%),true,false)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "false"));
}

TEST_F(TitleFormattingTests, test_XorTrueAndFalse_ReturnsTrue) {
    pl_replace_meta (it, "artist", "artist");
    char *bc = tf_compile("$if($xor(%artist%,%album%),true,false)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "true"));
}

TEST_F(TitleFormattingTests, test_XorFalseTrueFalse_ReturnsTrue) {
    pl_replace_meta (it, "artist", "artist");
    char *bc = tf_compile("$if($xor(%album%,%artist%,%album%),true,false)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "true"));
}

TEST_F(TitleFormattingTests, test_PlayingColumnWithEmptyFormat_GivesQueueIndexes) {
    playqueue_push (it);
    ctx.id = DB_COLUMN_PLAYING;
    ctx.flags |= DDB_TF_CONTEXT_HAS_ID;
    tf_eval (&ctx, NULL, buffer, 1000);
    EXPECT_TRUE(!strcmp (buffer, "(1)"));
    playqueue_pop ();
}

TEST_F(TitleFormattingTests, test_LengthSamplesOf100Start300End_Returns200) {
    pl_item_set_startsample (it, 100);
    pl_item_set_endsample (it, 300);
    char *bc = tf_compile("%length_samples%");
    tf_eval (&ctx, bc, buffer, 1000);
    EXPECT_TRUE(!strcmp (buffer, "200"));
}

TEST_F(TitleFormattingTests, test_AbbrTestString_ReturnsAbbreviatedString) {
    pl_item_set_startsample (it, 100);
    pl_item_set_endsample (it, 300);
    char *bc = tf_compile("$abbr('This is a Long Title (12-inch version) [needs tags]')");
    tf_eval (&ctx, bc, buffer, 1000);
    EXPECT_TRUE(!strcmp (buffer, "TiaLT1v[needst"));
}

TEST_F(TitleFormattingTests, test_AbbrTestUnicodeString_ReturnsAbbreviatedString) {
    pl_item_set_startsample (it, 100);
    pl_item_set_endsample (it, 300);
    char *bc = tf_compile("$abbr('This ɀHİJ a русский Title (12-inch version) [needs tags]')");
    tf_eval (&ctx, bc, buffer, 1000);
    EXPECT_TRUE(!strcmp (buffer, "TɀaрT1v[needst"));
}

TEST_F(TitleFormattingTests, test_AnsiTestString_ReturnsTheSameString) {
    char *bc = tf_compile("$ansi(ABCDабвг)");
    tf_eval (&ctx, bc, buffer, 1000);
    EXPECT_TRUE(!strcmp (buffer, "ABCDабвг"));
}

TEST_F(TitleFormattingTests, test_AsciiTestString_ReturnsAsciiSameStringWithInvalidCharsStripped) {
    char *bc = tf_compile("$ascii(олдABCDабвг)");
    tf_eval (&ctx, bc, buffer, 1000);
    EXPECT_TRUE(!strcmp (buffer, "ABCD"));
}


TEST_F(TitleFormattingTests, test_CapsTestAsciiString_ReturnsCapitalizeEachWordString) {
    char *bc = tf_compile("$caps(MY TEST STRING)");
    tf_eval (&ctx, bc, buffer, 1000);
    EXPECT_TRUE(!strcmp (buffer, "My Test String"));
}


TEST_F(TitleFormattingTests, test_CapsTestAsciiRandomizedString_ReturnsCapitalizeEachWordString) {
    char *bc = tf_compile("$caps(MY TesT STriNG)");
    tf_eval (&ctx, bc, buffer, 1000);
    EXPECT_TRUE(!strcmp (buffer, "My Test String"));
}

TEST_F(TitleFormattingTests, test_CapsTestUnicodeRandomizedString_ReturnsCapitalizeEachWordString) {
    char *bc = tf_compile("$caps(AsciiAlbumName РуССкоЕНазВАние ΠΥΘΑΓΌΡΑΣ)");
    tf_eval (&ctx, bc, buffer, 1000);
    EXPECT_TRUE(!strcmp (buffer, "Asciialbumname Русскоеназвание Πυθαγόρασ"));
}

TEST_F(TitleFormattingTests, test_CapsTestUnicodeStringWithNonMatchinByteLengthsForLowerUpperCaseChars_ReturnsCapitalizeEachWordString) {
    char *bc = tf_compile("$caps(ɑBCD ɀHİJ)");
    tf_eval (&ctx, bc, buffer, 1000);
    EXPECT_TRUE(!strcmp (buffer, "Ɑbcd Ɀhij"));
}

TEST_F(TitleFormattingTests, test_Caps2TestUnicodeRandomizedString_ReturnsCapitalizeEachWordString) {
    char *bc = tf_compile("$caps2(ɑBCD ɀHİJ)");
    tf_eval (&ctx, bc, buffer, 1000);
    EXPECT_TRUE(!strcmp (buffer, "ⱭBCD ⱿHİJ"));
}

TEST_F(TitleFormattingTests, test_Char1055And88And38899_ReturnsCorrespondingUTF8Chars) {
    char *bc = tf_compile("$char(1055)$char(88)$char(38899)");
    tf_eval (&ctx, bc, buffer, 1000);
    EXPECT_TRUE(!strcmp (buffer, "ПX音"));
}

TEST_F(TitleFormattingTests, test_Crc32Of123456789_Returns3421780262) {
    char *bc = tf_compile("$crc32(123456789)");
    tf_eval (&ctx, bc, buffer, 1000);
    EXPECT_TRUE(!strcmp (buffer, "3421780262"));
}

TEST_F(TitleFormattingTests, test_CrLf_InsertsLinebreak) {
    ctx.flags |= DDB_TF_CONTEXT_MULTILINE;
    char *bc = tf_compile("$crlf()");
    tf_eval (&ctx, bc, buffer, 1000);
    EXPECT_TRUE(!strcmp (buffer, "\n"));
}

TEST_F(TitleFormattingTests, test_DirectoryOnFilePath_ReturnsDirectory) {
    char *bc = tf_compile("$directory(/directory/file.path)");
    tf_eval (&ctx, bc, buffer, 1000);
    EXPECT_TRUE(!strcmp (buffer, "directory"));
}

TEST_F(TitleFormattingTests, test_DirectoryOnFilePathWithMultipleSlashes_ReturnsDirectory) {
    char *bc = tf_compile("$directory(/directory///file.path)");
    tf_eval (&ctx, bc, buffer, 1000);
    EXPECT_TRUE(!strcmp (buffer, "directory"));
}

TEST_F(TitleFormattingTests, test_DirectoryOnFilePathWithoutFrontSlash_ReturnsDirectory) {
    char *bc = tf_compile("$directory(directory/file.path)");
    tf_eval (&ctx, bc, buffer, 1000);
    EXPECT_TRUE(!strcmp (buffer, "directory"));
}

TEST_F(TitleFormattingTests, test_DirectoryOnFilePathWithMoreNestedness_ReturnsDirectory) {
    char *bc = tf_compile("$directory(/path/directory/file.path)");
    tf_eval (&ctx, bc, buffer, 1000);
    EXPECT_TRUE(!strcmp (buffer, "directory"));
}

TEST_F(TitleFormattingTests, test_DirectoryOnFilePathWithoutDirectory_ReturnsEmpty) {
    char *bc = tf_compile("$directory(file.path)");
    tf_eval (&ctx, bc, buffer, 1000);
    EXPECT_TRUE(!strcmp (buffer, ""));
}

TEST_F(TitleFormattingTests, test_DirectoryOnFilePathAtRoot_ReturnsEmpty) {
    char *bc = tf_compile("$directory(/file.path)");
    tf_eval (&ctx, bc, buffer, 1000);
    EXPECT_TRUE(!strcmp (buffer, ""));
}

TEST_F(TitleFormattingTests, test_DirectoryOnEmptyAtRoot_ReturnsEmpty) {
    char *bc = tf_compile("$directory(/)");
    tf_eval (&ctx, bc, buffer, 1000);
    EXPECT_TRUE(!strcmp (buffer, ""));
}

TEST_F(TitleFormattingTests, test_DirectoryOnEmpty_ReturnsEmpty) {
    char *bc = tf_compile("$directory()");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, ""));
}

TEST_F(TitleFormattingTests, test_DirectoryOnFilePathLevel0_ReturnsEmpty) {
    char *bc = tf_compile("$directory(/directory/file.path,0)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, ""));
}

TEST_F(TitleFormattingTests, test_DirectoryOnFilePathLevel1_ReturnsDirectory1) {
    char *bc = tf_compile("$directory(/directory3/directory2/directory1/file.path,1)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "directory1"));
}

TEST_F(TitleFormattingTests, test_DirectoryOnFilePathLevel2_ReturnsDirectory2) {
    char *bc = tf_compile("$directory(/directory3/directory2/directory1/file.path,2)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "directory2"));
}

TEST_F(TitleFormattingTests, test_DirectoryOnFilePathLevel4_ReturnsEmpty) {
    char *bc = tf_compile("$directory(/directory3/directory2/directory/file.path,4)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, ""));
}

TEST_F(TitleFormattingTests, test_DirectoryOnFilePathLevel2MultipleSlashes_ReturnsDirectory2) {
    char *bc = tf_compile("$directory(////directory3////directory2////directory////file.path,2)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "directory2"));
}

TEST_F(TitleFormattingTests, test_MultiLine_LineBreaksIgnored) {
    char *bc = tf_compile("hello\nworld");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "helloworld"));
}

TEST_F(TitleFormattingTests, test_MultiLineWithComments_LineBreaksAndCommentedLinesIgnored) {
    char *bc = tf_compile("// this is a comment\nhello\nworld\n//another comment\nmore text");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "helloworldmore text"));
}

TEST_F(TitleFormattingTests, test_QuotedSpecialChars_TreatedLiterally) {
    char *bc = tf_compile("'blah$blah%blah[][]'");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "blah$blah%blah[][]"));
}

TEST_F(TitleFormattingTests, test_FunctionArgumentsOnMultipleLinesWithComments_LinebreaksAndCommentsIgnored) {
    char *bc = tf_compile("$add(1,\n2,\n3,//4,\n5)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "11"));
}

TEST_F(TitleFormattingTests, test_DirectoryPathOnFilePath_ReturnsDirectoryPath) {
    char *bc = tf_compile("$directory_path('/a/b/c/d.mp3')");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "/a/b/c"));
}

TEST_F(TitleFormattingTests, test_DirectoryPathOnPathWithoutFile_ReturnsDirectoryPath) {
    char *bc = tf_compile("$directory_path('/a/b/c/d/')");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "/a/b/c/d"));
}

TEST_F(TitleFormattingTests, test_ExtOnFilePath_ReturnsExt) {
    char *bc = tf_compile("$ext('/a/b/c/d/file.mp3')");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "mp3"));
}

TEST_F(TitleFormattingTests, test_ExtOnFileWithoutExtPath_ReturnsEmpty) {
    char *bc = tf_compile("$ext('/a/b/c/d/file')");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, ""));
}

TEST_F(TitleFormattingTests, test_ExtOnFilePathWithoutFilename_ReturnsEmpty) {
    char *bc = tf_compile("$ext('/a/b/c/d/')");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, ""));
}

TEST_F(TitleFormattingTests, test_ExtOnFilePathEndingWithDot_ReturnsEmpty) {
    char *bc = tf_compile("$ext('/a/b/c/d/file.')");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, ""));
}

TEST_F(TitleFormattingTests, test_ExtOnFilePathDotFile_ReturnsExt) {
    char *bc = tf_compile("$ext('/a/b/c/d/.ext')");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "ext"));
}

TEST_F(TitleFormattingTests, test_ExtOnFileExtWithMultiplePeriod_ReturnsExt) {
    char *bc = tf_compile("$ext('/a/b/c/d/file.iso.wv')");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "wv"));
}

TEST_F(TitleFormattingTests, test_FilenameOnFilePath_ReturnsFilename) {
    char *bc = tf_compile("$filename('/a/b/c/d/file.mp3')");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "file.mp3"));
}

TEST_F(TitleFormattingTests, test_FilenameOnFilePathWithoutFile_ReturnsEmpty) {
    char *bc = tf_compile("$filename('/a/b/c/d/')");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, ""));
}

TEST_F(TitleFormattingTests, test_FilenameOnFilenameWithoutPath_ReturnsFilename) {
    char *bc = tf_compile("$filename('file.iso.wv')");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "file.iso.wv"));
}

TEST_F(TitleFormattingTests, test_FilenameMeta_ReturnsFilename) {
    pl_replace_meta (it, ":URI", "/path/file.mp3");
    char *bc = tf_compile("%filename%");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "file"));
}

TEST_F(TitleFormattingTests, test_Date_ReturnsYearValue) {
    pl_replace_meta (it, "year", "1980");
    char *bc = tf_compile("%date%");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "1980"));
}

TEST_F(TitleFormattingTests, test_CustomField_ReturnsTheFieldValue) {
    pl_replace_meta (it, "random_name", "random value");
    char *bc = tf_compile("%random_name%");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "random value"));
}

TEST_F(TitleFormattingTests, test_MultipleArtists_ReturnsArtistsSeparatedByCommas) {
    pl_append_meta (it, "artist", "Artist1");
    pl_append_meta (it, "artist", "Artist2");
    char *bc = tf_compile("%artist%");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_STREQ(buffer, "Artist1, Artist2");
}

TEST_F(TitleFormattingTests, test_EmptyTitle_YieldsFilename) {
    pl_replace_meta (it, ":URI", "/home/user/filename.mp3");
    char *bc = tf_compile("%title%");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_STREQ(buffer, "filename");
}

TEST_F(TitleFormattingTests, test_DoublingPercentDollarApostrophe_OutputsSinglePercentDollarApostrophe) {
    char *bc = tf_compile("''$$%%");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_STREQ(buffer, "'$%");
}

TEST_F(TitleFormattingTests, test_PlaybackTime_OutputsPlaybackTime) {
    streamer_set_playing_track (it);
    char *bc = tf_compile("%playback_time%");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "0:00"));
}

TEST_F(TitleFormattingTests, test_PlaybackTime_OutputsPlaybackTimeMs) {
    streamer_set_playing_track (it);
    char *bc = tf_compile("%playback_time_ms%");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "0:00.000"));
}

TEST_F(TitleFormattingTests, test_NoDynamicFlag_SkipsDynamicFields) {
    char *bc = tf_compile("header|%playback_time%|footer");
    ctx.flags |= DDB_TF_CONTEXT_NO_DYNAMIC;
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "header||footer"));
}

TEST_F(TitleFormattingTests, test_Track_Number_SingleDigit_ReturnsNonZeroPaddedTrackNumber) {
    pl_replace_meta (it, "track", "5");
    char *bc = tf_compile("%track number%");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "5"));
}

TEST_F(TitleFormattingTests, test_Track_Number_NonNumerical_ReturnsUnmodified) {
    pl_replace_meta (it, "track", "A01");
    char *bc = tf_compile("%track number%");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "A01"));
}

TEST_F(TitleFormattingTests, test_Track_Number_PaddingZero_ReturnsUnmodified) {
    pl_replace_meta (it, "track", "001");
    char *bc = tf_compile("%track number%");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "001"));
}

TEST_F(TitleFormattingTests, test_TrackNumber_SingleDigit_PaddingZeroAdded) {
    pl_replace_meta (it, "track", "1");
    char *bc = tf_compile("%tracknumber%");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "01"));
}

TEST_F(TitleFormattingTests, test_TrackNumber_PaddingZero_ReturnsUnmodified) {
    pl_replace_meta (it, "track", "001");
    char *bc = tf_compile("%tracknumber%");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "001"));
}

TEST_F(TitleFormattingTests, test_Length_DoesntGetPaddedWithSpace) {
    plt_set_item_duration(NULL, it, 130);
    char *bc = tf_compile("%length%");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "2:10"));
}

TEST_F(TitleFormattingTests, test_ImportLegacyDirectAccess_ProducesExpectedData) {
    const char *old = "%@disc@";
    char buf[100];
    tf_import_legacy (old, buf, sizeof (buf));
    EXPECT_TRUE(!strcmp (buf, "%disc%"));
}

TEST_F(TitleFormattingTests, test_NestedSquareBracketsWithUndefVarsAndLiteralData_ReturnEmpty) {
    pl_replace_meta (it, "title", "title");
    char *bc = tf_compile("[[%discnumber%]a] normaltext [%title%] [[%title%]a]");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, " normaltext title titlea"));
}

TEST_F(TitleFormattingTests, test_FixEof_PutsIndicatorAfterLineBreak) {
    pl_replace_meta (it, "title", "line1\nline2\n");
    char *bc = tf_compile("$fix_eol(%title%)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "line1 (...)"));
}

TEST_F(TitleFormattingTests, test_FixEofTwoArgs_PutsCustomIndicatorAfterLineBreak) {
    pl_replace_meta (it, "title", "line1\nline2\n");
    char *bc = tf_compile("$fix_eol(%title%, _..._)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "line1 _..._"));
}

TEST_F(TitleFormattingTests, test_FixEofTwoArgsWithSmallBuffer_DoesntOverflowOffByOne) {
    pl_replace_meta (it, "title", "hello\n");
    char *bc = tf_compile("$fix_eol(%title%, _..._)");
    tf_eval (&ctx, bc, buffer, 12);
    EXPECT_TRUE(!strcmp (buffer, "hello _..._"));
    tf_eval (&ctx, bc, buffer, 11);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, ""));
}

TEST_F(TitleFormattingTests, test_Hex_ReturnsHexConvertedNumber) {
    char *bc = tf_compile("$hex(11259375)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "abcdef"));
}

TEST_F(TitleFormattingTests, test_HexPadded_ReturnsHexConvertedNumberWithPadding) {
    char *bc = tf_compile("$hex(11259375,10)");
    tf_eval (&ctx, bc, buffer, 10);
    EXPECT_TRUE(!strcmp (buffer, ""));
    tf_eval (&ctx, bc, buffer, 11);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "0000abcdef"));
}

TEST_F(TitleFormattingTests, test_HexZero_ReturnsZero) {
    char *bc = tf_compile("$hex(0)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "0"));
}

TEST_F(TitleFormattingTests, test_QuotedSquareBrackets_ReturnsSquareBrackets) {
    char *bc = tf_compile("'['']'");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "[]"));
}

TEST_F(TitleFormattingTests, test_ImportLegacySquareBrackets_ProducesQuotedSquareBrackets) {
    const char *old = "[%y]";
    char buf[100];
    tf_import_legacy (old, buf, sizeof (buf));
    EXPECT_TRUE(!strcmp (buf, "'['%date%']'"));
}

TEST_F(TitleFormattingTests, test_Num_123_5_Returns_00123) {
    char *bc = tf_compile("$num(123,5)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "00123"));
}

TEST_F(TitleFormattingTests, test_Num_Minus123_5_Returns__0123) {
    char *bc = tf_compile("$num(-123,5)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "-0123"));
}

TEST_F(TitleFormattingTests, test_NumFractional_ReturnsIntegerFloor) {
    char *bc = tf_compile("$num(4.8,5)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "00004"));
}

TEST_F(TitleFormattingTests, test_NumNonNumber_ReturnsZero) {
    char *bc = tf_compile("$num(A1,5)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "00000"));
}

TEST_F(TitleFormattingTests, test_NumLargeNumber_DoesntTruncate) {
    char *bc = tf_compile("$num(1234,3)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "1234"));
}

TEST_F(TitleFormattingTests, test_NumNegativePadding_GivesZeroPadding) {
    char *bc = tf_compile("$num(1,-3)");
    tf_eval (&ctx, bc, buffer, 1000);
    EXPECT_TRUE(!strcmp (buffer, "1"));
}

TEST_F(TitleFormattingTests, test_isPlaying_StatePlayingAndStreamerTrackNotSameAsCtxTrack_ReturnsNone) {
    streamer_set_playing_track (it);
    ctx.it = NULL;
    fake_out_state_value = DDB_PLAYBACK_STATE_PLAYING;
    char *bc = tf_compile("$if(%isplaying%,YES,NO) %isplaying%");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "NO "));
}

TEST_F(TitleFormattingTests, test_isPlaying_StatePlayingAndStreamerTrackSameAsCtxTrack_Returns1) {
    streamer_set_playing_track (it);
    fake_out_state_value = DDB_PLAYBACK_STATE_PLAYING;
    char *bc = tf_compile("$if(%isplaying%,YES,NO) %isplaying%");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "YES 1"));
}

TEST_F(TitleFormattingTests, test_IsPlaying_StateStopped_ReturnsNone) {
    fake_out_state_value = DDB_PLAYBACK_STATE_STOPPED;
    char *bc = tf_compile("$if(%isplaying%,YES,NO) %isplaying%");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "NO "));
}

TEST_F(TitleFormattingTests, test_isPaused_StatePlayingAndStreamerTrackNotSameAsCtxTrack_ReturnsNone) {
    streamer_set_playing_track (it);
    ctx.it = NULL;
    fake_out_state_value = DDB_PLAYBACK_STATE_PLAYING;
    char *bc = tf_compile("$if(%ispaused%,YES,NO) %ispaused%");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "NO "));
}

TEST_F(TitleFormattingTests, test_isPaused_StatePlayingAndStreamerTrackSameAsCtxTrack_ReturnsNone) {
    streamer_set_playing_track (it);
    fake_out_state_value = DDB_PLAYBACK_STATE_PLAYING;
    char *bc = tf_compile("$if(%ispaused%,YES,NO) %ispaused%");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "NO "));
}

TEST_F(TitleFormattingTests, test_isPaused_StatePausedAndStreamerTrackNotSameAsCtxTrack_ReturnsNone) {
    streamer_set_playing_track (it);
    ctx.it = NULL;
    fake_out_state_value = DDB_PLAYBACK_STATE_PAUSED;
    char *bc = tf_compile("$if(%ispaused%,YES,NO) %ispaused%");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "NO "));
}

TEST_F(TitleFormattingTests, test_isPaused_StatePausedAndStreamerTrackSameAsCtxTrack_Returns1) {
    streamer_set_playing_track (it);
    fake_out_state_value = DDB_PLAYBACK_STATE_PAUSED;
    char *bc = tf_compile("$if(%ispaused%,YES,NO) %ispaused%");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "YES 1"));
}

TEST_F(TitleFormattingTests, test_MultiValueField_OutputAsCommaSeparated) {
    pl_append_meta(it, "artist", "Value1");
    pl_append_meta(it, "artist", "Value2");
    pl_append_meta(it, "artist", "Value3");
    char *bc = tf_compile("%artist%");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "Value1, Value2, Value3"));
}

TEST_F(TitleFormattingTests, test_LinebreaksAndTabs_OutputAsUnderscores) {
    pl_append_meta(it, "artist", "Text1\r\nText2\tText3");
    char *bc = tf_compile("%artist%");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "Text1__Text2_Text3"));
}

TEST_F(TitleFormattingTests, test_NestedSquareBracketsWithDefinedAndUndefinedVars_ReturnNonEmpty) {
    pl_replace_meta (it, "title", "title");
    char *bc = tf_compile("header [[%discnumber%][%title%] a] footer");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "header title a footer"));
}

TEST_F(TitleFormattingTests, test_PathStripFileUriScheme_ReturnStripped) {
    pl_replace_meta (it, ":URI", "file:///home/user/filename.mp3");
    char *bc = tf_compile("%path%");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "/home/user/filename.mp3"));
}

TEST_F(TitleFormattingTests, test_PathStripHTTPUriScheme_ReturnUnStripped) {
    pl_replace_meta (it, ":URI", "http://example.com/filename.mp3");
    char *bc = tf_compile("%path%");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "http://example.com/filename.mp3"));
}

TEST_F(TitleFormattingTests, test_RawPathWithFileUriScheme_ReturnUnStripped) {
    pl_replace_meta (it, ":URI", "file:///home/user/filename.mp3");
    char *bc = tf_compile("%_path_raw%");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "file:///home/user/filename.mp3"));
}

TEST_F(TitleFormattingTests, test_RawPathWithHttpUriScheme_ReturnUnStripped) {
    pl_replace_meta (it, ":URI", "http://example.com/filename.mp3");
    char *bc = tf_compile("%_path_raw%");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "http://example.com/filename.mp3"));
}

TEST_F(TitleFormattingTests, test_RawPathAbsolutePath_ReturnsExpected) {
    pl_replace_meta (it, ":URI", "/path/to/filename.mp3");
    char *bc = tf_compile("%_path_raw%");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "file:///path/to/filename.mp3"));
}

TEST_F(TitleFormattingTests, test_RawPathRelativePath_ReturnsEmpty) {
    pl_replace_meta (it, ":URI", "relative/path/to/filename.mp3");
    char *bc = tf_compile("%_path_raw%");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, ""));
}

TEST_F(TitleFormattingTests, test_PlaylistName_ReturnsPlaylistName) {
    char *bc = tf_compile("%_playlist_name%");
    playlist_t plt = {0};
    plt.title = (char *)"Test Playlist";
    ctx.plt = (ddb_playlist_t *)&plt;
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "Test Playlist"));
}

TEST_F(TitleFormattingTests, test_ReplaceWith3Arguments_ReturnsExpectedValue) {
    char *bc = tf_compile("$replace(ab,a,b,b,c)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "bc"));
}

TEST_F(TitleFormattingTests, test_ReplaceWith3ArgumentsNested_ReturnsExpectedValue) {
    char *bc = tf_compile("$replace($replace(ab,a,b),b,c)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "cc"));
}

TEST_F(TitleFormattingTests, test_ReplaceWith1Argument_ReturnsEmpty) {
    char *bc = tf_compile("$replace(ab)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, ""));
}

TEST_F(TitleFormattingTests, test_ReplaceWith2Arguments_ReturnsEmpty) {
    char *bc = tf_compile("$replace(ab,a)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, ""));
}

TEST_F(TitleFormattingTests, test_ReplaceWith4Arguments_ReturnsEmpty) {
    char *bc = tf_compile("$replace(ab,a,c,d)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, ""));
}

TEST_F(TitleFormattingTests, test_ReplaceLongerSubstring_ReturnsExpected) {
    char *bc = tf_compile("$replace(foobar,foo,DeaD,bar,BeeF)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "DeaDBeeF"));
}

TEST_F(TitleFormattingTests, test_replace_emptyStringWithEmpty_shouldComplete) {
    char *bc = tf_compile("$replace(foobar,,)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);

    EXPECT_STREQ(buffer, "foobar");
}

TEST_F(TitleFormattingTests, test_replace_emptyStringWithSomething_shouldProduceOriginalString) {
    char *bc = tf_compile("$replace(foobar,,bar)");

    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);

    EXPECT_TRUE(!strcmp (buffer, "foobar"));
}

TEST_F(TitleFormattingTests, test_FilenameExt_ReturnsFilenameWithExt) {
    pl_replace_meta (it, ":URI", "/Users/User/MyFile.mod");
    char *bc = tf_compile("%filename_ext%");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "MyFile.mod"));
}

TEST_F(TitleFormattingTests, test_UpperForAllLowerCase_ReturnsUppercase) {
    char *bc = tf_compile("$upper(abcd)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "ABCD"));
}

TEST_F(TitleFormattingTests, test_UpperForAllUpperCase_ReturnsUppercase) {
    char *bc = tf_compile("$upper(ABCD)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "ABCD"));
}

TEST_F(TitleFormattingTests, test_UpperForMixed_ReturnsUppercase) {
    char *bc = tf_compile("$upper(aBcD)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "ABCD"));
}

TEST_F(TitleFormattingTests, test_UpperForAllLowerCaseNonAscii_ReturnsUppercase) {
    char *bc = tf_compile("$upper(абвгд)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "АБВГД"));
}

TEST_F(TitleFormattingTests, test_LowerForAllUpperCase_ReturnsLowercase) {
    char *bc = tf_compile("$lower(ABCD)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "abcd"));
}

TEST_F(TitleFormattingTests, test_LowerForAllLowerCase_ReturnsLowercase) {
    char *bc = tf_compile("$lower(abcd)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "abcd"));
}

TEST_F(TitleFormattingTests, test_LowerForMixed_ReturnsLowercase) {
    char *bc = tf_compile("$lower(aBcD)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "abcd"));
}

TEST_F(TitleFormattingTests, test_LowerForAllUpperCaseNonAscii_ReturnsLowercase) {
    char *bc = tf_compile("$lower(АБВГД)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "абвгд"));
}

TEST_F(TitleFormattingTests, test_PathWithNullUri_ReturnsEmpty) {
    pl_delete_meta (it, ":URI");
    char *bc = tf_compile("%path%");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, ""));
}

TEST_F(TitleFormattingTests, test_RawPathWithNullUri_ReturnsEmpty) {
    pl_delete_meta (it, ":URI");
    char *bc = tf_compile("%_path_raw%");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, ""));
}

TEST_F(TitleFormattingTests, test_VfsPathTitle_GivesCorrectTitle) {
    pl_replace_meta (it, ":URI", "/path/file/myfile.zip:mytrack.mp3");
    char *bc = tf_compile("%title%");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "mytrack"));
}

TEST_F(TitleFormattingTests, test_RepeatSingleChar11Times_Gives11Chars) {
    char *bc = tf_compile("$repeat(x,11)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "xxxxxxxxxxx"));
}

TEST_F(TitleFormattingTests, test_RepeatTwoChars3Times_Gives3DoubleChars) {
    char *bc = tf_compile("$repeat(xy,3)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "xyxyxy"));
}

TEST_F(TitleFormattingTests, test_RepeatCalculatedExpr2Times_Gives2Exprs) {
    pl_replace_meta (it, "title", "abc");
    char *bc = tf_compile("$repeat(%title%,2)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "abcabc"));
}

TEST_F(TitleFormattingTests, test_RepeatCalculatedExprNTimes_GivesNExprs) {
    pl_replace_meta (it, "title", "abc");
    pl_replace_meta (it, "count", "3");
    char *bc = tf_compile("$repeat(%title%,%count%)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "abcabcabc"));
}

TEST_F(TitleFormattingTests, test_RepeatSingleCharZeroTimes_GivesZeroChars) {
    char *bc = tf_compile("pre$repeat(x,0)post");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "prepost"));
}

TEST_F(TitleFormattingTests, test_InsertStrMiddle_GivesInsertedStr) {
    pl_replace_meta (it, "title", "Insert [] Here");
    pl_replace_meta (it, "album", "Value");
    char *bc = tf_compile("$insert(%title%,%album%,8)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "Insert [Value] Here"));
}

TEST_F(TitleFormattingTests, test_InsertStrMiddleUnicode_GivesInsertedStr) {
    pl_replace_meta (it, "title", "Вставить [] сюда");
    pl_replace_meta (it, "album", "Значение");
    char *bc = tf_compile("$insert(%title%,%album%,10)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "Вставить [Значение] сюда"));
}

TEST_F(TitleFormattingTests, test_InsertStrEnd_GivesAppendedStr) {
    pl_replace_meta (it, "title", "Insert Here:");
    pl_replace_meta (it, "album", "Value");
    char *bc = tf_compile("$insert(%title%,%album%,12)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "Insert Here:Value"));
}

TEST_F(TitleFormattingTests, test_InsertStrOutOfBounds_GivesAppendedStr) {
    pl_replace_meta (it, "title", "Insert Here:");
    pl_replace_meta (it, "album", "Value");
    char *bc = tf_compile("$insert(%title%,%album%,13)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "Insert Here:Value"));
}

TEST_F(TitleFormattingTests, test_InsertStrBufferTooSmallUnicode_GivesTruncatedAtBeforeStr) {
    pl_replace_meta (it, "title", "Вставить [] сюда");
    pl_replace_meta (it, "album", "Значение");
    char *bc = tf_compile("$insert(%title%,%album%,10)");
    tf_eval (&ctx, bc, buffer, 5);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "Вс"));
}

TEST_F(TitleFormattingTests, test_InsertStrBufferTooSmallUnicode_GivesTruncatedAtMiddleStr) {
    pl_replace_meta (it, "title", "Вставить [] сюда");
    pl_replace_meta (it, "album", "Значение");
    char *bc = tf_compile("$insert(%title%,%album%,10)");
    tf_eval (&ctx, bc, buffer, 27);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "Вставить [Знач"));
}

TEST_F(TitleFormattingTests, test_InsertStrBufferTooSmallUnicode_GivesTruncatedAtAfterStr) {
    pl_replace_meta (it, "title", "Вставить [] сюда");
    pl_replace_meta (it, "album", "Значение");
    char *bc = tf_compile("$insert(%title%,%album%,10)");
    tf_eval (&ctx, bc, buffer, 41);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "Вставить [Значение] сю"));
}

TEST_F(TitleFormattingTests, test_InsertStrBegin_GivesPrependedStr) {
    pl_replace_meta (it, "title", ":Insert Before");
    pl_replace_meta (it, "album", "Value");
    char *bc = tf_compile("$insert(%title%,%album%,0)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "Value:Insert Before"));
}

TEST_F(TitleFormattingTests, test_LeftOfUnicodeString_Takes2Chars) {
    char *bc = tf_compile("$left(АБВГД,2)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "АБ"));
}

TEST_F(TitleFormattingTests, test_Left2OfUnicodeStringBufFor1Char_Takes1Char) {
    char *bc = tf_compile("$left(АБВГД,2)");
    tf_eval (&ctx, bc, buffer, 3);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "А"));
}

TEST_F(TitleFormattingTests, test_LenOfUnicodeString_ReturnsLengthInChars) {
    char *bc = tf_compile("$len(АБВГД)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "5"));
}

TEST_F(TitleFormattingTests, test_DimTextExpression_ReturnsPlainText) {
    char *bc = tf_compile("<<<dim this text>>>");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!ctx.dimmed);
    EXPECT_TRUE(!strcmp (buffer, "dim this text"));
}

TEST_F(TitleFormattingTests, test_DimTextExpression_ReturnsTextWithDimEscSequence) {
    char *bc = tf_compile("<<<dim this text>>>");
    ctx.flags |= DDB_TF_CONTEXT_TEXT_DIM;
    tf_eval (&ctx, bc, buffer, 1000);
    ctx.flags &= ~DDB_TF_CONTEXT_TEXT_DIM;
    tf_free (bc);
    EXPECT_TRUE(ctx.dimmed);
    EXPECT_TRUE(!strcmp (buffer, "\0331;-3mdim this text\0331;3m"));
}

TEST_F(TitleFormattingTests, test_BrightenTextExpression_ReturnsTextWithBrightenEscSequence) {
    char *bc = tf_compile(">>>brighten this text<<<");
    ctx.flags |= DDB_TF_CONTEXT_TEXT_DIM;
    tf_eval (&ctx, bc, buffer, 1000);
    ctx.flags &= ~DDB_TF_CONTEXT_TEXT_DIM;
    tf_free (bc);
    EXPECT_TRUE(ctx.dimmed);
    EXPECT_TRUE(!strcmp (buffer, "\0331;3mbrighten this text\0331;-3m"));
}

TEST_F(TitleFormattingTests, test_BrightenInfiniteLengthTextExpression_ReturnsTextWithBrightenEscSequence) {
    plt_set_item_duration(NULL, it, -1);
    char *bc = tf_compile("xxx>>>aaa%length%bbb<<<yyy");
    ctx.flags |= DDB_TF_CONTEXT_TEXT_DIM;
    tf_eval (&ctx, bc, buffer, 1000);
    ctx.flags &= ~DDB_TF_CONTEXT_TEXT_DIM;
    tf_free (bc);
    EXPECT_TRUE(ctx.dimmed);
    EXPECT_TRUE(!strcmp (buffer, "xxx\0331;3maaabbb\0331;-3myyy"));
}

TEST_F(TitleFormattingTests, test_0_7_2_ContextSizeCheck_ReturnsResult) {
    char *bc = tf_compile("test");
    ctx._size = (int)((char *)&ctx.dimmed - (char *)&ctx);
    tf_eval (&ctx, bc, buffer, 1000);
    ctx._size = sizeof (ctx);
    EXPECT_TRUE(!strcmp (buffer, "test"));
}

TEST_F(TitleFormattingTests, test_InvalidContextSizeCheck_ReturnsEmpty) {
    char *bc = tf_compile("test");
    ctx._size = (int)((char *)&ctx.dimmed - (char *)&ctx - 1);
    tf_eval (&ctx, bc, buffer, 1000);
    ctx._size = sizeof (ctx);
    EXPECT_STREQ(buffer, "");
}

TEST_F(TitleFormattingTests, test_PadHelloWith5_GivesHello) {
    char *bc = tf_compile("$pad(Hello,5)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "Hello"));
}

TEST_F(TitleFormattingTests, test_PadHelloWith5Xs_GivesHello) {
    char *bc = tf_compile("$pad(Hello,5,X)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "Hello"));
}

TEST_F(TitleFormattingTests, test_PadHelloWith10_Gives_Hello_____) {
    char *bc = tf_compile("$pad(Hello,10)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "Hello     "));
}

TEST_F(TitleFormattingTests, test_PadHelloWith10Xs_GivesHelloXXXXX) {
    char *bc = tf_compile("$pad(Hello,10,X)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "HelloXXXXX"));
}

TEST_F(TitleFormattingTests, test_PadHelloWith10XYs_GivesHelloXXXXX) {
    char *bc = tf_compile("$pad(Hello,10,XY)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "HelloXXXXX"));
}

TEST_F(TitleFormattingTests, test_PadUnicodeStringWith10UnicodeChars_GivesExpectedOutput) {
    char *bc = tf_compile("$pad(АБВГД,10,Ё)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "АБВГДЁЁЁЁЁ"));
}

TEST_F(TitleFormattingTests, test_PadRightHelloWith5_GivesHello) {
    char *bc = tf_compile("$pad_right(Hello,5)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "Hello"));
}

TEST_F(TitleFormattingTests, test_PadRightHelloWith5Xs_GivesHello) {
    char *bc = tf_compile("$pad_right(Hello,5,X)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "Hello"));
}

TEST_F(TitleFormattingTests, test_PadRightHelloWith10_Gives______Hello) {
    char *bc = tf_compile("$pad_right(Hello,10)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "     Hello"));
}

TEST_F(TitleFormattingTests, test_PadRightHelloWith10Xs_GivesXXXXXHello) {
    char *bc = tf_compile("$pad_right(Hello,10,X)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "XXXXXHello"));
}

TEST_F(TitleFormattingTests, test_PadRightHelloWith10XYs_GivesXXXXXHello) {
    char *bc = tf_compile("$pad_right(Hello,10,XY)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "XXXXXHello"));
}

TEST_F(TitleFormattingTests, test_PadRightUnicodeStringWith10UnicodeChars_GivesExpectedOutput) {
    char *bc = tf_compile("$pad_right(АБВГД,10,Ё)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "ЁЁЁЁЁАБВГД"));
}

TEST_F(TitleFormattingTests, test_StripPrefix_NoArgs_Fail) {
    char *bc = tf_compile("$stripprefix()");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, ""));
}

TEST_F(TitleFormattingTests, test_StripPrefix_NoArticle_PassThrough) {
    char *bc = tf_compile("$stripprefix(Hello)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "Hello"));
}

TEST_F(TitleFormattingTests, test_StripPrefix_JoinedA_PassThrough) {
    char *bc = tf_compile("$stripprefix(AA)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "AA"));
}

TEST_F(TitleFormattingTests, test_StripPrefix_JoinedThe_PassThrough) {
    char *bc = tf_compile("$stripprefix(TheThe)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "TheThe"));
}

TEST_F(TitleFormattingTests, test_StripPrefix_MultipleA_OneStripped) {
    char *bc = tf_compile("$stripprefix(A A)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "A"));
}

TEST_F(TitleFormattingTests, test_StripPrefix_MultipleThe_OneStripped) {
    char *bc = tf_compile("$stripprefix(The The)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "The"));
}

TEST_F(TitleFormattingTests, test_StripPrefix_MultipleSpaces_AllSpacesSkipped) {
    char *bc = tf_compile("$stripprefix(A  Word)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, " Word"));
}

TEST_F(TitleFormattingTests, test_StripPrefix_CustomPrefixList_PrefixStripped) {
    char *bc = tf_compile("$stripprefix(Some Word,The,A,Some,Word)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "Word"));
}

TEST_F(TitleFormattingTests, test_SwapPrefix_NoArticle_PassThrough) {
    char *bc = tf_compile("$swapprefix(Hello)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "Hello"));
}

TEST_F(TitleFormattingTests, test_SwapPrefix_JoinedA_PassThrough) {
    char *bc = tf_compile("$swapprefix(AA)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "AA"));
}

TEST_F(TitleFormattingTests, test_SwapPrefix_JoinedThe_PassThrough) {
    char *bc = tf_compile("$swapprefix(TheThe)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "TheThe"));
}

TEST_F(TitleFormattingTests, test_SwapPrefix_MultipleA_CommaAdded) {
    char *bc = tf_compile("$swapprefix(A A)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "A, A"));
}

TEST_F(TitleFormattingTests, test_SwapPrefix_MultipleThe_CommaAdded) {
    char *bc = tf_compile("$swapprefix(The The)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "The, The"));
}

TEST_F(TitleFormattingTests, test_SwapPrefix_MultipleSpaces_SpacePreservedWithComma) {
    char *bc = tf_compile("$swapprefix(A  Word)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, " Word, A"));
}

TEST_F(TitleFormattingTests, test_SwapPrefix_CustomPrefixList_PrefixSwapped) {
    char *bc = tf_compile("$swapprefix(Some Word,The,A,Some,Word)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "Word, Some"));
}

TEST_F(TitleFormattingTests, test_StricmpEqual_ReturnsYes) {
    char *bc = tf_compile("$if($stricmp(AbCd,AbCd),YES)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "YES"));
}

TEST_F(TitleFormattingTests, test_StricmpUnequal_ReturnsEmpty) {
    char *bc = tf_compile("$if($stricmp(AbCd,EfGh),YES)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, ""));
}

TEST_F(TitleFormattingTests, test_StricmpEqualWithDifferentCase_ReturnsYES) {
    char *bc = tf_compile("$if($stricmp(ABCD,abcd),YES)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "YES"));
}

TEST_F(TitleFormattingTests, test_Len2AsciiChars_ReturnsNumberOfChars) {
    char *bc = tf_compile("$len2(ABCDE)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "5"));
}

TEST_F(TitleFormattingTests, test_Len2UnicodeSingleWidthChars_ReturnsNumberOfChars) {
    char *bc = tf_compile("$len2(АБВГД)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "5"));
}

TEST_F(TitleFormattingTests, test_Len2UnicodeDoubleWidthChars_ReturnsNumberOfCharsDoubled) {
    char *bc = tf_compile("$len2(全形)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "4"));
}

TEST_F(TitleFormattingTests, test_ShortestFirst_ReturnsFirst) {
    char *bc = tf_compile("$shortest(1,22,333)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "1"));
}

TEST_F(TitleFormattingTests, test_ShortestLast_ReturnsLast) {
    char *bc = tf_compile("$shortest(333,22,1)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "1"));
}

TEST_F(TitleFormattingTests, test_ShortestMid_ReturnsMid) {
    char *bc = tf_compile("$shortest(333,1,22)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "1"));
}

TEST_F(TitleFormattingTests, test_LongestFirst_ReturnsFirst) {
    char *bc = tf_compile("$longest(333,22,1)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "333"));
}

TEST_F(TitleFormattingTests, test_LongestLast_ReturnsLast) {
    char *bc = tf_compile("$longest(1,22,333)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "333"));
}

TEST_F(TitleFormattingTests, test_LongestMid_ReturnsMid) {
    char *bc = tf_compile("$longest(1,333,22)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "333"));
}

TEST_F(TitleFormattingTests, test_LongerFirst_ReturnsFirst) {
    char *bc = tf_compile("$longer(22,1)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "22"));
}

TEST_F(TitleFormattingTests, test_LongerSecond_ReturnsSecond) {
    char *bc = tf_compile("$longer(1,22)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "22"));
}

TEST_F(TitleFormattingTests, test_PadcutStrLonger_ReturnsHeadOfStr) {
    char *bc = tf_compile("$padcut(Hello,3)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "Hel"));
}


TEST_F(TitleFormattingTests, test_PadcutStrShorter_ReturnsPaddedStr) {
    char *bc = tf_compile("$padcut(Hello,8)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "Hello   "));
}


TEST_F(TitleFormattingTests, test_PadcutStrCharLonger_ReturnsHeadOfStr) {
    char *bc = tf_compile("$padcut(Hello,3,x)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "Hel"));
}

TEST_F(TitleFormattingTests, test_PadcutStrCharShorter_ReturnsPaddedStr) {
    char *bc = tf_compile("$padcut(Hello,8,x)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "Helloxxx"));
}

TEST_F(TitleFormattingTests, test_PadcutRightStrLonger_ReturnsHeadOfStr) {
    char *bc = tf_compile("$padcut_right(Hello,3)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "Hel"));
}


TEST_F(TitleFormattingTests, test_PadcutRightStrShorter_ReturnsPaddedStr) {
    char *bc = tf_compile("$padcut_right(Hello,8)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "   Hello"));
}


TEST_F(TitleFormattingTests, test_PadcutRightStrCharLonger_ReturnsHeadOfStr) {
    char *bc = tf_compile("$padcut_right(Hello,3,x)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "Hel"));
}

TEST_F(TitleFormattingTests, test_PadcutRightStrCharShorter_ReturnsPaddedStr) {
    char *bc = tf_compile("$padcut_right(Hello,8,x)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "xxxHello"));
}

TEST_F(TitleFormattingTests, test_ProgressPos0Range100Len10_ReturnsBar10CharsWithKnobAt0) {
    char *bc = tf_compile("$progress(0,100,10,x,=)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "x========="));
}

TEST_F(TitleFormattingTests, test_ProgressPos100Range100Len10_ReturnsBar10CharsWithKnobAt9) {
    char *bc = tf_compile("$progress(100,100,10,x,=)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "=========x"));
}

TEST_F(TitleFormattingTests, test_ProgressPos50Range100Len10_ReturnsBar10CharsWithKnobAt5) {
    char *bc = tf_compile("$progress(50,100,10,x,=)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "=====x===="));
}

TEST_F(TitleFormattingTests, test_ProgressRange0_ReturnsEmpty) {
    char *bc = tf_compile("$progress(5,0,10,x,=)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "=========x"));
}

TEST_F(TitleFormattingTests, test_ProgressLen0_ReturnsEmpty) {
    char *bc = tf_compile("$progress(5,100,0,x,=)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, ""));
}

TEST_F(TitleFormattingTests, test_ProgressCharEmpty_ReturnsEmpty) {
    char *bc = tf_compile("$progress(5,100,0,x,)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, ""));
}

TEST_F(TitleFormattingTests, test_ProgressKnobEmpty_ReturnsEmpty) {
    char *bc = tf_compile("$progress(5,100,0,,=)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, ""));
}

TEST_F(TitleFormattingTests, test_ProgressRangeEmpty_ReturnsEmpty) {
    char *bc = tf_compile("$progress(5,,0,x,=)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, ""));
}

TEST_F(TitleFormattingTests, test_ProgressAllArgsEmpty_ReturnsEmpty) {
    char *bc = tf_compile("$progress(,,,,)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, ""));
}

TEST_F(TitleFormattingTests, test_Progress2Pos3Range5Len5WithUnicodeChars_ReturnsBar5CharsWithKnobAt3) {
    char *bc = tf_compile("$progress2(3,5,5,★,☆)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "★★★☆☆"));
}

TEST_F(TitleFormattingTests, test_Progress2Pos3Range5Len5WithMultpleUnicodeChars_ReturnsBar5CharsWithKnobAt3) {
    char *bc = tf_compile("$progress2(3,5,5,⏣⌽,⚙︎⌬)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "⏣⌽⏣⌽⏣⌽⚙︎⌬⚙︎⌬"));
}


TEST_F(TitleFormattingTests, test_Progress2Pos0Range100Len10_ReturnsBar10CharsWithKnobAt0) {
    char *bc = tf_compile("$progress2(0,100,10,x,=)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "=========="));
}

TEST_F(TitleFormattingTests, test_Progress2Pos100Range100Len10_ReturnsBar10CharsWithKnobAt9) {
    char *bc = tf_compile("$progress2(100,100,10,x,=)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "xxxxxxxxxx"));
}

TEST_F(TitleFormattingTests, test_Progress2Pos50Range100Len10_ReturnsBar10CharsWithKnobAt5) {
    char *bc = tf_compile("$progress2(50,100,10,x,=)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "xxxxx====="));
}

TEST_F(TitleFormattingTests, test_Progress2Range0_ReturnsEmpty) {
    char *bc = tf_compile("$progress2(5,0,10,x,=)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "xxxxxxxxxx"));
}

TEST_F(TitleFormattingTests, test_Progress2Len0_ReturnsEmpty) {
    char *bc = tf_compile("$progress2(5,100,0,x,=)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, ""));
}

TEST_F(TitleFormattingTests, test_Progress2CharEmpty_ReturnsEmpty) {
    char *bc = tf_compile("$progress2(5,100,0,x,)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, ""));
}

TEST_F(TitleFormattingTests, test_Progress2KnobEmpty_ReturnsEmpty) {
    char *bc = tf_compile("$progress2(5,100,0,,=)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, ""));
}

TEST_F(TitleFormattingTests, test_Progress2RangeEmpty_ReturnsEmpty) {
    char *bc = tf_compile("$progress2(5,,0,x,=)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, ""));
}

TEST_F(TitleFormattingTests, test_Progress2AllArgsEmpty_ReturnsEmpty) {
    char *bc = tf_compile("$progress2(,,,,)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, ""));
}

TEST_F(TitleFormattingTests, test_Right5Chars_ReturnsLast5Chars) {
    char *bc = tf_compile("$right(ABCDE12345,5)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "12345"));
}

TEST_F(TitleFormattingTests, test_Right5CharsShortStr_ReturnsWholeStr) {
    char *bc = tf_compile("$right(ABC,5)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "ABC"));
}

TEST_F(TitleFormattingTests, test_Roman1_ReturnsI) {
    char *bc = tf_compile("$roman(1)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "I"));
}

TEST_F(TitleFormattingTests, test_Roman5_ReturnsV) {
    char *bc = tf_compile("$roman(5)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "V"));
}

TEST_F(TitleFormattingTests, test_Roman10_ReturnsX) {
    char *bc = tf_compile("$roman(10)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "X"));
}


TEST_F(TitleFormattingTests, test_Roman100500_ReturnsEmpty) {
    char *bc = tf_compile("$roman(100500)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, ""));
}

TEST_F(TitleFormattingTests, test_Roman51880_Returns_MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMDCCCLXXX) {
    char *bc = tf_compile("$roman(51880)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMDCCCLXXX"));
}

TEST_F(TitleFormattingTests, test_Rot13DeaDBeeF12345_ReturnsQrnQOrrS12345) {
    char *bc = tf_compile("$rot13(DeaDBeeF12345)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "QrnQOrrS12345"));
}

TEST_F(TitleFormattingTests, test_StrchrDeaDBeeF_B_Returns5) {
    char *bc = tf_compile("$strchr(DeaDBeeF,B)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "5"));
}

TEST_F(TitleFormattingTests, test_StrchrDeaDBeeF_R_Returns0) {
    char *bc = tf_compile("$strchr(DeaDBeeF,R)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "0"));
}

TEST_F(TitleFormattingTests, test_StrrchrDeaDBeeF_B_Returns4) {
    char *bc = tf_compile("$strrchr(DeaDBeeF,D)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "4"));
}

TEST_F(TitleFormattingTests, test_StrrchrDeaDBeeF_R_Returns0) {
    char *bc = tf_compile("$strrchr(DeaDBeeF,R)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "0"));
}

TEST_F(TitleFormattingTests, test_NestingDirectoryInSubstr_HasNoIntermediateTruncation) {
    pl_replace_meta (it, ":URI", "/media/Icy/Music/Long/Folder/Structure/2019.01.01 [Meta1] Meta2 [Meta3] Meta4 [Meta5] Meta6 [Meta7] Meta8 [Meta9]/some_reasonably_long_path.flac");
    char *bc = tf_compile("$substr($directory(%path%,1),1,$sub($strstr($directory(%path%,1),' ['),1))");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free(bc);
    EXPECT_TRUE(!strcmp (buffer, "2019.01.01"));
}

TEST_F(TitleFormattingTests, test_Tab_ProducesTabChar) {
    ctx.flags = DDB_TF_CONTEXT_MULTILINE;
    char *bc = tf_compile("$tab()");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "\t"));
}

TEST_F(TitleFormattingTests, test_Tab5_Produces5TabChars) {
    ctx.flags = DDB_TF_CONTEXT_MULTILINE;
    char *bc = tf_compile("$tab(5)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "\t\t\t\t\t"));
}

TEST_F(TitleFormattingTests, test_TrimNoLeadingTrailingSpaces_ReturnsOriginal) {
    char *bc = tf_compile("$trim(hello)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "hello"));
}

TEST_F(TitleFormattingTests, test_TrimLeadingSpaces_ReturnsTrimmedString) {
    char *bc = tf_compile("$trim(   hello)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "hello"));
}

TEST_F(TitleFormattingTests, test_TrimTrailingSpaces_ReturnsTrimmedString) {
    char *bc = tf_compile("$trim(hello   )");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "hello"));
}

TEST_F(TitleFormattingTests, test_TrimLeadingAndTrailingSpaces_ReturnsTrimmedString) {
    char *bc = tf_compile("$trim(    hello   )");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "hello"));
}

TEST_F(TitleFormattingTests, test_TrimLeadingAndTrailingSpacesWithTabs_ReturnsTrimmedToTabsString) {
    ctx.flags = DDB_TF_CONTEXT_MULTILINE;
    char *bc = tf_compile("$trim( \t   hello  \t )");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "\t   hello  \t"));
}

#pragma mark - Tint

TEST_F(TitleFormattingTests, test_CalculateTintFromString_LeadingTint_Valid) {
    char str[] = "\0331;1mhello";

    tint_stop_t tintStops[100];

    char *output;
    unsigned count = calculate_tint_stops_from_string (str, tintStops, 100, &output);

    EXPECT_EQ (count, 1);
    EXPECT_TRUE(!strcmp(output,"hello"));
    EXPECT_EQ(tintStops[0].tint, 1);
    EXPECT_EQ(tintStops[0].index, 0);

    free (output);
}

TEST_F(TitleFormattingTests, test_CalculateTintFromString_TrailingTint_Valid) {
    char str[] = "hello\0331;1m";

    tint_stop_t tintStops[100];

    char *output;
    unsigned count = calculate_tint_stops_from_string (str, tintStops, 100, &output);

    EXPECT_EQ (count, 1);
    EXPECT_TRUE(!strcmp(output,"hello"));
    EXPECT_EQ(tintStops[0].tint, 1);
    EXPECT_EQ(tintStops[0].index, 5);

    free (output);
}

TEST_F(TitleFormattingTests, test_CalculateTintFromString_MultipleTintLeadingTrailing_Valid) {
    char str[] = "\0331;-1mhello\0331;1m";

    tint_stop_t tintStops[100];

    char *output;
    unsigned count = calculate_tint_stops_from_string (str, tintStops, 100, &output);

    EXPECT_EQ (count, 2);
    EXPECT_TRUE(!strcmp(output,"hello"));
    EXPECT_EQ(tintStops[0].tint, -1);
    EXPECT_EQ(tintStops[0].index, 0);
    EXPECT_EQ(tintStops[1].tint, 0);
    EXPECT_EQ(tintStops[1].index, 5);

    free (output);
}

TEST_F(TitleFormattingTests, test_CalculateTintFromString_MultipleTintMiddle_Valid) {
    char str[] = "Leading\0331;-5mMiddle\0331;5mTrailing";

    tint_stop_t tintStops[100];

    char *output;
    unsigned count = calculate_tint_stops_from_string (str, tintStops, 100, &output);

    EXPECT_EQ (count, 2);
    EXPECT_TRUE(!strcmp(output,"LeadingMiddleTrailing"));
    EXPECT_EQ(tintStops[0].tint, -5);
    EXPECT_EQ(tintStops[0].index, 7);
    EXPECT_EQ(tintStops[1].tint, 0);
    EXPECT_EQ(tintStops[1].index, 13);

    free (output);
}

#pragma mark - RGB

TEST_F(TitleFormattingTests, test_CalculateRGBFromString_LeadingRGB_Valid) {
    char str[] = "\0332;20;30;40mhello";

    tint_stop_t tintStops[100];

    char *output;
    unsigned count = calculate_tint_stops_from_string (str, tintStops, 100, &output);

    EXPECT_EQ (count, 1);
    EXPECT_TRUE(!strcmp(output,"hello"));
    EXPECT_EQ(tintStops[0].has_rgb, 1);
    EXPECT_EQ(tintStops[0].r, 20);
    EXPECT_EQ(tintStops[0].g, 30);
    EXPECT_EQ(tintStops[0].b, 40);

    free (output);
}

TEST_F(TitleFormattingTests, test_CalculateRGBFromString_Unterminated_Invalid) {
    char str[] = "\0332;20;30;40hello";

    tint_stop_t tintStops[100];

    char *output;
    unsigned count = calculate_tint_stops_from_string (str, tintStops, 100, &output);

    EXPECT_EQ (count, 0);
    EXPECT_TRUE(!strcmp(output,"\0332;20;30;40hello"));

    free (output);
}

TEST_F(TitleFormattingTests, test_CalculateRGBFromString_Negative_Reset) {
    char str[] = "\0332;-20;30;-40mhello";

    tint_stop_t tintStops[100];

    char *output;
    unsigned count = calculate_tint_stops_from_string (str, tintStops, 100, &output);

    EXPECT_EQ (count, 1);
    EXPECT_EQ(tintStops[0].has_rgb, 0);
    EXPECT_TRUE(!strcmp(output,"hello"));

    free (output);
}

TEST_F(TitleFormattingTests, test_CalculateRGBFromString_LargerThan255_Clamped) {
    char str[] = "\0332;1000;30;2000mhello";

    tint_stop_t tintStops[100];

    char *output;
    unsigned count = calculate_tint_stops_from_string (str, tintStops, 100, &output);

    EXPECT_EQ (count, 1);
    EXPECT_TRUE(!strcmp(output,"hello"));
    EXPECT_EQ(tintStops[0].has_rgb, 1);
    EXPECT_EQ(tintStops[0].r, 255);
    EXPECT_EQ(tintStops[0].g, 30);
    EXPECT_EQ(tintStops[0].b, 255);

    free (output);
}

TEST_F(TitleFormattingTests, test_CalculateRGBFromString_TrailingRGB_Valid) {
    char str[] = "hello\0332;20;30;40m";

    tint_stop_t tintStops[100];

    char *output;
    unsigned count = calculate_tint_stops_from_string (str, tintStops, 100, &output);

    EXPECT_EQ (count, 1);
    EXPECT_TRUE(!strcmp(output,"hello"));
    EXPECT_EQ(tintStops[0].has_rgb, 1);
    EXPECT_EQ(tintStops[0].r, 20);
    EXPECT_EQ(tintStops[0].g, 30);
    EXPECT_EQ(tintStops[0].b, 40);

    free (output);
}

TEST_F(TitleFormattingTests, test_CalculateRGBFromString_MultipleRGBLeadingTrailing_Valid) {
    char str[] = "\0332;20;30;40mhello\0332;50;60;70m";

    tint_stop_t tintStops[100];

    char *output;
    unsigned count = calculate_tint_stops_from_string (str, tintStops, 100, &output);

    EXPECT_EQ (count, 2);
    EXPECT_TRUE(!strcmp(output,"hello"));
    EXPECT_EQ(tintStops[0].has_rgb, 1);
    EXPECT_EQ(tintStops[0].r, 20);
    EXPECT_EQ(tintStops[0].g, 30);
    EXPECT_EQ(tintStops[0].b, 40);
    EXPECT_EQ(tintStops[1].has_rgb, 1);
    EXPECT_EQ(tintStops[1].r, 50);
    EXPECT_EQ(tintStops[1].g, 60);
    EXPECT_EQ(tintStops[1].b, 70);

    free (output);
}

TEST_F(TitleFormattingTests, test_CalculateRGBFromString_MultipleRGBMiddle_Valid) {
    char str[] = "Leading\0332;20;30;40mMiddle\0332;50;60;70mTrailing";

    tint_stop_t tintStops[100];

    char *output;
    unsigned count = calculate_tint_stops_from_string (str, tintStops, 100, &output);

    EXPECT_EQ (count, 2);
    EXPECT_TRUE(!strcmp(output,"LeadingMiddleTrailing"));
    EXPECT_EQ(tintStops[0].has_rgb, 1);
    EXPECT_EQ(tintStops[0].r, 20);
    EXPECT_EQ(tintStops[0].g, 30);
    EXPECT_EQ(tintStops[0].b, 40);
    EXPECT_EQ(tintStops[1].has_rgb, 1);
    EXPECT_EQ(tintStops[1].r, 50);
    EXPECT_EQ(tintStops[1].g, 60);
    EXPECT_EQ(tintStops[1].b, 70);

    free (output);
}

#pragma mark - Tint + RGB

TEST_F(TitleFormattingTests, test_CalculateRGBFromString_TintWithRGB_Valid) {
    char str[] = "\0331;-3m\0332;20;30;40mHello";

    tint_stop_t tintStops[100];

    char *output;
    unsigned count = calculate_tint_stops_from_string (str, tintStops, 100, &output);

    EXPECT_EQ (count, 2);
    EXPECT_TRUE(!strcmp(output,"Hello"));
    EXPECT_EQ(tintStops[0].has_rgb, 0);
    EXPECT_EQ(tintStops[0].tint, -3);
    EXPECT_EQ(tintStops[1].has_rgb, 1);
    EXPECT_EQ(tintStops[1].r, 20);
    EXPECT_EQ(tintStops[1].g, 30);
    EXPECT_EQ(tintStops[1].b, 40);

    free (output);
}

TEST_F(TitleFormattingTests, test_year_empty_returnNothing) {
    char *bc = tf_compile("$year()");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, ""));
}

TEST_F(TitleFormattingTests, test_year_text_returnNothing) {
    char *bc = tf_compile("$year(abcd)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, ""));
}

TEST_F(TitleFormattingTests, test_year_shorterThan4_returnNothing) {
    char *bc = tf_compile("$year(123)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, ""));
}

TEST_F(TitleFormattingTests, test_year_Exactly4_returnValue) {
    char *bc = tf_compile("$year(1234)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "1234"));
}

TEST_F(TitleFormattingTests, test_year_LongerThan4WithText_returnYear) {
    char *bc = tf_compile("$year(9999text)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_TRUE(!strcmp (buffer, "9999"));
}

TEST_F(TitleFormattingTests, test_itematindex_0_returnsFirstItem) {
    char *bc = tf_compile("$itematindex(0,%artist%)");

    char value[] = "value1\0value2";
    pl_add_meta_full(it, "artist", value, sizeof(value));

    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_STREQ(buffer, "value1");
}

TEST_F(TitleFormattingTests, test_itematindex_1_returnsSecondItem) {
    char *bc = tf_compile("$itematindex(1,%artist%)");

    char value[] = "value1\0value2";
    pl_add_meta_full(it, "artist", value, sizeof(value));

    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_STREQ(buffer, "value2");
}

TEST_F(TitleFormattingTests, test_itematindex_outOfBounds_returnsEmpty) {
    char *bc = tf_compile("$itematindex(2,%artist%)");

    char value[] = "value1\0value2";
    pl_add_meta_full(it, "artist", value, sizeof(value));

    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_STREQ(buffer, "");
}

TEST_F(TitleFormattingTests, test_itematindex_singleValue_returnsValue) {
    char *bc = tf_compile("$itematindex(2,%artist%)");

    char value[] = "value1";
    pl_add_meta_full(it, "artist", value, sizeof(value));

    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_STREQ(buffer, "value1");
}

TEST_F(TitleFormattingTests, test_itematindex_NoArguments_returnsEmpty) {
    char *bc = tf_compile("$itematindex()");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_STREQ(buffer, "");
}

TEST_F(TitleFormattingTests, test_itematindex_1Arguments_returnsEmpty) {
    char *bc = tf_compile("$itematindex(0)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_STREQ(buffer, "");
}

TEST_F(TitleFormattingTests, test_itematindex_3Arguments_returnsEmpty) {
    char *bc = tf_compile("$itematindex(0,1,2)");
    tf_eval (&ctx, bc, buffer, 1000);
    tf_free (bc);
    EXPECT_STREQ(buffer, "");
}

TEST_F(TitleFormattingTests, test_meta_bufferTooShortWithMultibyteCharsInput_returnsOnlyWholeMultibyteChars) {
    char *bc = tf_compile("$meta(comment)");
    pl_add_meta(it, "comment", "ΘΘΘΘ");
    tf_eval (&ctx, bc, buffer, 8);
    tf_free (bc);
    EXPECT_STREQ(buffer, "ΘΘΘ");
}
