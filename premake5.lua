workspace "deadbeef"
   configurations { "debug", "release", "debug32", "release32" }


defines {
    "VERSION=\"devel\"",
    "_GNU_SOURCE",
    "HAVE_LOG2=1"
}

filter "configurations:debug or debug32"
  defines { "DEBUG" }
  symbols "On"

filter "configurations:debug or release"
  buildoptions { "-fPIC", "-std=c99" }
  includedirs { "plugins/libmp4ff", "static-deps/lib-x86-64/include/x86_64-linux-gnu", "static-deps/lib-x86-64/include"  }
  libdirs { "static-deps/lib-x86-64/lib/x86_64-linux-gnu", "static-deps/lib-x86-64/lib" }


filter "configurations:debug32 or release32"
  buildoptions { "-std=c99", "-m32" }
  linkoptions { "-m32" }
  includedirs { "plugins/libmp4ff", "static-deps/lib-x86-32/include/i386-linux-gnu", "static-deps/lib-x86-32/include"  }
  libdirs { "static-deps/lib-x86-32/lib/i386-linux-gnu", "static-deps/lib-x86-32/lib" }

filter "configurations:release32 or release"
  buildoptions { "-O2" }

project "deadbeef"
   kind "ConsoleApp"
   language "C"
   targetdir "bin/%{cfg.buildcfg}"

   files {
       "*.h",
       "*.c",
       "md5/*.h",
       "md5/*.c",
       "plugins/libparser/*.h",
       "plugins/libparser/*.c",
       "ConvertUTF/*.h",
       "ConvertUTF/*.c"
   }

   defines { "PORTABLE=1", "STATICLINK=1", "PREFIX=\"donotuse\"", "LIBDIR=\"donotuse\"", "DOCDIR=\"donotuse\"" }
   links { "m", "pthread", "dl" }

project "mp3"
   kind "SharedLib"
   language "C"
   targetdir "bin/%{cfg.buildcfg}/plugins"
   targetprefix ""

   files {
       "plugins/mp3/*.h",
       "plugins/mp3/*.c",
   }

   defines { "USE_LIBMPG123=1", "USE_LIBMAD=1" }
   links { "mpg123", "mad" }

project "aac_plugin"
   kind "SharedLib"
   language "C"
   targetdir "bin/%{cfg.buildcfg}/plugins"
   targetprefix ""
   targetname "aac"

   files {
       "plugins/aac/*.h",
       "plugins/aac/*.c",
       "shared/mp4tagutil.h",
       "shared/mp4tagutil.c",
       "plugins/libmp4ff/*.h",
       "plugins/libmp4ff/*.c"
   }

   defines { "USE_MP4FF=1", "USE_TAGGING=1" }
   links { "faad" }

project "alac_plugin"
   kind "SharedLib"
   language "C"
   targetdir "bin/%{cfg.buildcfg}/plugins"
   targetprefix ""
   targetname "alac"

   files {
       "plugins/alac/alac_plugin.c",
       "plugins/alac/alac.c",
       "plugins/alac/decomp.h",
       "plugins/alac/demux.c",
       "plugins/alac/demux.h",
       "plugins/alac/stream.c",
       "plugins/alac/stream.h",
       "shared/mp4tagutil.h",
       "shared/mp4tagutil.c",
       "plugins/libmp4ff/*.h",
       "plugins/libmp4ff/*.c"
   }

   defines { "USE_MP4FF=1", "USE_TAGGING=1" }
   links { "faad" }

project "flac_plugin"
   kind "SharedLib"
   language "C"
   targetdir "bin/%{cfg.buildcfg}/plugins"
   targetprefix ""
   targetname "flac"

   files {
       "plugins/flac/*.h",
       "plugins/flac/*.c",
       "plugins/liboggedit/*.h",
       "plugins/liboggedit/*.c",
   }

   defines { "HAVE_OGG_STREAM_FLUSH_FILL" }
   links { "FLAC", "ogg" }

project "wavpack_plugin"
   kind "SharedLib"
   language "C"
   targetdir "bin/%{cfg.buildcfg}/plugins"
   targetprefix ""
   targetname "wavpack"

   files {
       "plugins/wavpack/*.h",
       "plugins/wavpack/*.c",
   }

   links { "wavpack" }

