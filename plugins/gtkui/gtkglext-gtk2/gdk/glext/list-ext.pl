#!/usr/bin/perl -w
#
# list-ext.pl
#
# list OpenGL extensions.
#
# written by Naofumi Yasufuku <naofumi@users.sourceforge.net>
#

@glext_headers = ("glext.h", "glext-extra.h");
@glxext_headers = ("glxext.h", "glxext-extra.h");
@wglext_headers = ("wglext.h", "wglext-extra.h");

if (!@ARGV) {
    @input_headers = (@glext_headers, @glxext_headers, @wglext_headers);
} elsif ($ARGV[0] eq "--gl") {
    @input_headers = @glext_headers;
} elsif ($ARGV[0] eq "--glx") {
    @input_headers = @glxext_headers;
} elsif ($ARGV[0] eq "--wgl") {
    @input_headers = @wglext_headers;
}

foreach $in (@input_headers) {
    open(IN, $in) || die "cannot open $in";
    while (<IN>) {
        chomp;
        if (/#ifndef\s+(GL|GLX|WGL)_[a-zA-Z0-9]+_[a-z0-9_]+/) {
            @line = split;
            push(@tmp_exts, $line[1]);
        }
    }
    close(IN);
}

@exts = sort(@tmp_exts);
print "$exts[0]\n";
for ($i = 1; $i <= $#exts; $i++) {
    if ($exts[$i] ne $exts[$i-1]) {
        print "$exts[$i]\n";
    }
}
