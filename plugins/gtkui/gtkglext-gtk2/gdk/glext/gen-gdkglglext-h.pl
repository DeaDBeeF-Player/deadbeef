#!/usr/bin/perl -w
#
# gen-gdkglglext-h.pl
#
# Script for generating gdk/gdkglglext.h from SGI's OpenGL extension
# header.
#
# written by Naofumi Yasufuku <naofumi@users.sourceforge.net>
#

@input_headers = ("glext.h", "glext-extra.h");

#---------------
open(IN, "common-header.h") || die "cannot open common-header.h";
print while (<IN>);
close(IN);

print <<EOF;
/*
 * This is a generated file.  Please modify "gen-gdkglglext-h.pl".
 */

#ifndef __GDK_GL_GLEXT_H__
#define __GDK_GL_GLEXT_H__

#include <glib.h>

#ifdef G_OS_WIN32
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#endif

#include <GL/gl.h>

#include <gdk/gdkgldefs.h>
#include <gdk/gdkglquery.h>

G_BEGIN_DECLS

#ifndef HAVE_GLHALFNV
#if defined(GL_NV_half_float) && defined(GDKGLEXT_NEED_GLHALFNV_TYPEDEF)
typedef unsigned short GLhalfNV;
#endif
#endif

/* Avoid old glext.h bug. */
#if !defined(GL_SGIS_point_parameters) && defined(GL_POINT_SIZE_MIN_SGIS)
#define GL_SGIS_point_parameters 1
#endif

#undef __glext_h_
#undef GL_GLEXT_VERSION
#include <gdk/glext/glext.h>
#include <gdk/glext/glext-extra.h>

EOF
#---------------

foreach $in (@input_headers) {
    open(IN, $in) || die "cannot open $in";

    while (<IN>) {
	if (/#ifndef\s+GL_[a-zA-Z0-9]+_[a-z0-9_]+/) {
	    @line = split;
	    $_ = <IN>;
	    if (/#define\s+$line[1]/) {
		while (<IN>) {
		    if (/#ifdef\s+GL_GLEXT_PROTOTYPES/) {

			$extension = $line[1];

			# function prototypes
			@functions = ();
			while (<IN>) {
			    last if (/#endif/);
			    ($func) = /(gl\w+)/;
			    push(@functions, $func);
			}

			# typedefs
			@typedefs = ();
			while (<IN>) {
			    last if (/#endif/);
			    chomp;
			    push(@typedefs, $_);
			}

			generate_code();

			last;

		    } elsif (/#endif/) {
			last;
		    }
		}
	    }
	}
    }

    close(IN);
}

#---------------
print <<EOF;
G_END_DECLS

#endif /* __GDK_GL_GLEXT_H__ */
EOF
#---------------

# code generator
sub generate_code {
    print "/*\n";
    print " * $extension\n";
    print " */\n\n";

    $i = 0;
    foreach $func (@functions) {
	print "/* $func */\n";

	$type = $typedefs[$i++];
	$type =~ s/PFN\w+/GdkGLProc_$func/;
	print "$type\n";

	print "GdkGLProc    gdk_gl_get_$func (void);\n";

	$_ = $type;
	($args) = /\(.*\)\s+\((.*)\)/;
	@args_list = split(/,\s+/, $args);
	foreach (@args_list) {
	    ($_) = /.*\s+\**(\w+)$/;
	    $_ = "" if (!$_);
	}
	$args = join(", ", @args_list);

	if ($args eq "") {
	    print "#define      gdk_gl_$func(proc) \\\n";
	    print "  ( ((GdkGLProc_$func) (proc)) () )\n";
	} else {
	    print "#define      gdk_gl_$func(proc, $args) \\\n";
	    print "  ( ((GdkGLProc_$func) (proc)) ($args) )\n";
	}

	print "\n";
    }

    print "/* proc struct */\n\n";

    print "typedef struct _GdkGL_$extension GdkGL_$extension;\n\n";

    print "struct _GdkGL_$extension\n";
    print "{\n";
    foreach $func (@functions) {
	print "  GdkGLProc_$func $func;\n";
    }
    print "};\n\n";

    print "GdkGL_$extension *gdk_gl_get_$extension (void);\n\n";
}