project "ffmpeg"
   kind "SharedLib"
   language "C"
   targetdir "bin/%{cfg.buildcfg}/plugins"
   targetprefix ""

   files {
       "plugins/ffmpeg/*.h",
       "plugins/ffmpeg/*.c",
   }

   links {"avcodec", "pthread", "avformat", "avcodec", "avutil", "z", "opencore-amrnb", "opencore-amrwb", "opus"}

project "vorbis_plugin"
   kind "SharedLib"
   language "C"
   targetdir "bin/%{cfg.buildcfg}/plugins"
   targetprefix ""
   targetname "vorbis"

   files {
       "plugins/vorbis/*.h",
       "plugins/vorbis/*.c",
       "plugins/liboggedit/*.h",
       "plugins/liboggedit/*.c",
   }

   defines { "HAVE_OGG_STREAM_FLUSH_FILL" }
   links { "vorbisfile", "vorbis", "m", "ogg" }

project "opus_plugin"
   kind "SharedLib"
   language "C"
   targetdir "bin/%{cfg.buildcfg}/plugins"
   targetprefix ""
   targetname "opus"

   files {
       "plugins/opus/*.h",
       "plugins/opus/*.c",
       "plugins/liboggedit/*.h",
       "plugins/liboggedit/*.c",
   }

   defines { "HAVE_OGG_STREAM_FLUSH_FILL" }
   links { "opusfile", "opus", "m", "ogg" }
   filter "configurations:debug32 or release32"
   
      includedirs { "static-deps/lib-x86-32/include/opus" }

   filter "configurations:debug or release"
   
      includedirs { "static-deps/lib-x86-64/include/opus" }

project "ffap"
   kind "SharedLib"
   language "C"
   targetdir "bin/%{cfg.buildcfg}/plugins"
   targetprefix ""

   files {
       "plugins/ffap/*.h",
       "plugins/ffap/*.c",
       "plugins/ffap/dsputil_yasm.asm",
   }

   filter 'files:**.asm'
       buildmessage 'YASM Assembling : %{file.relpath}'

       filter "configurations:debug32 or release32"
           buildcommands
           {
               'yasm -f elf -D ARCH_X86_32 -m x86 -DPREFIX -o "obj/%{cfg.buildcfg}/ffap/%{file.basename}.o" "%{file.relpath}"'
           }

           buildoutputs
           {
               "obj/%{cfg.buildcfg}/ffap/%{file.basename}.o"
           }

           defines { "APE_USE_ASM=yes", "ARCH_X86_32=1" }

       filter "configurations:debug or release"
           buildcommands
           {
               'yasm -f elf -D ARCH_X86_64 -m amd64 -DPIC -DPREFIX -o "obj/%{cfg.buildcfg}/ffap/%{file.basename}.o" "%{file.relpath}"'
           }

           buildoutputs
           {
               "obj/%{cfg.buildcfg}/ffap/%{file.basename}.o"
           }

           defines { "APE_USE_ASM=yes", "ARCH_X86_64=1" }


project "hotkeys"
   kind "SharedLib"
   language "C"
   targetdir "bin/%{cfg.buildcfg}/plugins"
   targetprefix ""

   files {
       "plugins/hotkeys/*.h",
       "plugins/hotkeys/*.c",
       "plugins/libparser/*.h",
       "plugins/libparser/*.c",
   }

   links { "X11" }

project "alsa"
   kind "SharedLib"
   language "C"
   targetdir "bin/%{cfg.buildcfg}/plugins"
   targetprefix ""

   files {
       "plugins/alsa/*.h",
       "plugins/alsa/*.c",
   }

   links { "asound" }

project "dsp_libsrc"
   kind "SharedLib"
   language "C"
   targetdir "bin/%{cfg.buildcfg}/plugins"
   targetprefix ""

   files {
       "plugins/dsp_libsrc/src.c",
   }

   links { "samplerate" }

project "pulse"
   kind "SharedLib"
   language "C"
   targetdir "bin/%{cfg.buildcfg}/plugins"
   targetprefix ""

   files {
       "plugins/pulse/*.h",
       "plugins/pulse/*.c",
   }

   links { "pulse-simple" }

