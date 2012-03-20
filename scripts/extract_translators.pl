#!/usr/bin/env perl

use strict;
use Locale::Language;
use Locale::Country;

my @files = split /\s+/, `ls po/*.po`;
my @files = sort {$a cmp $b} @files;

foreach my $f (@files) {
    open F, "<$f";

    my $code = $f;
    $code =~ s/^po\/(.*)\.po/$1/g;

    my $script;
    if ($code =~ /(.*)@(.*)/) {
        $code = $1;
        $script = $2;
    }
    
    my $country;
    if ($code =~ /(.*)_(.*)/) {
        $code = $1;
        $country = code2country ($2);
    }

    my $lang = code2language($code);
    #print "$code\n";
    print "$lang";
    if ($country) {
        print " ($country)";
    }
    if ($script) {
        print " ($script)";
    }
    print " $code\n";
    while (<F>) {
        if (/^#/) {
            if (/\@/ && !(/Yakovenko/) && !(/EMAIL/)) {
                s/^#//;
                s/, [0-9]+\.$//g;
                chomp;
                print "$_\n";
            }
        }
        else {
            last;
        }
    }
    print "\n";

    close F;
}
