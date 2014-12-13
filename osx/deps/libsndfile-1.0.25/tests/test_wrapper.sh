#!/bin/sh

# Copyright (C) 2008-2011 Erik de Castro Lopo <erikd@mega-nerd.com>
#
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in
#       the documentation and/or other materials provided with the
#       distribution.
#     * Neither the author nor the names of any contributors may be used
#       to endorse or promote products derived from this software without
#       specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


HOST_TRIPLET=i386-apple-darwin12.4.0
PACKAGE_VERSION=1.0.25
LIB_VERSION=`echo $PACKAGE_VERSION | sed "s/[a-z].*//"`

if test -f tests/sfversion ; then
	cd tests
	fi

if test ! -f sfversion ; then
	echo "Not able to find test executables."
	exit 1
	fi

if test -f libsndfile.so.$LIB_VERSION ; then
	# This will work on Linux, but not on Mac.
	# Windows is already sorted out.
	export LD_LIBRARY_PATH=`pwd`
	if test ! -f libsndfile.so.1 ; then
		ln -s libsndfile.so.$LIB_VERSION libsndfile.so.1
		fi
	fi

sfversion=`./sfversion | sed "s/-exp$//"`

if test $sfversion != libsndfile-$PACKAGE_VERSION ; then
	echo "Error : sfversion ($sfversion) and PACKAGE_VERSION ($PACKAGE_VERSION) don't match."
	exit 1
	fi

# Force exit on errors.
set -e

# Generic-tests
uname -a

# Check the header file.
sh pedantic-header-test.sh

# Need this for when we're running from files collected into the
# libsndfile-testsuite-1.0.25 tarball.
if test -x test_main ; then
	echo "Running unit tests from src/ directory of source code tree."
	./test_main
	echo
	echo "Running end-to-end tests from tests/ directory."
	fi

./error_test
./pcm_test
./ulaw_test
./alaw_test
./dwvw_test
./command_test ver
./command_test norm
./command_test format
./command_test peak
./command_test trunc
./command_test inst
./command_test current_sf_info
./command_test bext
./command_test bextch
./command_test chanmap
./floating_point_test
./checksum_test
./scale_clip_test
./headerless_test
./rdwr_test
./locale_test
./win32_ordinal_test
./external_libs_test
./format_check_test

# The w64 G++ compiler requires an extra runtime DLL which we don't have,
# so skip this test.
case "$HOST_TRIPLET" in
	x86_64-w64-mingw32)
		;;
	i686-w64-mingw32)
		;;
	*)
		./cpp_test
		;;
	esac

echo "----------------------------------------------------------------------"
echo "  $sfversion passed common tests."
echo "----------------------------------------------------------------------"

# aiff-tests
./write_read_test aiff
./lossy_comp_test aiff_ulaw
./lossy_comp_test aiff_alaw
./lossy_comp_test aiff_gsm610
echo "=========================="
echo "./lossy_comp_test aiff_ima"
echo "=========================="
./peak_chunk_test aiff
./header_test aiff
./misc_test aiff
./string_test aiff
./multi_file_test aiff
./aiff_rw_test
echo "----------------------------------------------------------------------"
echo "  $sfversion passed tests on AIFF files."
echo "----------------------------------------------------------------------"

# au-tests
./write_read_test au
./lossy_comp_test au_ulaw
./lossy_comp_test au_alaw
./lossy_comp_test au_g721
./lossy_comp_test au_g723
./header_test au
./misc_test au
./multi_file_test au
echo "----------------------------------------------------------------------"
echo "  $sfversion passed tests on AU files."
echo "----------------------------------------------------------------------"

# caf-tests
./write_read_test caf
./lossy_comp_test caf_ulaw
./lossy_comp_test caf_alaw
./header_test caf
./peak_chunk_test caf
./misc_test caf
echo "----------------------------------------------------------------------"
echo "  $sfversion passed tests on CAF files."
echo "----------------------------------------------------------------------"

# wav-tests
./write_read_test wav
./lossy_comp_test wav_pcm
./lossy_comp_test wav_ima
./lossy_comp_test wav_msadpcm
./lossy_comp_test wav_ulaw
./lossy_comp_test wav_alaw
./lossy_comp_test wav_gsm610
./lossy_comp_test wav_g721
./peak_chunk_test wav
./header_test wav
./misc_test wav
./string_test wav
./multi_file_test wav
echo "----------------------------------------------------------------------"
echo "  $sfversion passed tests on WAV files."
echo "----------------------------------------------------------------------"

# w64-tests
./write_read_test w64
./lossy_comp_test w64_ima
./lossy_comp_test w64_msadpcm
./lossy_comp_test w64_ulaw
./lossy_comp_test w64_alaw
./lossy_comp_test w64_gsm610
./header_test w64
./misc_test w64
echo "----------------------------------------------------------------------"
echo "  $sfversion passed tests on W64 files."
echo "----------------------------------------------------------------------"