project "ddb_gui_GTK2"
   kind "SharedLib"
   language "C"
   targetdir "bin/%{cfg.buildcfg}/plugins"
   targetprefix ""
   files {
       "plugins/gtkui/*.h",
       "plugins/gtkui/*.c",
       "shared/pluginsettings.h",
       "shared/pluginsettings.c",
       "shared/trkproperties_shared.h",
       "shared/trkproperties_shared.c",
       "plugins/libparser/parser.h",
       "plugins/libparser/parser.c",
       "utf8.c",
   }
   excludes {
        "plugins/gtkui/deadbeefapp.c",
        "plugins/gtkui/gtkui-gresources.c"
   }

   links { "jansson", "gtk-x11-2.0", "pango-1.0", "cairo", "gdk-x11-2.0", "gdk_pixbuf-2.0", "gobject-2.0", "gthread-2.0", "glib-2.0" }

    filter "configurations:debug32 or release32"
    
       includedirs { "static-deps/lib-x86-32/gtk-2.16.0/include/**", "static-deps/lib-x86-32/gtk-2.16.0/lib/**", "plugins/gtkui", "plugins/libparser" }
       libdirs { "static-deps/lib-x86-32/gtk-2.16.0/lib", "static-deps/lib-x86-32/gtk-2.16.0/lib/**" }

    filter "configurations:debug or release"
    
       includedirs { "static-deps/lib-x86-64/gtk-2.16.0/include/**", "static-deps/lib-x86-64/gtk-2.16.0/lib/**", "plugins/gtkui", "plugins/libparser" }
       libdirs { "static-deps/lib-x86-64/gtk-2.16.0/lib", "static-deps/lib-x86-64/gtk-2.16.0/lib/**" }

project "ddb_gui_GTK3"
   kind "SharedLib"
   language "C"
   targetdir "bin/%{cfg.buildcfg}/plugins"
   targetprefix ""
   files {
       "plugins/gtkui/*.h",
       "plugins/gtkui/*.c",
       "shared/pluginsettings.h",
       "shared/pluginsettings.c",
       "shared/trkproperties_shared.h",
       "shared/trkproperties_shared.c",
       "plugins/libparser/parser.h",
       "plugins/libparser/parser.c",
       "utf8.c",
   }

   prebuildcommands {
	"glib-compile-resources --sourcedir=plugins/gtkui --target=plugins/gtkui/gtkui-gresources.c --generate-source plugins/gtkui/gtkui.gresources.xml"
   }
   defines { "USE_GTK_APPLICATION=1" }

   links { "jansson", "gtk-3", "gdk-3", "pangocairo-1.0", "pango-1.0", "atk-1.0", "cairo-gobject", "cairo", "gdk_pixbuf-2.0", "gio-2.0", "gobject-2.0", "gthread-2.0", "glib-2.0" }

    filter "configurations:debug32 or release32"

       includedirs { "static-deps/lib-x86-32/gtk-3.10.8/usr/include/**", "static-deps/lib-x86-32/gtk-3.10.8/usr/lib/**", "plugins/gtkui", "plugins/libparser" }
       libdirs { "static-deps/lib-x86-32/gtk-3.10.8/lib/**", "static-deps/lib-x86-32/gtk-3.10.8/usr/lib/**" }

    filter "configurations:debug or release"

       includedirs { "static-deps/lib-x86-64/gtk-3.10.8/usr/include/**", "static-deps/lib-x86-64/gtk-3.10.8/usr/lib/**", "plugins/gtkui", "plugins/libparser" }
       libdirs { "static-deps/lib-x86-64/gtk-3.10.8/lib/**", "static-deps/lib-x86-64/gtk-3.10.8/usr/lib/**" }

project "rg_scanner"
   kind "SharedLib"
   language "C"
   targetdir "bin/%{cfg.buildcfg}/plugins"
   targetprefix ""

   files {
       "plugins/rg_scanner/*.h",
       "plugins/rg_scanner/*.c",
       "plugins/rg_scanner/ebur128/*.h",
       "plugins/rg_scanner/ebur128/*.c",
   }

project "converter"
   kind "SharedLib"
   language "C"
   targetdir "bin/%{cfg.buildcfg}/plugins"
   targetprefix ""

   defines {
      "USE_TAGGING=1"
   }
   files {
       "plugins/converter/converter.c",
       "plugins/libmp4ff/*.c",
       "shared/mp4tagutil.c",
   }

project "sndfile_plugin"
   kind "SharedLib"
   language "C"
   targetdir "bin/%{cfg.buildcfg}/plugins"
   targetprefix ""

   files {
       "plugins/sndfile/*.c",
       "plugins/sndfile/*.h",
   }
   links { "sndfile" }
   targetname "sndfile"

