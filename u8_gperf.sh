#!/bin/sh
gperf -c -t -H u8_lc_hash -N u8_lc_in_word_set u8_lc_map.txt >u8_lc_map.h
