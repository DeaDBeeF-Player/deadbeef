workspace "deadbeef"
   configurations { "debug", "release", "debug32", "release32" }
   platforms { "Linux", "Windows" }
   defaultplatform "Linux"
defines {
    "VERSION=\"devel\"",
    "_GNU_SOURCE",
    "HAVE_LOG2=1"
}

filter "configurations:debug or debug32"
  defines { "DEBUG" }
  symbols "On"

filter "configurations:debug or release"
  buildoptions { "-fPIC" }
  includedirs { "plugins/libmp4ff", "static-deps/lib-x86-64/include/x86_64-linux-gnu", "static-deps/lib-x86-64/include"  }
  libdirs { "static-deps/lib-x86-64/lib/x86_64-linux-gnu", "static-deps/lib-x86-64/lib" }


filter "configurations:debug32 or release32"
  buildoptions { "-std=c99", "-m32" }
  linkoptions { "-m32" }
  includedirs { "plugins/libmp4ff", "static-deps/lib-x86-32/include/i386-linux-gnu", "static-deps/lib-x86-32/include"  }
  libdirs { "static-deps/lib-x86-32/lib/i386-linux-gnu", "static-deps/lib-x86-32/lib" }

filter "configurations:release32 or release"
  buildoptions { "-O2" }

filter "system:Windows"
  buildoptions { "-include shared/windows/mingw32_layer.h", "-fno-builtin"}
  includedirs { "shared/windows/include", "/mingw64/include/opus" }
  libdirs { "static-deps/lib-x86-64/lib/x86_64-linux-gnu", "static-deps/lib-x86-64/lib" }
  defines { "USE_STDIO", "HAVE_ICONV", "ENABLE_NLS", "PACKAGE=\"deadbeef\"", "_POSIX_C_SOURCE" }

  links { "ws2_32", "psapi", "shlwapi", "iconv", "intl", "libwin", "dl"}

project "libwin"
   kind "StaticLib"
   language "C"
   targetdir "bin/%{cfg.buildcfg}/"
   files {
       "shared/windows/mingw32_layer.h",
       "shared/windows/fopen.c",
       "shared/windows/junk_iconv2.c",
       "shared/windows/path_short.c",
       "shared/windows/scandir.c",
       "shared/windows/stat.c",
       "shared/windows/strcasestr.c",
       "shared/windows/strndup.c",
       "shared/windows/rename.c"
   }
   links {"dl"}
   removelinks {"libwin"}

function pkgconfig (pkgname)
  links { pkgconfig_libs (pkgname) }
  includedirs { pkgconfig_includedirs (pkgname) }
  libdirs { pkgconfig_libdirs (pkgname) }
end

function pkgconfig_includedirs (pkgname)
  command = "pkg-config --cflags-only-I " .. pkgname
  returnval = os.outputof (command)
  if (returnval == nil)
  then
    error ("pkg-config failed for " .. pkgname)
  end
  parts = string.explode(returnval, " ")
  tab2 = {}
  for i, v in ipairs(parts) do
    tab2[i] = string.sub (v, 3)
  end
  return tab2
end

function pkgconfig_libdirs (pkgname)
  command = "pkg-config --libs-only-L " .. pkgname
  returnval = os.outputof (command)
  if (returnval == nil)
  then
    error ("pkg-config failed for " .. pkgname)
  end
  parts = string.explode (returnval, " ")
  tab2 = {}
  for i, v in ipairs(parts) do
    tab2[i] = string.sub (v, 3)
  end
  return tab2
end

function pkgconfig_libs (pkgname)
  command = "pkg-config --libs-only-l " .. pkgname
  returnval = os.outputof (command)
  if (returnval == nil)
  then
    error ("pkg-config failed for " .. pkgname)
  end
  parts = string.explode (returnval, " ")
  tab2 = {}
  for i, v in ipairs(parts) do
    tab2[i] = string.sub (v, 3)
  end
  return tab2
end

project "deadbeef"
   kind "WindowedApp"
   language "C"
   targetdir "bin/%{cfg.buildcfg}"

   files {
       "*.h",
       "*.c",
       "md5/*.h",
       "md5/*.c",
       "plugins/libparser/*.h",
       "plugins/libparser/*.c",
       -- "ConvertUTF/*.h",
       -- "ConvertUTF/*.c"
   }
   filter "system:Windows"
      files {
        "icons/deadbeef-icon.rc"
      }

   defines { "PORTABLE=1", "STATICLINK=1", "PREFIX=\"donotuse\"", "LIBDIR=\"donotuse\"", "DOCDIR=\"donotuse\"", "LOCALEDIR=\"donotuse\"", "HAVE_ICONV" }
   links { "m", "pthread", "dl", "iconv" }

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
   filter "system:not windows"
       links { "X11" }

project "alsa"
   removeplatforms { "Windows" }
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
   removeplatforms { "Windows" }
   kind "SharedLib"
   language "C"
   targetdir "bin/%{cfg.buildcfg}/plugins"
   targetprefix ""

   files {
       "plugins/pulse/*.h",
       "plugins/pulse/*.c",
   }

   links { "pulse-simple" }

