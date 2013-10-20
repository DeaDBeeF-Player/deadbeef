#!/usr/bin/perl -w
#
# gen-gdkglglxext-h.pl
#
# Script for generating gdk/x11/gdkglglxext.h from SGI's OpenGL extension
# header.
#
# written by Naofumi Yasufuku <naofumi@users.sourceforge.net>
#

@input_headers = ("glxext.h", "glxext-extra.h");

#---------------
open(IN, "common-header.h") || die "cannot open common-header.h";
print while (<IN>);
close(IN);

print <<EOF;
/*
 * This is a generated file.  Please modify "gen-gdkglglxext-h.pl".
 */

#ifndef __GDK_GL_GLXEXT_H__
#define __GDK_GL_GLXEXT_H__

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <GL/gl.h>
#include <GL/glx.h>

#include <gdk/gdkgldefs.h>
#include <gdk/gdkglquery.h>
#include <gdk/gdkglconfig.h>

G_BEGIN_DECLS

#ifndef GLX_VERSION_1_3
typedef struct __GLXFBConfigRec *GLXFBConfig;
typedef XID GLXFBConfigID;
typedef XID GLXContextID;
typedef XID GLXWindow;
typedef XID GLXPbuffer;
#endif

#ifndef HAVE_GLXFBCONFIGSGIX
#if defined(GLX_SGIX_fbconfig) && defined(GDKGLEXT_NEED_GLXFBCONFIGSGIX_TYPEDEF)
typedef struct __GLXFBConfigRec *GLXFBConfigSGIX;
#endif
#endif

#ifndef HAVE_GLXFBCONFIGIDSGIX
#if defined(GLX_SGIX_fbconfig) && defined(GDKGLEXT_NEED_GLXFBCONFIGIDSGIX_TYPEDEF)
typedef XID GLXFBConfigIDSGIX;
#endif
#endif

#ifndef HAVE_GLXPBUFFERSGIX
#if defined(GLX_SGIX_pbuffer) && defined(GDKGLEXT_NEED_GLXPBUFFERSGIX_TYPEDEF)
typedef XID GLXPbufferSGIX;
#endif
#endif

/* for __GLXextFuncPtr typedef in glxext.h */
#ifndef HAVE___GLXEXTFUNCPTR
#if defined(GLX_ARB_get_proc_address) && defined(GDKGLEXT_NEED_GLXEXTFUNCPTR_TYPEDEF)
#undef GLX_ARB_get_proc_address
#endif
#endif

/* Suppress 'redefined' warnings (Solaris 8, etc.) */

#if !defined(GLX_ARB_multisample) && defined(GLX_SAMPLE_BUFFERS_ARB)
#define GLX_ARB_multisample 1
#endif

#if !defined(GLX_SGIS_multisample) && defined(GLX_SAMPLE_BUFFERS_SGIS)
#define GLX_SGIS_multisample 1
#endif

#if !defined(GLX_EXT_visual_rating) && defined(GLX_VISUAL_CAVEAT_EXT)
#define GLX_EXT_visual_rating 1
#endif

#if !defined(GLX_EXT_import_context) && defined(GLX_SHARE_CONTEXT_EXT)
#define GLX_EXT_import_context 1
#endif

/* Avoid old glxext.h bug. */
#if !defined(GLX_SGIX_video_source) && !defined(GDKGLEXT_NEED_GLXVIDEOSOURCESGIX_TYPEDEF)
#define GLX_SGIX_video_source 1
#endif

#undef __glxext_h_
#undef GLX_GLXEXT_VERSION
#include <gdk/glext/glxext.h>
#include <gdk/glext/glxext-extra.h>

EOF
#---------------

foreach $in (@input_headers) {
    open(IN, $in) || die "cannot open $in";

    while (<IN>) {
	if (/#ifndef\s+GLX_[a-zA-Z0-9]+_[a-z0-9_]+/) {
	    @line = split;
	    $_ = <IN>;
	    if (/#define\s+$line[1]/) {
		while (<IN>) {
		    if (/#ifdef\s+GLX_GLXEXT_PROTOTYPES/) {

			$extension = $line[1];

			# function prototypes
			@functions = ();
			while (<IN>) {
			    last if (/#endif/);
			    ($func) = /(glX\w+)/;
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

#endif /* __GDK_GL_GLXEXT_H__ */
EOF
#---------------

# code generator
sub generate_code {
    print "/*\n";
    print " * $extension\n";
    print " */\n\n";

    if ($extension eq "GLX_SGIX_video_source") {
	print "#ifdef _VL_H\n\n";
    } elsif ($extension eq "GLX_SGIX_dmbuffer") {
	print "#ifdef _DM_BUFFER_H_\n\n";
    }

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

    if ($extension =~ /^GLX_VERSION_.*/) {
	print "GdkGL_$extension *gdk_gl_get_$extension (void);\n\n";
    } else {
	print "GdkGL_$extension *gdk_gl_get_$extension (GdkGLConfig *glconfig);\n\n";
    }

    if ($extension eq "GLX_SGIX_video_source") {
	print "#endif /* _VL_H */\n\n";
    } elsif ($extension eq "GLX_SGIX_dmbuffer") {
	print "#endif /* _DM_BUFFER_H_ */\n\n";
    }
}
