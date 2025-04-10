package extract;
use strict;
use warnings;

use base qw(Exporter);
@extract::EXPORT = qw(extract);
use File::Find::Rule;

my @ignore_props = ("box");
my @ignore_paths_android = (
    'plugins/alsa',
    'plugins/artwork-legacy',
    'plugins/cdda',
    'plugins/cocoaui',
    'plugins/converter',
    'plugins/coreaudio',
    'plugins/dca',
    'plugins/dsp_libsrc',
    'plugins/ffmpeg',
    'plugins/gtkui',
    'plugins/hotkeys',
    'plugins/mono2stereo',
    'plugins/notify',
    'plugins/nullout',
    'plugins/oss',
    'plugins/pltbrowser',
    'plugins/psf',
    'plugins/pulse',
    'plugins/rg_scanner',
    'plugins/shellexec',
    'plugins/shellexecui',
    'plugins/shn',
    'plugins/sndio',
    'plugins/soundtouch',
    'plugins/supereq',
    'plugins/wildmidi'
);

my @ignore_values = ('mpg123','mad');

sub extract {
    my $ddb_path = shift;
    my $android_xml = shift;
    my @lines;

    my @files = sort(File::Find::Rule->file()->name("*.c")->in($ddb_path));
    for my $f (@files) {
        if ($android_xml && grep ({$f =~ /\/$_\//} @ignore_paths_android)) {
            print "skipped $f (ignore list)\n";
            next;
        }
        if ($f =~ /^\/?tools\//
            || $f =~ /^\/?examples\//
            || $f =~ /\/soundtouch\//) {
            print "skip $f\n";
            next;
        }
        print "$f\n";
        open F, "<$f" or die "Failed to open $f\n";
        my $relf = $f;#substr ($f, length($ddb_path)+1);
        while (<F>) {
            # configdialog
            my $line = $_;
            if (/^\s*"property\s+/) {
                my $prop;
                if (/^\s*"property\s+([a-zA-Z0-9_]+)/) {
                    $prop = $1;
                }
                elsif (/^(\s*"property\s+\\")/) {
                    my $begin = $1;
                    my $s = substr ($_, length ($begin));
                    if ($s =~ /(.*?)\\"/) {
                        $prop = $1;
                    }
                }
                if ($prop && !grep ({$_ eq $prop} @ignore_props)) {
                    if ($prop =~ /[A-Za-z]/ && !grep ({$_->{msgid} eq $prop} @lines)) {
                        push @lines, { f=>$relf, line=>$., msgid=>$prop };
                    }

                    # handle prop values
                    if ($line =~ /.*select\[([0-9]+)\]\s+(.*)$/) {
                        my $cnt = $1;
                        my $input = $2;
                        # get select values
                        sub next_token {
                            my $s = shift;
                            if ($$s =~ /^\s*\\"/) {
                                $$s =~ s/^\s*\\"(.*?)\\"(.*)$/$2/;
                                return $1;
                            }
                            else {
                                $$s =~ s/^\s*([\.a-zA-Z0-9_\-]+?)[ ;](.*)$/$2/;
                                return $1;
                            }
                        }
                        next_token(\$input);
                        next_token(\$input);
                        for (my $i = 0; $i < $cnt; $i++) {
                            my $val = next_token (\$input);
                            next if ($val =~ /^[0-9\.-_]*$/);
                            next if grep ({$_ eq $val} @ignore_values);
                            if ($val =~ /[A-Za-z]/ && !grep ({$_->{msgid} eq $val} @lines)) {
                                push @lines, { f=>$relf, line=>$., msgid=>$val };
                            }
                        }
                    }
                }
            }
            elsif (/^[^(]*DB_plugin_action_t .* \{?/ || /^[^(]*ddb_dialog_t .* \{?/) {
                # read until we hit title or };
                my @actionLines = ($_);
                while (<F>) {
                    last if (/\s*\};$/);
                    push @actionLines, $_;
                }
                foreach (@actionLines) {
                    if (/^(.*\s*\.title\s*=\s*")/) {
                        my $begin = $1;
                        my $s = substr ($_, length ($begin));
                        if ($s =~ /(.*[^\\])"/) {
                            my $prop = $1;

                            # Action titles can specify menu path, using /
                            # separated names. \/ needs to be preserved though.
                            my $copy = $prop;
                            $copy =~ s/\\\//\x{1}/g;

                            my @items = split ('/', $copy, -1);
                            for $prop (@items) {
                                $prop =~ s/\x{1}/\\\//g;
                                if ($prop =~ /[A-Za-z]/ && !grep ({$_->{msgid} eq $prop} @lines)) {
                                    push @lines, { f=>$relf, line=>$., msgid=>$prop };
                                }
                            }
                        }
                    }
                }
            }
        }
        close F;
    }
    return @lines;
}

1;
