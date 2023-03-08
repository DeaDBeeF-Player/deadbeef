//
//  MP3ParserTest.m
//  Tests
//
//  Created by Oleksiy Yakovenko on 6/14/18.
//  Copyright © 2018 Oleksiy Yakovenko. All rights reserved.
//

#include "../plugins/mp3/mp3parser.h"
#include <deadbeef/common.h>
#include "vfs.h"
#include <gtest/gtest.h>

#define EXPECT_EQ_WITH_ACCURACY(a,b,accuracy) EXPECT_LT(std::abs(a-b), accuracy)

TEST(MP3ParserTests, test_2secSquareWithLameHeader_XingDetectedWith88200Samples) {
    mp3info_t info;
    char path[PATH_MAX];
    snprintf (path, sizeof (path), "%s/TestData/mp3parser/2sec-square-lamehdr.mp3", dbplugindir);
    DB_FILE *fp = vfs_fopen (path);
    int64_t fsize = vfs_fgetlength(fp);
    int res = mp3_parse_file (&info, 0, fp, fsize, 0, 0, -1);
    EXPECT_TRUE(!res);
    EXPECT_EQ(info.have_xing_header, 1);
    EXPECT_EQ(info.ref_packet.samplerate, 44100);
    EXPECT_EQ(info.totalsamples-info.delay-info.padding, 88200);
    EXPECT_EQ(info.delay, 1105);
    EXPECT_EQ(info.padding, 551);
    EXPECT_EQ(info.lame_musiclength, 16508);
}

TEST(MP3ParserTests, test_2secSquareWithLameHeader_FullScanGet88200Samples) {
    mp3info_t info;
    char path[PATH_MAX];
    snprintf (path, sizeof (path), "%s/TestData/mp3parser/2sec-square-lamehdr.mp3", dbplugindir);
    DB_FILE *fp = vfs_fopen (path);
    int64_t fsize = vfs_fgetlength(fp);
    int res = mp3_parse_file (&info, MP3_PARSE_FULLSCAN, fp, fsize, 0, 0, -1);
    EXPECT_TRUE(!res);
    EXPECT_EQ(info.have_xing_header, 1);
    EXPECT_EQ(info.ref_packet.samplerate, 44100);
    EXPECT_EQ(info.totalsamples, 88200+576+1080);
    EXPECT_EQ(info.pcmsample, 0);
    EXPECT_EQ(info.delay, 529);
    EXPECT_EQ(info.padding, 0);
    EXPECT_EQ(info.lame_musiclength, 16508);
}

TEST(MP3ParserTests, test_2secSquareSeekTo0_GivesPacketAfterXingPos208) {
    mp3info_t info;
    char path[PATH_MAX];
    snprintf (path, sizeof (path), "%s/TestData/mp3parser/2sec-square-lamehdr.mp3", dbplugindir);
    DB_FILE *fp = vfs_fopen (path);
    int64_t fsize = vfs_fgetlength(fp);
    int res = mp3_parse_file (&info, 0, fp, fsize, 0, 0, 0);
    EXPECT_TRUE(!res);
    EXPECT_EQ(info.packet_offs, 208);
}

TEST(MP3ParserTests, test_2secSquareSeekToDelay_GivesPacketAfterXingPos208) {
    mp3info_t info;
    char path[PATH_MAX];
    snprintf (path, sizeof (path), "%s/TestData/mp3parser/2sec-square-lamehdr.mp3", dbplugindir);
    DB_FILE *fp = vfs_fopen (path);
    int64_t fsize = vfs_fgetlength(fp);
    int res = mp3_parse_file (&info, 0, fp, fsize, 0, 0, 576);
    EXPECT_TRUE(!res);
    EXPECT_EQ(info.packet_offs, 208);
}

TEST(MP3ParserTests, test_2secSquareSeekTo1sec_SeeksTo1SecMinus10Packets) {
    mp3info_t info;
    char path[PATH_MAX];
    snprintf (path, sizeof (path), "%s/TestData/mp3parser/2sec-square-lamehdr.mp3", dbplugindir);
    DB_FILE *fp = vfs_fopen (path);
    int64_t fsize = vfs_fgetlength(fp);
    int res = mp3_parse_file (&info, 0, fp, fsize, 0, 0, 576+44100);
    EXPECT_TRUE(!res);
    EXPECT_EQ(info.packet_offs, 6059);
    EXPECT_EQ(info.pcmsample, 32256);
}

TEST(MP3ParserTests, test_2secSquareNoLameHeader_Gives88200Samples) {
    mp3info_t info;
    char path[PATH_MAX];
    snprintf (path, sizeof (path), "%s/TestData/mp3parser/2sec-square-nolamehdr.mp3", dbplugindir);
    DB_FILE *fp = vfs_fopen (path);
    int64_t fsize = vfs_fgetlength(fp);
    int res = mp3_parse_file (&info, 0, fp, fsize, 0, 0, -1);
    EXPECT_TRUE(!res);
    EXPECT_EQ(info.have_xing_header, 0);
    EXPECT_EQ(info.ref_packet.samplerate, 44100);
    // lame adds default encoder delay and padding of 576 and 1080, even without header
    EXPECT_EQ(info.totalsamples, 88200+576+1080);
    EXPECT_EQ(info.pcmsample, 0);
    EXPECT_EQ(info.delay, 529);
    EXPECT_EQ(info.padding, 0);
}