# rf64-tests
./write_read_test rf64
./header_test rf64
./misc_test rf64
echo "----------------------------------------------------------------------"
echo "  $sfversion passed tests on RF64 files."
echo "----------------------------------------------------------------------"

# raw-tests
./write_read_test raw
./lossy_comp_test raw_ulaw
./lossy_comp_test raw_alaw
./lossy_comp_test raw_gsm610
./lossy_comp_test vox_adpcm
./raw_test
echo "----------------------------------------------------------------------"
echo "  $sfversion passed tests on RAW (header-less) files."
echo "----------------------------------------------------------------------"

# paf-tests
./write_read_test paf
./header_test paf
./misc_test paf
echo "----------------------------------------------------------------------"
echo "  $sfversion passed tests on PAF files."
echo "----------------------------------------------------------------------"

# svx-tests
./write_read_test svx
./header_test svx
./misc_test svx
echo "----------------------------------------------------------------------"
echo "  $sfversion passed tests on SVX files."
echo "----------------------------------------------------------------------"

# nist-tests
./write_read_test nist
./lossy_comp_test nist_ulaw
./lossy_comp_test nist_alaw
./header_test nist
./misc_test nist
echo "----------------------------------------------------------------------"
echo "  $sfversion passed tests on NIST files."
echo "----------------------------------------------------------------------"

# ircam-tests
./write_read_test ircam
./lossy_comp_test ircam_ulaw
./lossy_comp_test ircam_alaw
./header_test ircam
./misc_test ircam
echo "----------------------------------------------------------------------"
echo "  $sfversion passed tests on IRCAM files."
echo "----------------------------------------------------------------------"

# voc-tests
./write_read_test voc
./lossy_comp_test voc_ulaw
./lossy_comp_test voc_alaw
./header_test voc
./misc_test voc
echo "----------------------------------------------------------------------"
echo "  $sfversion passed tests on VOC files."
echo "----------------------------------------------------------------------"

# mat4-tests
./write_read_test mat4
./header_test mat4
./misc_test mat4
echo "----------------------------------------------------------------------"
echo "  $sfversion passed tests on MAT4 files."
echo "----------------------------------------------------------------------"

# mat5-tests
./write_read_test mat5
./header_test mat5
./misc_test mat5
echo "----------------------------------------------------------------------"
echo "  $sfversion passed tests on MAT5 files."
echo "----------------------------------------------------------------------"

# pvf-tests
./write_read_test pvf
./header_test pvf
./misc_test pvf
echo "----------------------------------------------------------------------"
echo "  $sfversion passed tests on PVF files."
echo "----------------------------------------------------------------------"

# xi-tests
./lossy_comp_test xi_dpcm
echo "----------------------------------------------------------------------"
echo "  $sfversion passed tests on XI files."
echo "----------------------------------------------------------------------"

# htk-tests
./write_read_test htk
./header_test htk
./misc_test htk
echo "----------------------------------------------------------------------"
echo "  $sfversion passed tests on HTK files."
echo "----------------------------------------------------------------------"

# avr-tests
./write_read_test avr
./header_test avr
./misc_test avr
echo "----------------------------------------------------------------------"
echo "  $sfversion passed tests on AVR files."
echo "----------------------------------------------------------------------"

# sds-tests
./write_read_test sds
./header_test sds
./misc_test sds
echo "----------------------------------------------------------------------"
echo "  $sfversion passed tests on SDS files."
echo "----------------------------------------------------------------------"

# sd2-tests
./write_read_test sd2
echo "----------------------------------------------------------------------"
echo "  $sfversion passed tests on SD2 files."
echo "----------------------------------------------------------------------"

# wve-tests
./lossy_comp_test wve
echo "----------------------------------------------------------------------"
echo "  $sfversion passed tests on WVE files."
echo "----------------------------------------------------------------------"

# mpc2k-tests
./write_read_test mpc2k
./header_test mpc2k
./misc_test mpc2k
echo "----------------------------------------------------------------------"
echo "  $sfversion passed tests on MPC 2000 files."
echo "----------------------------------------------------------------------"

# flac-tests
./write_read_test flac
./string_test flac
echo "----------------------------------------------------------------------"
echo "  $sfversion passed tests on FLAC files."
echo "----------------------------------------------------------------------"

# vorbis-tests
./ogg_test
./vorbis_test
./lossy_comp_test ogg_vorbis
./string_test ogg
./misc_test ogg
echo "----------------------------------------------------------------------"
echo "  $sfversion passed tests on OGG/VORBIS files."
echo "----------------------------------------------------------------------"

# io-tests
./stdio_test
./pipe_test
./virtual_io_test
echo "----------------------------------------------------------------------"
echo "  $sfversion passed stdio/pipe/vio tests."
echo "----------------------------------------------------------------------"


