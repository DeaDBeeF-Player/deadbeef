workspace "deadbeef"
   configurations { "debug", "release", "debug32", "release32" }


defines {
    "VERSION=\"devel\"",
    "_GNU_SOURCE",
    "HAVE_LOG2=1"
}

linkgroups 'On'

filter "configurations:debug or debug32"
  defines { "DEBUG" }
  symbols "On"

filter "configurations:debug or release"
  buildoptions { "-fPIC", "-std=c99" }
  includedirs { "static-deps/lib-x86-64/include/x86_64-linux-gnu", "static-deps/lib-x86-64/include" }
  libdirs { "static-deps/lib-x86-64/lib/x86_64-linux-gnu", "static-deps/lib-x86-64/lib" }


filter "configurations:debug32 or release32"
  buildoptions { "-std=c99", "-m32" }
  linkoptions { "-m32" }
  includedirs { "static-deps/lib-x86-32/include/i386-linux-gnu", "static-deps/lib-x86-32/include" }
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
       "external/wcwidth/wcwidth.c",
       "external/wcwidth/wcwidth.h",
       "ConvertUTF/*.h",
       "ConvertUTF/*.c",
       "shared/ctmap.c",
       "shared/ctmap.h"
   }

   defines { "PORTABLE=1", "STATICLINK=1", "PREFIX=\"donotuse\"", "LIBDIR=\"donotuse\"", "DOCDIR=\"donotuse\"" }
   links { "m", "pthread", "dl" }

project "mp4p"
  kind "StaticLib"
  language "C"
  targetdir "bin/%{cfg.buildcfg}/plugins"
  targetprefix ""
  files {
      "external/mp4p/src/*.c",
  }
  includedirs { "external/mp4p/include" }

project "liboggedit"
  kind "StaticLib"
  language "C"
  targetdir "bin/%{cfg.buildcfg}/plugins"
  targetprefix ""
  files {
      "plugins/liboggedit/*.c",
      "plugins/liboggedit/*.h"
  }

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

   includedirs { "external/mp4p/include" }

   files {
       "plugins/aac/aac.c",
       "plugins/aac/aac_decoder_faad2.c",
       "plugins/aac/aac_decoder_wrap.c",
       "plugins/aac/aac_parser.c",
       "plugins/aac/aac_decoder_faad2.h",
       "plugins/aac/aac_decoder_protocol.h",
       "plugins/aac/aac_parser.h",
       "shared/mp4tagutil.h",
       "shared/mp4tagutil.c"
   }

   links { "faad", "mp4p" }