project "sid"
   kind "SharedLib"
   language "C"
   targetdir "bin/%{cfg.buildcfg}/plugins"
   targetprefix ""

   includedirs {
        "plugins/sid/sidplay-libs/libsidplay/include",
        "plugins/sid/sidplay-libs/builders/resid-builder/include",
        "plugins/sid/sidplay-libs",
        "plugins/sid/sidplay-libs/unix",
        "plugins/sid/sidplay-libs/libsidplay",
        "plugins/sid/sidplay-libs/libsidplay/include",
        "plugins/sid/sidplay-libs/libsidplay/include/sidplay",
        "plugins/sid/sidplay-libs/libsidutils/include/sidplay/utils",
        "plugins/sid/sidplay-libs/builders/resid-builder/include/sidplay/builders",
        "plugins/sid/sidplay-libs/builders/resid-builder/include"
    }
   defines {
      "HAVE_STRCASECMP=1",
      "HAVE_STRNCASECMP=1",
      "PACKAGE=\"libsidplay2\"",
   }

   files {
       "plugins/sid/*.c",
       "plugins/sid/*.cpp",
       "plugins/sid/sidplay-libs/libsidplay/src/*.cpp",
       "plugins/sid/sidplay-libs/libsidplay/src/*.c",
       "plugins/sid/sidplay-libs/builders/resid-builder/src/*.cpp",
       "plugins/sid/sidplay-libs/libsidplay/src/c64/*.cpp",
       "plugins/sid/sidplay-libs/libsidplay/src/mos6510/*.cpp",
       "plugins/sid/sidplay-libs/libsidplay/src/mos6526/*.cpp",
       "plugins/sid/sidplay-libs/libsidplay/src/mos656x/*.cpp",
       "plugins/sid/sidplay-libs/libsidplay/src/sid6526/*.cpp",
       "plugins/sid/sidplay-libs/libsidplay/src/sidtune/*.cpp",
       "plugins/sid/sidplay-libs/libsidplay/src/xsid/*.cpp",
       "plugins/sid/sidplay-libs/resid/*.cpp"
   }
   targetname "sid"
   links { "stdc++" }

project "psf"
   kind "SharedLib"
   language "C"
   targetdir "bin/%{cfg.buildcfg}/plugins"
   targetprefix ""

   includedirs {
        "plugins/psf",
        "plugins/psf/eng_ssf",
        "plugins/psf/eng_qsf",
        "plugins/psf/eng_dsf",
    }
   defines {
      "HAS_PSXCPU=1",
   }

   files {
        "plugins/psf/plugin.c",
        "plugins/psf/psfmain.c",
        "plugins/psf/corlett.c",
        "plugins/psf/eng_dsf/eng_dsf.c",
        "plugins/psf/eng_dsf/dc_hw.c",
        "plugins/psf/eng_dsf/aica.c",
        "plugins/psf/eng_dsf/aicadsp.c",
        "plugins/psf/eng_dsf/arm7.c",
        "plugins/psf/eng_dsf/arm7i.c",
        "plugins/psf/eng_ssf/m68kcpu.c",
        "plugins/psf/eng_ssf/m68kopac.c",
        "plugins/psf/eng_ssf/m68kopdm.c",
        "plugins/psf/eng_ssf/m68kopnz.c",
        "plugins/psf/eng_ssf/m68kops.c",
        "plugins/psf/eng_ssf/scsp.c",
        "plugins/psf/eng_ssf/scspdsp.c",
        "plugins/psf/eng_ssf/sat_hw.c",
        "plugins/psf/eng_ssf/eng_ssf.c",
        "plugins/psf/eng_qsf/eng_qsf.c",
        "plugins/psf/eng_qsf/kabuki.c",
        "plugins/psf/eng_qsf/qsound.c",
        "plugins/psf/eng_qsf/z80.c",
        "plugins/psf/eng_qsf/z80dasm.c",
        "plugins/psf/eng_psf/eng_psf.c",
        "plugins/psf/eng_psf/psx.c",
        "plugins/psf/eng_psf/psx_hw.c",
        "plugins/psf/eng_psf/peops/spu.c",
        "plugins/psf/eng_psf/eng_psf2.c",
        "plugins/psf/eng_psf/peops2/spu2.c",
        "plugins/psf/eng_psf/peops2/dma2.c",
        "plugins/psf/eng_psf/peops2/registers2.c",
        "plugins/psf/eng_psf/eng_spu.c",
   }
   targetname "psf"
   links { "z", "m" }

project "m3u"
   kind "SharedLib"
   language "C"
   targetdir "bin/%{cfg.buildcfg}/plugins"
   targetprefix ""

   files {
       "plugins/m3u/*.c",
       "plugins/m3u/*.h",
   }

