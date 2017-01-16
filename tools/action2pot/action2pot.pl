#!/usr/bin/env perl

# Find all C files, extract all configdlg and plugin_action properties/titles
# into localization files

use strict;
use warnings;
use FindBin qw'$Bin';
use lib "$FindBin::Bin/../perl_lib";
use lib "$FindBin::Bin";
use File::Find::Rule;
use Getopt::Long qw(GetOptions);

my $help;
my $android_xml;
my $c_source;

GetOptions (
    "help|?" => \$help,
    "--android-xml" => \$android_xml,
    "--c-source" => \$c_source,
) or die("Error in command line arguments\n");

if ($help) {
    print "Usage: $0 [options]\n";
    print "With no options, action2pot will generate strings.pot from deadbeef source code\n\n";
    print "Options:\n";
    print "  --help               Show this text\n";
    print "  --android-xml        Generate android xml strings.xml\n";
    print "  --c-source           Generate strings.c file compatible with xgettext\n";
    exit (0);
}

my $ddb_path = $FindBin::Bin.'/../..';

my @ignore_props = ("box");

my @lines;

for my $f (File::Find::Rule->file()->name("*.c")->in($ddb_path)) {
    open F, "<$f" or die "Failed to open $f\n";
    my $relf = substr ($f, length($ddb_path)+1);
    while (<F>) {
        # configdialog
        if (/^\s*"property\s+/) {
            my $prop;
            if (/^\s*"property\s+([a-zA-Z0-9_]+)/) {
                $prop = $1;
            }
            elsif (/^(\s*"property\s+")/) {
                my $begin = $1;
                my $s = substr ($_, length ($begin));
                if ($s =~ /(.*[^\\])"/) {
                    $prop = $1;
                }
            }
            if ($prop && !grep ({$_ eq $prop} @ignore_props)) {
                if (!grep ({$_->{msgid} eq $prop} @lines)) {
                    push @lines, { f=>$relf, line=>$., msgid=>$prop };
                }
            }
        }
        elsif (/^.*DB_plugin_action_t .* {/) {
            # read until we hit title or };
            while (<F>) {
                if (/^(\s*\.title\s*=\s*")/) {
                    my $begin = $1;
                    my $s = substr ($_, length ($begin));
                    if ($s =~ /(.*[^\\])"/) {
                        my $prop = $1;
                        if (!grep ({$_->{msgid} eq $prop} @lines)) {
                            push @lines, { f=>$relf, line=>$., msgid=>$prop };
                        }
                    }
                }
            }
        }
    }
    close F;
}

if ($android_xml) {
    open XML, '>:encoding(utf8)', 'strings.xml' or die "Failed to open strings.xml\n";
    print XML "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
    print XML "<resources>\n";
    for my $l (@lines) {
        # TODO: generate usable string id from msgid;
        # the string id must be conforming to android resource id naming rules,
        # i.e. alphanumeric+underscores, as short as possible.
        # Would be great if msgid can be converted to string id at runtime,
        # otherwise need to dump a map file alongside.
        # Could make use of gperf here.
        print XML "    <string name=\"\">$l->{msgid}</string>\n";
    }
    print XML "</resources>\n";
    close XML;
}
elsif ($c_source) {
    open C, '>:encoding(utf8)', 'strings.c' or die "Failed to open strings.c\n";
    for my $l (@lines) {
        print C "_(\"$l->{msgid}\");\n";
    }
    close C;
}
else {
    open POT, '>:encoding(utf8)', 'strings.pot' or die "Failed to open strings.pot\n";

    print POT "msgid \"\"\nmsgstr \"\"\n\"Content-Type: text/plain; charset=UTF-8\\n\"\n\"Content-Transfer-Encoding: 8bit\\n\"\n";

    for my $l (@lines) {
        print POT "\n#: $l->{f}:$l->{line}\nmsgid \"$l->{msgid}\"\nmsgstr \"\"\n";
    }
    close POT;
}
