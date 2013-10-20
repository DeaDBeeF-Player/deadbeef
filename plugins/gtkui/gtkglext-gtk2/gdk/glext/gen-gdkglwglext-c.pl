#!/usr/bin/perl -w
#
# gen-gdkglwglext-c.pl
#
# Script for generating gdk/win32/gdkglwglext.c from SGI's OpenGL extension
# header.
#
# written by Naofumi Yasufuku <naofumi@users.sourceforge.net>
#

@input_headers = ("wglext.h", "wglext-extra.h");

#---------------
open(IN, "common-header.h") || die "cannot open common-header.h";
print while (<IN>);
close(IN);

print <<EOF;
/*
 * This is a generated file.  Please modify "gen-gdkglwglext-c.pl".
 */

#include "gdkglwin32.h"
#include "gdkglprivate-win32.h"
#include "gdkglwglext.h"

EOF
#---------------

foreach $in (@input_headers) {
    open(IN, $in) || die "cannot open $in";

    while (<IN>) {
	if (/#ifndef\s+WGL_[a-zA-Z0-9]+_[a-z0-9_]+/) {
	    @line = split;
	    $_ = <IN>;
	    if (/#define\s+$line[1]/) {
		while (<IN>) {
		    if (/#ifdef\s+WGL_WGLEXT_PROTOTYPES/) {

			$extension = $line[1];

			# function prototypes
			@functions = ();
			while (<IN>) {
			    last if (/#endif/);
			    ($func) = /(wgl\w+)/;
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

# code generator
sub generate_code {
    print "/*\n";
    print " * $extension\n";
    print " */\n\n";

    print "static GdkGL_$extension _procs_$extension = {\n";
    print "  (GdkGLProc_$functions[0]) -1";
    for ($i = 1; $i <= $#functions; $i++) {
	print ",\n  (GdkGLProc_$functions[$i]) -1";
    }
    print "\n};\n\n";

    foreach $func (@functions) {
	print "/* $func */\n";
	print "GdkGLProc\n";
	print "gdk_gl_get_$func (void)\n";
	print "{\n";
	print "  if (wglGetCurrentContext () == NULL)\n";
	print "    return NULL;\n";
	print "\n";
	print "  if (_procs_$extension.$func == (GdkGLProc_$func) -1)\n";
	print "    _procs_$extension.$func =\n";
	print "      (GdkGLProc_$func) gdk_gl_get_proc_address (\"$func\");\n";
	print "\n";
	print "  GDK_GL_NOTE (MISC,\n";
	print "    g_message (\" - gdk_gl_get_$func () - \%s\",\n";
	print "               (_procs_$extension.$func) ? \"supported\" : \"not supported\"));\n";
	print "\n";
	print "  return (GdkGLProc) (_procs_$extension.$func);\n";
	print "}\n\n";
    }

    print "/* Get $extension functions */\n";
    print "GdkGL_$extension *\n";
    print "gdk_gl_get_$extension (GdkGLConfig *glconfig)\n";
    print "{\n";
    print "  static gint supported = -1;\n";
    print "\n";
    print "  if (wglGetCurrentContext () == NULL)\n";
    print "    return NULL;\n";
    print "\n";
    print "  if (supported == -1)\n";
    print "    {\n";
    print "      supported = gdk_win32_gl_query_wgl_extension (glconfig, \"$extension\");\n";
    print "\n";
    print "      if (supported)\n";
    print "        {\n";
    foreach $func (@functions) {
	print "          supported &= (gdk_gl_get_$func () != NULL);\n";
    }
    print "        }\n";
    print "    }\n";
    print "\n";
    print "  GDK_GL_NOTE (MISC,\n";
    print "    g_message (\" - gdk_gl_get_$extension () - \%s\",\n";
    print "               (supported) ? \"supported\" : \"not supported\"));\n";
    print "\n";
    print "  if (!supported)\n";
    print "    return NULL;\n";
    print "\n";
    print "  return &_procs_$extension;\n";
    print "}\n\n";
}