project "adplug_plugin"
   kind "SharedLib"
   language "C"
   targetdir "bin/%{cfg.buildcfg}/plugins"
   targetprefix ""
   targetname "adplug"

   files {
       "plugins/adplug/plugin.c",
       "plugins/adplug/adplug-db.cpp",
       "plugins/adplug/libbinio/binfile.h",
       "plugins/adplug/libbinio/binio.h",
       "plugins/adplug/libbinio/binstr.h",
       "plugins/adplug/libbinio/binwrap.h",
       "plugins/adplug/libbinio/binfile.cpp",
       "plugins/adplug/libbinio/binio.cpp",
       "plugins/adplug/libbinio/binstr.cpp",
       "plugins/adplug/libbinio/binwrap.cpp",
       "plugins/adplug/adplug/a2m.cpp",
       "plugins/adplug/adplug/a2m.h",
       "plugins/adplug/adplug/adl.cpp",
       "plugins/adplug/adplug/adl.h",
       "plugins/adplug/adplug/adlib.cpp",
       "plugins/adplug/adplug/adlib.h",
       "plugins/adplug/adplug/adlibemu.h",
       "plugins/adplug/adplug/adplug.cpp",
       "plugins/adplug/adplug/adplug.h",
       "plugins/adplug/adplug/adtrack.cpp",
       "plugins/adplug/adplug/adtrack.h",
       "plugins/adplug/adplug/amd.cpp",
       "plugins/adplug/adplug/amd.h",
       "plugins/adplug/adplug/analopl.cpp",
       "plugins/adplug/adplug/analopl.h",
       "plugins/adplug/adplug/bam.cpp",
       "plugins/adplug/adplug/bam.h",
       "plugins/adplug/adplug/bmf.cpp",
       "plugins/adplug/adplug/bmf.h",
       "plugins/adplug/adplug/cff.cpp",
       "plugins/adplug/adplug/cff.h",
       "plugins/adplug/adplug/cmf.cpp",
       "plugins/adplug/adplug/cmf.h",
       "plugins/adplug/adplug/cmfmcsop.cpp",
       "plugins/adplug/adplug/cmfmcsop.h",
       "plugins/adplug/adplug/d00.cpp",
       "plugins/adplug/adplug/d00.h",
       "plugins/adplug/adplug/database.cpp",
       "plugins/adplug/adplug/database.h",
       "plugins/adplug/adplug/debug.h",
       "plugins/adplug/adplug/dfm.cpp",
       "plugins/adplug/adplug/dfm.h",
       "plugins/adplug/adplug/diskopl.cpp",
       "plugins/adplug/adplug/diskopl.h",
       "plugins/adplug/adplug/dmo.cpp",
       "plugins/adplug/adplug/dmo.h",
       "plugins/adplug/adplug/dro.cpp",
       "plugins/adplug/adplug/dro.h",
       "plugins/adplug/adplug/dro2.cpp",
       "plugins/adplug/adplug/dro2.h",
       "plugins/adplug/adplug/dtm.cpp",
       "plugins/adplug/adplug/dtm.h",
       "plugins/adplug/adplug/emuopl.cpp",
       "plugins/adplug/adplug/emuopl.h",
       "plugins/adplug/adplug/flash.cpp",
       "plugins/adplug/adplug/flash.h",
       "plugins/adplug/adplug/fmc.cpp",
       "plugins/adplug/adplug/fmc.h",
       "plugins/adplug/adplug/fmopl.h",
       "plugins/adplug/adplug/fprovide.cpp",
       "plugins/adplug/adplug/fprovide.h",
       "plugins/adplug/adplug/got.cpp",
       "plugins/adplug/adplug/got.h",
       "plugins/adplug/adplug/herad.cpp",
       "plugins/adplug/adplug/herad.h",
       "plugins/adplug/adplug/hsc.cpp",
       "plugins/adplug/adplug/hsc.h",
       "plugins/adplug/adplug/hsp.cpp",
       "plugins/adplug/adplug/hsp.h",
       "plugins/adplug/adplug/hybrid.cpp",
       "plugins/adplug/adplug/hybrid.h",
       "plugins/adplug/adplug/hyp.cpp",
       "plugins/adplug/adplug/hyp.h",
       "plugins/adplug/adplug/imf.cpp",
       "plugins/adplug/adplug/imf.h",
       "plugins/adplug/adplug/jbm.cpp",
       "plugins/adplug/adplug/jbm.h",
       "plugins/adplug/adplug/kemuopl.h",
       "plugins/adplug/adplug/ksm.cpp",
       "plugins/adplug/adplug/ksm.h",
       "plugins/adplug/adplug/lds.cpp",
       "plugins/adplug/adplug/lds.h",
       "plugins/adplug/adplug/mad.cpp",
       "plugins/adplug/adplug/mad.h",
       "plugins/adplug/adplug/mdi.cpp",
       "plugins/adplug/adplug/mdi.h",
       "plugins/adplug/adplug/mid.cpp",
       "plugins/adplug/adplug/mid.h",
       "plugins/adplug/adplug/mididata.h",
       "plugins/adplug/adplug/mkj.cpp",
       "plugins/adplug/adplug/mkj.h",
       "plugins/adplug/adplug/msc.cpp",
       "plugins/adplug/adplug/msc.h",
       "plugins/adplug/adplug/mtk.cpp",
       "plugins/adplug/adplug/mtk.h",
       "plugins/adplug/adplug/mus.cpp",
       "plugins/adplug/adplug/mus.h",
       "plugins/adplug/adplug/nemuopl.cpp",
       "plugins/adplug/adplug/nemuopl.h",
       "plugins/adplug/adplug/nukedopl.h",
       "plugins/adplug/adplug/opl.h",
       "plugins/adplug/adplug/player.cpp",
       "plugins/adplug/adplug/player.h",
       "plugins/adplug/adplug/players.cpp",
       "plugins/adplug/adplug/players.h",
       "plugins/adplug/adplug/protrack.cpp",
       "plugins/adplug/adplug/protrack.h",
       "plugins/adplug/adplug/psi.cpp",
       "plugins/adplug/adplug/psi.h",
       "plugins/adplug/adplug/rad.cpp",
       "plugins/adplug/adplug/rad.h",
       "plugins/adplug/adplug/rat.cpp",
       "plugins/adplug/adplug/rat.h",
       "plugins/adplug/adplug/raw.cpp",
       "plugins/adplug/adplug/raw.h",
       "plugins/adplug/adplug/realopl.cpp",
       "plugins/adplug/adplug/realopl.h",
       "plugins/adplug/adplug/rix.cpp",
       "plugins/adplug/adplug/rix.h",
       "plugins/adplug/adplug/rol.cpp",
       "plugins/adplug/adplug/rol.h",
       "plugins/adplug/adplug/s3m.cpp",
       "plugins/adplug/adplug/s3m.h",
       "plugins/adplug/adplug/sa2.cpp",
       "plugins/adplug/adplug/sa2.h",
       "plugins/adplug/adplug/silentopl.h",
       "plugins/adplug/adplug/sng.cpp",
       "plugins/adplug/adplug/sng.h",
       "plugins/adplug/adplug/sop.cpp",
       "plugins/adplug/adplug/sop.h",
       "plugins/adplug/adplug/surroundopl.cpp",
       "plugins/adplug/adplug/surroundopl.h",
       "plugins/adplug/adplug/temuopl.cpp",
       "plugins/adplug/adplug/temuopl.h",
       "plugins/adplug/adplug/u6m.cpp",
       "plugins/adplug/adplug/u6m.h",
       "plugins/adplug/adplug/version.h",
       "plugins/adplug/adplug/vgm.cpp",
       "plugins/adplug/adplug/vgm.h",
       "plugins/adplug/adplug/wemuopl.h",
       "plugins/adplug/adplug/woodyopl.cpp",
       "plugins/adplug/adplug/woodyopl.h",
       "plugins/adplug/adplug/xad.cpp",
       "plugins/adplug/adplug/xad.h",
       "plugins/adplug/adplug/xsm.cpp",
       "plugins/adplug/adplug/xsm.h",
       "plugins/adplug/adplug/adlibemu.c",
       "plugins/adplug/adplug/debug.c",
       "plugins/adplug/adplug/fmopl.c",
       "plugins/adplug/adplug/nukedopl.c"
   }

   defines { "stricmp=strcasecmp" }
   includedirs { "plugins/adplug/adplug", "plugins/adplug/libbinio" }
   links { "stdc++" }