project "vfs_curl"
   kind "SharedLib"
   language "C"
   targetdir "bin/%{cfg.buildcfg}/plugins"
   targetprefix ""

   files {
       "plugins/vfs_curl/*.c",
       "plugins/vfs_curl/*.h",
   }

   links { "curl", "rt" }

project "converter_gtk2"
   kind "SharedLib"
   language "C"
   targetdir "bin/%{cfg.buildcfg}/plugins"
   targetprefix ""

   files {
       "plugins/converter/convgui.c",
       "plugins/converter/callbacks.c",
       "plugins/converter/interface.c",
       "plugins/converter/support.c",
   }
   links { "gtk-x11-2.0", "pango-1.0", "cairo", "gdk-x11-2.0", "gdk_pixbuf-2.0", "gobject-2.0", "gthread-2.0", "glib-2.0" }

   filter "configurations:debug32 or release32"
       includedirs { "static-deps/lib-x86-32/gtk-2.16.0/include/**", "static-deps/lib-x86-32/gtk-2.16.0/lib/**", "plugins/gtkui", "plugins/libparser" }
       libdirs { "static-deps/lib-x86-32/gtk-2.16.0/lib", "static-deps/lib-x86-32/gtk-2.16.0/lib/**" }

   filter "configurations:release or debug"
       includedirs { "static-deps/lib-x86-64/gtk-2.16.0/include/**", "static-deps/lib-x86-64/gtk-2.16.0/lib/**", "plugins/gtkui", "plugins/libparser" }
       libdirs { "static-deps/lib-x86-64/gtk-2.16.0/lib", "static-deps/lib-x86-64/gtk-2.16.0/lib/**" }


project "wildmidi_plugin"
   kind "SharedLib"
   language "C"
   targetdir "bin/%{cfg.buildcfg}/plugins"
   targetprefix ""
   targetname "wildmidi"

   files {
       "plugins/wildmidi/*.h",
       "plugins/wildmidi/*.c",
       "plugins/wildmidi/src/*.h",
       "plugins/wildmidi/src/*.c",
   }

   excludes {
       "plugins/wildmidi/src/wildmidi.c"
   }

   includedirs { "plugins/wildmidi/include" }

   defines { "WILDMIDI_VERSION=\"0.2.2\"", "WILDMIDILIB_VERSION=\"0.2.2\"", "TIMIDITY_CFG=\"/etc/timidity.conf\"" }
   links { "m" }

project "artwork_plugin"
   kind "SharedLib"
   language "C"
   targetdir "bin/%{cfg.buildcfg}/plugins"
   targetprefix ""
   targetname "artwork"

   files {
       "plugins/artwork-legacy/*.c",
       "plugins/libmp4ff/*.c"
   }

   excludes {
   }

   includedirs { "../libmp4ff" }

   defines { "USE_OGG=1", "USE_VFS_CURL", "USE_METAFLAC", "USE_MP4FF", "USE_TAGGING=1" }
   links { "jpeg", "png", "z", "FLAC", "ogg" }

project "supereq_plugin"
   kind "SharedLib"
   language "C"
   targetdir "bin/%{cfg.buildcfg}/plugins"
   targetprefix ""
   targetname "supereq"

   files {
       "plugins/supereq/*.c",
       "plugins/supereq/*.cpp"
   }

   defines { "USE_OOURA" }
   links { "m", "stdc++" }

project "mono2stereo_plugin"
   kind "SharedLib"
   language "C"
   targetdir "bin/%{cfg.buildcfg}/plugins"
   targetprefix ""
   targetname "ddb_mono2stereo"

   files {
       "plugins/mono2stereo/*.c",
   }

project "nullout"
   kind "SharedLib"
   language "C"
   targetdir "bin/%{cfg.buildcfg}/plugins"
   targetprefix ""

   files {
       "plugins/nullout/*.h",
       "plugins/nullout/*.c",
   }


project "resources"
    kind "Utility"
    postbuildcommands {
        "{MKDIR} bin/%{cfg.buildcfg}/pixmaps",
        "{COPY} icons/32x32/deadbeef.png bin/%{cfg.buildcfg}",
        "{COPY} pixmaps/*.png bin/%{cfg.buildcfg}/pixmaps/",
        "{MKDIR} bin/%{cfg.buildcfg}/plugins/convpresets",
        "{COPY} plugins/converter/convpresets bin/%{cfg.buildcfg}/plugins/",
    }