TEST(MP3ParserTests, test_2secSquareNoXHSeekTo1sec_SeeksTo1SecMinus10Packets) {
    mp3info_t info;
    char path[PATH_MAX];
    snprintf (path, sizeof (path), "%s/TestData/mp3parser/2sec-square-nolamehdr.mp3", dbplugindir);
    DB_FILE *fp = vfs_fopen (path);
    int64_t fsize = vfs_fgetlength(fp);
    int res = mp3_parse_file (&info, 0, fp, fsize, 0, 0, 576+44100);
    EXPECT_TRUE(!res);
    EXPECT_EQ(info.have_xing_header, 0);
    EXPECT_EQ(info.packet_offs, 5851);
    EXPECT_EQ(info.pcmsample, 32256);
}

// the file contains garbage/invalid data around the middle of the file, with packet markers.
// we still expect the parser to deal with it
TEST(MP3ParserTests, test_2secSquareWithGarbage_Reports88200SamplesLength) {
    mp3info_t info;
    char path[PATH_MAX];
    snprintf (path, sizeof (path), "%s/TestData/mp3parser/2sec-square-nolamehdr-garbage.mp3", dbplugindir);
    DB_FILE *fp = vfs_fopen (path);
    int64_t fsize = vfs_fgetlength(fp);
    int res = mp3_parse_file (&info, 0, fp, fsize, 0, 0, -1);
    EXPECT_TRUE(!res);
    EXPECT_EQ(info.have_xing_header, 0);
    EXPECT_EQ(info.ref_packet.samplerate, 44100);
    // lame adds default encoder delay and padding of 576 and 1080, even without header
    EXPECT_EQ(info.totalsamples, 88200+576+1080);
    EXPECT_EQ(info.pcmsample, 0);
    EXPECT_EQ(info.delay, 529);
    EXPECT_EQ(info.padding, 0);
}

TEST(MP3ParserTests, test_FiniteCBRNetworkStream_ReportsAccurateDuration) {
    mp3info_t info;
    char path[PATH_MAX];
    snprintf (path, sizeof (path), "%s/TestData/mp3parser/cbr_rhytm_30sec.mp3", dbplugindir);
    DB_FILE *fp = vfs_fopen (path);
    int64_t fsize = vfs_fgetlength(fp);
    int res = mp3_parse_file (&info, MP3_PARSE_ESTIMATE_DURATION, fp, fsize, 0, 0, -1);
    EXPECT_TRUE(!res);
    EXPECT_EQ(info.have_xing_header, 0);
    EXPECT_EQ(info.ref_packet.samplerate, 32000);
    EXPECT_EQ(info.totalsamples, 1025280);
    EXPECT_EQ(info.npackets, 890);
    EXPECT_LT(info.valid_packets, info.npackets);
}

TEST(MP3ParserTests, test_FiniteCBRLameHdrNetworkStream_ReportsAccurateDuration) {
    mp3info_t info;
    char path[PATH_MAX];
    snprintf (path, sizeof (path), "%s/TestData/mp3parser/cbr_rhytm_30sec_lamehdr.mp3", dbplugindir);
    DB_FILE *fp = vfs_fopen (path);
    int64_t fsize = vfs_fgetlength(fp);
    int res = mp3_parse_file (&info, MP3_PARSE_ESTIMATE_DURATION, fp, fsize, 0, 0, -1);
    EXPECT_TRUE(!res);
    EXPECT_EQ(info.have_xing_header, 1);
    EXPECT_EQ(info.ref_packet.samplerate, 32000);
    EXPECT_EQ(info.totalsamples, 1025280);
    EXPECT_EQ(info.npackets, 890);
    EXPECT_LT(info.valid_packets, info.npackets);
}

TEST(MP3ParserTests, test_FiniteVBRNetworkStream_ReportsApproximateDuration) {
    mp3info_t info;
    char path[PATH_MAX];
    snprintf (path, sizeof (path), "%s/TestData/mp3parser/vbr_rhytm_30sec.mp3", dbplugindir);
    DB_FILE *fp = vfs_fopen (path);
    int64_t fsize = vfs_fgetlength(fp);
    int res = mp3_parse_file (&info, MP3_PARSE_ESTIMATE_DURATION, fp, fsize, 0, 0, -1);
    EXPECT_TRUE(!res);
    EXPECT_EQ(info.have_xing_header, 0);
    EXPECT_EQ(info.ref_packet.samplerate, 32000);
    EXPECT_EQ_WITH_ACCURACY(info.totalsamples, 1025280, 2000);
    EXPECT_EQ_WITH_ACCURACY(info.npackets, 890, 10);
    EXPECT_LT(info.valid_packets, info.npackets);
}

TEST(MP3ParserTests, test_FiniteVBRLameHdrNetworkStream_ReportsAccurateDuration) {
    mp3info_t info;
    char path[PATH_MAX];
    snprintf (path, sizeof (path), "%s/TestData/mp3parser/vbr_rhytm_30sec_lamehdr.mp3", dbplugindir);
    DB_FILE *fp = vfs_fopen (path);
    int64_t fsize = vfs_fgetlength(fp);
    int res = mp3_parse_file (&info, MP3_PARSE_ESTIMATE_DURATION, fp, fsize, 0, 0, -1);
    EXPECT_TRUE(!res);
    EXPECT_EQ(info.have_xing_header, 1);
    EXPECT_EQ(info.ref_packet.samplerate, 32000);
    EXPECT_EQ(info.totalsamples, 1025280);
    EXPECT_EQ_WITH_ACCURACY(info.npackets, 890, 10);
    EXPECT_LT(info.valid_packets, info.npackets);
}