project "alac_plugin"
   kind "SharedLib"
   language "C"
   targetdir "bin/%{cfg.buildcfg}/plugins"
   targetprefix ""
   targetname "alac"

   includedirs { "external/mp4p/include" }

   files {
       "plugins/alac/alac_plugin.c",
       "plugins/alac/alac.c",
       "plugins/alac/decomp.h",
       "shared/mp4tagutil.h",
       "shared/mp4tagutil.c"
   }

   links { "faad", "mp4p" }

project "flac_plugin"
   kind "SharedLib"
   language "C"
   targetdir "bin/%{cfg.buildcfg}/plugins"
   targetprefix ""
   targetname "flac"

   files {
       "plugins/flac/*.h",
       "plugins/flac/*.c"
   }

   defines { "HAVE_OGG_STREAM_FLUSH_FILL" }
   links { "FLAC", "ogg", "liboggedit" }

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
       "plugins/vorbis/*.c"
   }

   defines { "HAVE_OGG_STREAM_FLUSH_FILL" }
   links { "vorbisfile", "vorbis", "m", "ogg", "liboggedit" }

project "opus_plugin"
   kind "SharedLib"
   language "C"
   targetdir "bin/%{cfg.buildcfg}/plugins"
   targetprefix ""
   targetname "opus"

   files {
       "plugins/opus/*.h",
       "plugins/opus/*.c"
   }

   defines { "HAVE_OGG_STREAM_FLUSH_FILL" }
   links { "opusfile", "opus", "m", "ogg", "liboggedit" }
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
       "shared/eqpreset.c",
       "shared/eqpreset.h",
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
       "shared/eqpreset.c",
       "shared/eqpreset.h",
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

   includedirs { "external/mp4p/include" }

   files {
       "plugins/converter/converter.c",
       "shared/mp4tagutil.c",
   }
   links { "mp4p"}

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
       "shared/mp4tagutil.*",
   }

   excludes {
   }

   includedirs { "external/mp4p/include", "shared" }

   defines { "USE_OGG=1", "USE_VFS_CURL", "USE_METAFLAC" }
   links { "jpeg", "png", "z", "FLAC", "ogg", "mp4p" }

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

