#!/usr/bin/perl

@lst = qw(16 22 24 32 36 48 64 72 96 128 192 256);
foreach $i (@lst) {
    $size = $i-2;
    $size = $size.'x'.$size;
    $dir = $i.'x'.$i;
    `mkdir $dir`;
    `convert -density 100 -background None scalable/deadbeef.svg -bordercolor None -support 0.1 -resize $size -border 1x1 $dir/deadbeef.png`;
}