project "portaudio"
   kind "SharedLib"
   language "C"
   targetdir "bin/%{cfg.buildcfg}/plugins"
   targetprefix ""

   files {
       "plugins/portaudio/*.h",
       "plugins/portaudio/*.c",
   }

   links { "portaudio " } -- space needed to avoid linking with itself, lol

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

    pkgconfig ("gtk+-2.0 jansson")

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

   pkgconfig("gtk+-3.0 jansson")
   -- links { "jansson", "gtk-3", "gdk-3", "pangocairo-1.0", "pango-1.0", "atk-1.0", "cairo-gobject", "cairo", "gdk_pixbuf-2.0", "gio-2.0", "gobject-2.0", "gthread-2.0", "glib-2.0" }

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
   removeplatforms { "Windows" }
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

   links { "curl" }

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

   pkgconfig ("gtk+-2.0")
   -- links { "gtk-x11-2.0", "pango-1.0", "cairo", "gdk-x11-2.0", "gdk_pixbuf-2.0", "gobject-2.0", "gthread-2.0", "glib-2.0" }

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


project "translations"
   kind "Utility"
   language "C"
   targetprefix ""

   prebuildcommands {
      "for i in po/*.po ; do (echo $$i ; msgfmt $$i -o po/`basename $$i .po`.gmo ) ; done",
   }


project "resources"
    kind "Utility"
    postbuildcommands {
        "{MKDIR} bin/%{cfg.buildcfg}/pixmaps",
        "{COPY} icons/32x32/deadbeef.png bin/%{cfg.buildcfg}",
        "{COPY} pixmaps/*.png pixmaps/*.svg bin/%{cfg.buildcfg}/pixmaps/",
        "{MKDIR} bin/%{cfg.buildcfg}/plugins/convpresets",
        "{COPY} plugins/converter/convpresets bin/%{cfg.buildcfg}/plugins/",
    }

project "resources_windows"
    kind "Utility"
    postbuildcommands {
        -- misc
        "{MKDIR} bin/%{cfg.buildcfg}/plugins",
        "{MKDIR} bin/%{cfg.buildcfg}/pixmaps",
        "{MKDIR} bin/%{cfg.buildcfg}/doc",
        "{MKDIR} bin/%{cfg.buildcfg}/share/themes bin/%{cfg.buildcfg}/share/icons",
        "{MKDIR} bin/%{cfg.buildcfg}/config",
        "{COPY} ChangeLog help.txt COPYING.GPLv2 COPYING.LGPLv2.1 about.txt translators.txt  bin/%{cfg.buildcfg}/doc/",
        -- translations
        "{MKDIR} bin/%{cfg.buildcfg}/locale",
        "for i in po/*.gmo ; do (base=`basename $$i .gmo` ; mkdir -p bin/%{cfg.buildcfg}/locale/$$base/LC_MESSAGES ; cp $$i bin/%{cfg.buildcfg}/locale/$$base/LC_MESSAGES/deadbeef.mo) ; done",
        "{COPY} translation/help.ru.txt  bin/%{cfg.buildcfg}/doc/",
        -- libraries
        "rm  bin/%{cfg.buildcfg}/plugins/*.lib | true",
        "rm  bin/%{cfg.buildcfg}/libwin.lib | true",
        "ldd bin/%{cfg.buildcfg}/plugins/*.dll bin/%{cfg.buildcfg}/deadbeef.exe | awk \'NF == 4 {print $$3}; NF == 2 {print $$1}\' |grep -i -v \"System32\" | grep -i -v \"WinSxS\" |sort -u | tr \'\\r\\n\' \' \'> .libraries.tmp",
        "{COPY} `cat .libraries.tmp` bin/%{cfg.buildcfg}/ | true",
        -- gtk2 theme
        "{MKDIR} bin/%{cfg.buildcfg}/lib/gtk-2.0/2.10.0/engines",
        "for i in /mingw32 /mingw64 /usr; do (cp -r $$i/share/themes/MS-Windows bin/%{cfg.buildcfg}/share/themes/ 2>>/dev/null ; cp $$i/lib/gtk-2.0/2.10.0/engines/libwimp.dll bin/%{cfg.buildcfg}/lib/gtk-2.0/2.10.0/engines 2>>/dev/null ); done; true",
        "{MKDIR} bin/%{cfg.buildcfg}/etc bin/%{cfg.buildcfg}/etc/gtk-2.0",
        "{TOUCH} bin/%{cfg.buildcfg}/etc/gtk-2.0/settings.ini",
        "echo -e \"[Settings]\\r\\ngtk-theme-name = MS-Windows\\n\" > bin/%{cfg.buildcfg}/etc/gtk-2.0/settings.ini",
        -- gtk3 misc
        "{MKDIR} bin/%{cfg.buildcfg}/etc bin/%{cfg.buildcfg}/etc/gtk-3.0",
        "{TOUCH} bin/%{cfg.buildcfg}/etc/gtk-3.0/settings.ini",
        "echo -e \"[Settings]\\r\\ngtk-theme-name = Windows-10\\r\\ngtk-icon-theme-name = Windows-10-Icons\" > bin/%{cfg.buildcfg}/etc/gtk-3.0/settings.ini",
        "for i in /mingw32 /mingw64 /usr; do (cp -r $$i/share/icons/hicolor bin/%{cfg.buildcfg}/share/icons/ ; cp -r $$i/share/glib-2.0 bin/%{cfg.buildcfg}/share/ ); done; true",
        -- Windows-10 theme and icons can be obtained from https://github.com/B00merang-Project/Windows-10 and https://github.com/B00merang-Project/Windows-10-Icons)
        "for i in /mingw32 /mingw64 /usr; do (cp -r $$i/share/icons/Windows-10-Icons bin/%{cfg.buildcfg}/share/icons/ 2>>/dev/null ; cp -r $$i/share/themes/Windows-10 bin/%{cfg.buildcfg}/share/themes/ 2>>/dev/null ); done; true",
        "echo \"output_plugin PortAudio output plugin\" > bin/%{cfg.buildcfg}/config/config"
    }