project "ddb_soundtouch"
   kind "SharedLib"
   language "C++"
   targetdir "bin/%{cfg.buildcfg}/plugins"
   targetprefix ""

   includedirs { "plugins/soundtouch/soundtouch/include" }

   files {
       "plugins/soundtouch/plugin.c",
       "plugins/soundtouch/st.cpp",
       "plugins/soundtouch/soundtouch/source/SoundTouch/AAFilter.cpp",
       "plugins/soundtouch/soundtouch/source/SoundTouch/BPMDetect.cpp",
       "plugins/soundtouch/soundtouch/source/SoundTouch/FIFOSampleBuffer.cpp",
       "plugins/soundtouch/soundtouch/source/SoundTouch/FIRFilter.cpp",
       "plugins/soundtouch/soundtouch/source/SoundTouch/InterpolateCubic.cpp",
       "plugins/soundtouch/soundtouch/source/SoundTouch/InterpolateLinear.cpp",
       "plugins/soundtouch/soundtouch/source/SoundTouch/InterpolateShannon.cpp",
       "plugins/soundtouch/soundtouch/source/SoundTouch/PeakFinder.cpp",
       "plugins/soundtouch/soundtouch/source/SoundTouch/RateTransposer.cpp",
       "plugins/soundtouch/soundtouch/source/SoundTouch/SoundTouch.cpp",
       "plugins/soundtouch/soundtouch/source/SoundTouch/SoundTouch.sln",
       "plugins/soundtouch/soundtouch/source/SoundTouch/SoundTouch.vcxproj",
       "plugins/soundtouch/soundtouch/source/SoundTouch/TDStretch.cpp",
       "plugins/soundtouch/soundtouch/source/SoundTouch/cpu_detect_x86.cpp",
       "plugins/soundtouch/soundtouch/source/SoundTouch/mmx_optimized.cpp",
       "plugins/soundtouch/soundtouch/source/SoundTouch/sse_optimized.cpp"
   }

project "tta"
   kind "SharedLib"
   language "C"
   targetdir "bin/%{cfg.buildcfg}/plugins"
   targetprefix ""

   files {
       "plugins/tta/ttaplug.c",
       "plugins/tta/filter.h",
       "plugins/tta/ttadec.c",
       "plugins/tta/ttadec.h"
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

