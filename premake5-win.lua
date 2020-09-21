include "premake5-tools.lua"

workspace "deadbeef"
   configurations { "debug", "release" }
   platforms { "Windows", "Linux" }
   toolset "clang"
   filter "system:Windows"
     defaultplatform "Windows"
   filter "system:not Windows"
     defaultplatform "Linux"

newoption {
  trigger = "version-override",
  description = "override version with today's date",
}

newoption {
    trigger = "debug-console",
    description = "Run deadbeef in console for debug output",
  }

newoption {
  trigger = "standard",
  description = "compile deadbeef with standard set of plugins for windows",
}

if _OPTIONS["standard"] ~= nil then
  plugins_to_disable = {"plugin-converter", "plugin-converter_gtk2",
                        "plugin-converter_gtk3","plugin-ffmpeg","plugin-waveout",
                        "plugin-wildmidi", "plugin-soundtouch", "plugin-sid", "plugin-gme",
                        "plugin-mms", "plugin-cdda", "plugin-medialib"}
  for i,v in ipairs(plugins_to_disable) do
    if _OPTIONS[v] == nil then
      _OPTIONS[v] = "disabled"
    end
  end
end

-- turn off order dependent linking (maybe fix later)
linkgroups 'On'

filter "configurations:debug"
  defines { "DEBUG" }
  symbols "On"

filter "configurations:release"
  buildoptions { "-O2" }

filter "configurations:debug or release"
  includedirs {
    "plugins/libmp4ff",
    "static-deps/lib-x86-64/include/x86_64-linux-gnu",
    "static-deps/lib-x86-64/include",
    "external/mp4p/include"
  }
  libdirs {
    "static-deps/lib-x86-64/lib/x86_64-linux-gnu",
    "static-deps/lib-x86-64/lib"
  }
  defines {
    "VERSION=\"" .. get_version() .. "\"",
    "_GNU_SOURCE",
    "HAVE_LOG2=1",
    "HAVE_ICONV=1"
  }
  if nls() then
    defines {"ENABLE_NLS"}
    defines {"PACKAGE=\"deadbeef\""}
  end
  -- set default options
  kind "SharedLib"
  language "C"
  targetdir "bin/%{cfg.buildcfg}/plugins"
  targetprefix ""


filter "platforms:Windows"
  buildoptions {
    "-include shared/windows/mingw32_layer.h",
    "-fno-builtin"
  }
  includedirs {
    "shared/windows/include",
    "/mingw64/include/opus",
    "xdispatch_ddb/include"
  }
  libdirs {
    "static-deps/lib-x86-64/lib/x86_64-linux-gnu",
    "static-deps/lib-x86-64/lib",
    "xdispatch_ddb/lib"
  }
  defines {
    "USE_STDIO",
    "HAVE_ICONV",
    "_POSIX_C_SOURCE"
  }
  if nls() then
    links {"intl"}
  end
  links {"ws2_32", "psapi", "shlwapi", "iconv", "libwin", "dl"}

-- clang preset in premake5 does not support icon compiling, define it here
filter 'files:**.rc'
  buildcommands {'windres -O coff -o "%{cfg.objdir}/%{file.basename}.o" "%{file.relpath}"'}
  buildoutputs {'%{cfg.objdir}/%{file.basename}.o'}

-- YASM compiling for ffap
filter 'files:**.asm'
  buildmessage 'YASM Assembling : %{file.relpath}'
  buildcommands {'mkdir -p obj/%{cfg.buildcfg}/ffap/'}
  buildcommands {'yasm -f elf -D ARCH_X86_64 -m amd64 -DPIC -DPREFIX -o "obj/%{cfg.buildcfg}/ffap/%{file.basename}.o" "%{file.relpath}"'}
  buildoutputs {"obj/%{cfg.buildcfg}/ffap/%{file.basename}.o"}

-- Libraries

project "libwin"
  kind "StaticLib"
  language "C"
  targetdir "bin/%{cfg.buildcfg}/"
  files {
    "shared/windows/fopen.c",
    "shared/windows/mingw32_layer.h",
    "shared/windows/mkdir.c",
    "shared/windows/rmdir.c",
    "shared/windows/rename.c",
    "shared/windows/scandir.c",
    "shared/windows/stat.c",
    "shared/windows/strcasestr.c",
    "shared/windows/strndup.c",
    "shared/windows/utils.c"
  }
  links {"dl"}
  removelinks {"libwin"}
  removeplatforms "Linux"

project "mp4p"
  kind "StaticLib"
  language "C"
  targetdir "bin/%{cfg.buildcfg}/"
  targetprefix ""
  files {
    "external/mp4p/src/*.c",
  }
  includedirs { "external/mp4p/include" }
  filter "platforms:not Windows"
    buildoptions {"-fPIC"}

project "liboggedit"
  kind "StaticLib"
  language "C"
  targetdir "bin/%{cfg.buildcfg}/"
  targetprefix ""
  files {
    "plugins/liboggedit/*.c",
    "plugins/liboggedit/*.h"
  }
  filter "platforms:not Windows"
    buildoptions {"-fPIC"}
-- DeaDBeeF

project "deadbeef"
  if (_OPTIONS["debug-console"]) then ddb_type = "ConsoleApp" else ddb_type = "WindowedApp" end
  kind (ddb_type)
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
    "shared/ctmap.c",
    "shared/ctmap.h"
  }
  defines {
    "PORTABLE=1",
    "STATICLINK=1",
    "PREFIX=\"donotuse\"",
    "LIBDIR=\"donotuse\"",
    "DOCDIR=\"donotuse\"",
    "LOCALEDIR=\"donotuse\""
  }
  links { "m", "pthread", "dl"}
  filter "system:Windows"
    files {
      "icons/deadbeef-icon.rc",
      "shared/windows/Resources.rc"
    }

local mp3_v = option ("plugin-mp3", "libmpg123", "mad")
if mp3_v then
project "mp3"
  files {
    "plugins/mp3/mp3.h",
    "plugins/mp3/mp3.c",
    "plugins/mp3/mp3parser.c"
  }
  if mp3_v["libmpg123"] then
    files {
      "plugins/mp3/mp3_mpg123.c",
      "plugins/mp3/mp3_mpg123.h"
    }
    defines { "USE_LIBMPG123=1" }
    links {"mpg123"}
  end
  if mp3_v["mad"] then
    files {
      "plugins/mp3/mp3_mad.c",
      "plugins/mp3/mp3_mad.h"
    }
    defines {"USE_LIBMAD=1"}
    links {"mad"}
   end
end

-- no pkgconfig for libfaad, do checking manually
if _OPTIONS["plugin-aac"] == "auto" or _OPTIONS["plugin-aac"] == nil then
  -- hard-coded :(
  -- todo: linuxify
  if os.outputof("ls /mingw64/include/neaacdec.h") == nil  then
    print ("\27[93m" .. "neaacdec.h not found in \"/mingw64/include/\", run premake5 with \"--plugin-aac=enabled\" to force enable aac plugin" ..  "\27[39m")
    _OPTIONS["plugin-aac"] = "disabled"
  else
    _OPTIONS["plugin-aac"] = "enabled"
  end
end

if option ("plugin-aac") then
project "aac_plugin"
  targetname "aac"
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
end

if option ("plugin-adplug") then
project "adplug_plugin"
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
  defines {"stricmp=strcasecmp"}
  includedirs {"plugins/adplug/adplug", "plugins/adplug/libbinio"}
  links {"stdc++"}
end

if option ("plugin-alac") then
project "alac_plugin"
  targetname "alac"
  files {
    "plugins/alac/alac_plugin.c",
    "plugins/alac/alac.c",
    "plugins/alac/decomp.h",
    "shared/mp4tagutil.h",
    "shared/mp4tagutil.c"
  }
  links {"faad", "mp4p"}
end

if option ("plugin-alsa", "alsa") then
project "alsa_plugin"
  targetname "alsa"
  removeplatforms {"Windows"}
  files {
    "plugins/alsa/*.c"
  }
  pkgconfig ("alsa")
end

local cdda_v = option ("plugin-cdda", "libcdio libcddb", "libcdio_paranoia")
if cdda_v and cdda_v["libcdio"] then
project "cdda_plugin"
  targetname "cdda"
  files {
    "plugins/cdda/cdda.c"
  }
  pkgconfig ("libcdio libcddb")
  filter "system:not Windows"
  if cdda_v["libcdio_paranoia"]then
    defines {"USE_PARANOIA"}
    pkgconfig ("libcdio_paranoia")
  end
end

if option ("plugin-dca") then
project "dca_plugin"
  targetname "dca"
  files {
    "plugins/dca/dcaplug.c",
    "plugins/dca/extract_dca.c",
    "plugins/dca/gettimeofday.c",
    "plugins/dca/parse.c",
    "plugins/dca/bitstream.c",
    "plugins/dca/downmix.c",
    "plugins/dca/audio_out.h",
    "plugins/dca/dca.h",
    "plugins/dca/dts.h",
    "plugins/dca/gettimeofday.h",
    "plugins/dca/tendra.h",
    "plugins/dca/dca_internal.h",
    "plugins/dca/tables_adpcm.h",
    "plugins/dca/tables_fir.h",
    "plugins/dca/tables.h",
    "plugins/dca/tables_huffman.h",
    "plugins/dca/tables_quantization.h",
    "plugins/dca/tables_vq.h",
    "plugins/dca/bitstream.h"
  }
  prebuildcommands {"touch plugins/dca/config.h"}
  postbuildcommands {"rm plugins/dca/config.h"}
end

if option ("plugin-dumb") then
project "dumb_plugin"
  targetname "dumb"
  files {
    "plugins/dumb/dumb-kode54/src/it/loadmod2.c",
    "plugins/dumb/dumb-kode54/src/it/itorder.c",
    "plugins/dumb/dumb-kode54/src/it/readokt2.c",
    "plugins/dumb/dumb-kode54/src/it/readptm.c",
    "plugins/dumb/dumb-kode54/src/it/loadxm.c",
    "plugins/dumb/dumb-kode54/src/it/ptmeffect.c",
    "plugins/dumb/dumb-kode54/src/it/loadany2.c",
    "plugins/dumb/dumb-kode54/src/it/loadokt2.c",
    "plugins/dumb/dumb-kode54/src/it/loadasy.c",
    "plugins/dumb/dumb-kode54/src/it/readasy.c",
    "plugins/dumb/dumb-kode54/src/it/loadpsm2.c",
    "plugins/dumb/dumb-kode54/src/it/readstm.c",
    "plugins/dumb/dumb-kode54/src/it/readxm2.c",
    "plugins/dumb/dumb-kode54/src/it/readmod.c",
    "plugins/dumb/dumb-kode54/src/it/readam.c",
    "plugins/dumb/dumb-kode54/src/it/loadoldpsm2.c",
    "plugins/dumb/dumb-kode54/src/it/loadxm2.c",
    "plugins/dumb/dumb-kode54/src/it/loadmod.c",
    "plugins/dumb/dumb-kode54/src/it/loadany.c",
    "plugins/dumb/dumb-kode54/src/it/loadmtm.c",
    "plugins/dumb/dumb-kode54/src/it/itrender.c",
    "plugins/dumb/dumb-kode54/src/it/loadasy2.c",
    "plugins/dumb/dumb-kode54/src/it/readpsm.c",
    "plugins/dumb/dumb-kode54/src/it/itload.c",
    "plugins/dumb/dumb-kode54/src/it/loadriff2.c",
    "plugins/dumb/dumb-kode54/src/it/itread.c",
    "plugins/dumb/dumb-kode54/src/it/loadmtm2.c",
    "plugins/dumb/dumb-kode54/src/it/loadriff.c",
    "plugins/dumb/dumb-kode54/src/it/readmtm.c",
    "plugins/dumb/dumb-kode54/src/it/reads3m.c",
    "plugins/dumb/dumb-kode54/src/it/itload2.c",
    "plugins/dumb/dumb-kode54/src/it/readxm.c",
    "plugins/dumb/dumb-kode54/src/it/loadpsm.c",
    "plugins/dumb/dumb-kode54/src/it/readany2.c",
    "plugins/dumb/dumb-kode54/src/it/loadamf.c",
    "plugins/dumb/dumb-kode54/src/it/loadptm2.c",
    "plugins/dumb/dumb-kode54/src/it/readokt.c",
    "plugins/dumb/dumb-kode54/src/it/itread2.c",
    "plugins/dumb/dumb-kode54/src/it/itmisc.c",
    "plugins/dumb/dumb-kode54/src/it/loadokt.c",
    "plugins/dumb/dumb-kode54/src/it/loads3m.c",
    "plugins/dumb/dumb-kode54/src/it/loadptm.c",
    "plugins/dumb/dumb-kode54/src/it/readdsmf.c",
    "plugins/dumb/dumb-kode54/src/it/readamf2.c",
    "plugins/dumb/dumb-kode54/src/it/itunload.c",
    "plugins/dumb/dumb-kode54/src/it/readmod2.c",
    "plugins/dumb/dumb-kode54/src/it/readamf.c",
    "plugins/dumb/dumb-kode54/src/it/readoldpsm.c",
    "plugins/dumb/dumb-kode54/src/it/loadamf2.c",
    "plugins/dumb/dumb-kode54/src/it/read6692.c",
    "plugins/dumb/dumb-kode54/src/it/read669.c",
    "plugins/dumb/dumb-kode54/src/it/readriff.c",
    "plugins/dumb/dumb-kode54/src/it/readany.c",
    "plugins/dumb/dumb-kode54/src/it/load669.c",
    "plugins/dumb/dumb-kode54/src/it/loadstm2.c",
    "plugins/dumb/dumb-kode54/src/it/loadstm.c",
    "plugins/dumb/dumb-kode54/src/it/load6692.c",
    "plugins/dumb/dumb-kode54/src/it/readstm2.c",
    "plugins/dumb/dumb-kode54/src/it/reads3m2.c",
    "plugins/dumb/dumb-kode54/src/it/loads3m2.c",
    "plugins/dumb/dumb-kode54/src/it/loadoldpsm.c",
    "plugins/dumb/dumb-kode54/src/it/xmeffect.c",
    "plugins/dumb/dumb-kode54/src/helpers/riff.c",
    "plugins/dumb/dumb-kode54/src/helpers/memfile.c",
    "plugins/dumb/dumb-kode54/src/helpers/silence.c",
    "plugins/dumb/dumb-kode54/src/helpers/stdfile.c",
    "plugins/dumb/dumb-kode54/src/helpers/clickrem.c",
    "plugins/dumb/dumb-kode54/src/helpers/tarray.c",
    "plugins/dumb/dumb-kode54/src/helpers/resample.c",
    "plugins/dumb/dumb-kode54/src/helpers/barray.c",
    "plugins/dumb/dumb-kode54/src/helpers/lpc.c",
    "plugins/dumb/dumb-kode54/src/helpers/resampler.c",
    "plugins/dumb/dumb-kode54/src/helpers/sampbuf.c",
    "plugins/dumb/dumb-kode54/src/core/unload.c",
    "plugins/dumb/dumb-kode54/src/core/loadduh.c",
    "plugins/dumb/dumb-kode54/src/core/duhlen.c",
    "plugins/dumb/dumb-kode54/src/core/register.c",
    "plugins/dumb/dumb-kode54/src/core/rawsig.c",
    "plugins/dumb/dumb-kode54/src/core/rendduh.c",
    "plugins/dumb/dumb-kode54/src/core/makeduh.c",
    "plugins/dumb/dumb-kode54/src/core/duhtag.c",
    "plugins/dumb/dumb-kode54/src/core/readduh.c",
    "plugins/dumb/dumb-kode54/src/core/dumbfile.c",
    "plugins/dumb/dumb-kode54/src/core/atexit.c",
    "plugins/dumb/dumb-kode54/src/core/rendsig.c",
    "plugins/dumb/dumb-kode54/src/tools/it/modulus.h",
    "plugins/dumb/dumb-kode54/include/internal/it.h",
    "plugins/dumb/dumb-kode54/include/internal/dumb.h",
    "plugins/dumb/dumb-kode54/include/internal/barray.h",
    "plugins/dumb/dumb-kode54/include/internal/tarray.h",
    "plugins/dumb/dumb-kode54/include/internal/resampler.h",
    "plugins/dumb/dumb-kode54/include/internal/riff.h",
    "plugins/dumb/dumb-kode54/include/internal/lpc.h",
    "plugins/dumb/dumb-kode54/include/internal/dumbfile.h",
    "plugins/dumb/dumb-kode54/include/internal/stack_alloc.h",
    "plugins/dumb/dumb-kode54/include/dumb.h",
    "plugins/dumb/dumb-kode54/src/helpers/resample.inc",
    "plugins/dumb/dumb-kode54/src/helpers/resamp2.inc",
    "plugins/dumb/dumb-kode54/src/helpers/resamp3.inc",
    "plugins/dumb/modloader.cpp",
    "plugins/dumb/modloader.h",
    "plugins/dumb/umr.h",
    "plugins/dumb/unmo3.h",
    "plugins/dumb/unrealfmt.cpp",
    "plugins/dumb/unrealfmtdata.cpp",
    "plugins/dumb/urf.h",
    "plugins/dumb/cdumb.c",
    "plugins/dumb/cdumb.h",
    "plugins/dumb/dumb-kode54/src/helpers/resampler_sse2.c"
  }
  includedirs {
    "plugins/dumb/dumb-kode54/include"
  }
  buildoptions {
    "-fno-exceptions",
    "-fno-rtti",
    "-fno-unwind-tables",
    "-msse2"
  }
  defines {"HAVE_SSE2"}
  links {"m", "stdc++"}
end

if option ("plugin-gme") then
project "gme_plugin"
  targetname "gme"
  files {
    "plugins/gme/cgme.c",
    "plugins/gme/gmewrap.cpp",
    "plugins/gme/gmewrap.h",
    "plugins/gme/game-music-emu-0.6pre/gme/Ay_Apu.cpp",
    "plugins/gme/game-music-emu-0.6pre/gme/Ay_Core.cpp",
    "plugins/gme/game-music-emu-0.6pre/gme/Ay_Cpu.cpp",
    "plugins/gme/game-music-emu-0.6pre/gme/Ay_Emu.cpp",
    "plugins/gme/game-music-emu-0.6pre/gme/Blip_Buffer.cpp",
    "plugins/gme/game-music-emu-0.6pre/gme/Bml_Parser.cpp",
    "plugins/gme/game-music-emu-0.6pre/gme/Classic_Emu.cpp",
    "plugins/gme/game-music-emu-0.6pre/gme/Data_Reader.cpp",
    "plugins/gme/game-music-emu-0.6pre/gme/Downsampler.cpp",
    "plugins/gme/game-music-emu-0.6pre/gme/Dual_Resampler.cpp",
    "plugins/gme/game-music-emu-0.6pre/gme/Effects_Buffer.cpp",
    "plugins/gme/game-music-emu-0.6pre/gme/Fir_Resampler.cpp",
    "plugins/gme/game-music-emu-0.6pre/gme/Gb_Apu.cpp",
    "plugins/gme/game-music-emu-0.6pre/gme/Gb_Cpu.cpp",
    "plugins/gme/game-music-emu-0.6pre/gme/Gb_Oscs.cpp",
    "plugins/gme/game-music-emu-0.6pre/gme/Gbs_Core.cpp",
    "plugins/gme/game-music-emu-0.6pre/gme/Gbs_Cpu.cpp",
    "plugins/gme/game-music-emu-0.6pre/gme/Gbs_Emu.cpp",
    "plugins/gme/game-music-emu-0.6pre/gme/Gme_File.cpp",
    "plugins/gme/game-music-emu-0.6pre/gme/Gme_Loader.cpp",
    "plugins/gme/game-music-emu-0.6pre/gme/Gym_Emu.cpp",
    "plugins/gme/game-music-emu-0.6pre/gme/Hes_Apu.cpp",
    "plugins/gme/game-music-emu-0.6pre/gme/Hes_Apu_Adpcm.cpp",
    "plugins/gme/game-music-emu-0.6pre/gme/Hes_Core.cpp",
    "plugins/gme/game-music-emu-0.6pre/gme/Hes_Cpu.cpp",
    "plugins/gme/game-music-emu-0.6pre/gme/Hes_Emu.cpp",
    "plugins/gme/game-music-emu-0.6pre/gme/Kss_Core.cpp",
    "plugins/gme/game-music-emu-0.6pre/gme/Kss_Cpu.cpp",
    "plugins/gme/game-music-emu-0.6pre/gme/Kss_Emu.cpp",
    "plugins/gme/game-music-emu-0.6pre/gme/Kss_Scc_Apu.cpp",
    "plugins/gme/game-music-emu-0.6pre/gme/M3u_Playlist.cpp",
    "plugins/gme/game-music-emu-0.6pre/gme/Multi_Buffer.cpp",
    "plugins/gme/game-music-emu-0.6pre/gme/Music_Emu.cpp",
    "plugins/gme/game-music-emu-0.6pre/gme/Nes_Apu.cpp",
    "plugins/gme/game-music-emu-0.6pre/gme/Nes_Cpu.cpp",
    "plugins/gme/game-music-emu-0.6pre/gme/Nes_Fds_Apu.cpp",
    "plugins/gme/game-music-emu-0.6pre/gme/Nes_Fme7_Apu.cpp",
    "plugins/gme/game-music-emu-0.6pre/gme/Nes_Namco_Apu.cpp",
    "plugins/gme/game-music-emu-0.6pre/gme/Nes_Oscs.cpp",
    "plugins/gme/game-music-emu-0.6pre/gme/Nes_Vrc6_Apu.cpp",
    "plugins/gme/game-music-emu-0.6pre/gme/Nes_Vrc7_Apu.cpp",
    "plugins/gme/game-music-emu-0.6pre/gme/Nsf_Core.cpp",
    "plugins/gme/game-music-emu-0.6pre/gme/Nsf_Cpu.cpp",
    "plugins/gme/game-music-emu-0.6pre/gme/Nsf_Emu.cpp",
    "plugins/gme/game-music-emu-0.6pre/gme/Nsf_Impl.cpp",
    "plugins/gme/game-music-emu-0.6pre/gme/Nsfe_Emu.cpp",
    "plugins/gme/game-music-emu-0.6pre/gme/Opl_Apu.cpp",
    "plugins/gme/game-music-emu-0.6pre/gme/Resampler.cpp",
    "plugins/gme/game-music-emu-0.6pre/gme/Rom_Data.cpp",
    "plugins/gme/game-music-emu-0.6pre/gme/SPC_Filter.cpp",
    "plugins/gme/game-music-emu-0.6pre/gme/Sap_Apu.cpp",
    "plugins/gme/game-music-emu-0.6pre/gme/Sap_Core.cpp",
    "plugins/gme/game-music-emu-0.6pre/gme/Sap_Cpu.cpp",
    "plugins/gme/game-music-emu-0.6pre/gme/Sap_Emu.cpp",
    "plugins/gme/game-music-emu-0.6pre/gme/Sgc_Core.cpp",
    "plugins/gme/game-music-emu-0.6pre/gme/Sgc_Cpu.cpp",
    "plugins/gme/game-music-emu-0.6pre/gme/Sgc_Emu.cpp",
    "plugins/gme/game-music-emu-0.6pre/gme/Sgc_Impl.cpp",
    "plugins/gme/game-music-emu-0.6pre/gme/Sms_Apu.cpp",
    "plugins/gme/game-music-emu-0.6pre/gme/Sms_Fm_Apu.cpp",
    "plugins/gme/game-music-emu-0.6pre/gme/Spc_Emu.cpp",
    "plugins/gme/game-music-emu-0.6pre/gme/Spc_Sfm.cpp",
    "plugins/gme/game-music-emu-0.6pre/gme/Track_Filter.cpp",
    "plugins/gme/game-music-emu-0.6pre/gme/Upsampler.cpp",
    "plugins/gme/game-music-emu-0.6pre/gme/Vgm_Core.cpp",
    "plugins/gme/game-music-emu-0.6pre/gme/Vgm_Emu.cpp",
    "plugins/gme/game-music-emu-0.6pre/gme/Ym2413_Emu.cpp",
    "plugins/gme/game-music-emu-0.6pre/gme/Ym2612_Emu.cpp",
    "plugins/gme/game-music-emu-0.6pre/gme/Ym2612_Emu_Gens.cpp",
    "plugins/gme/game-music-emu-0.6pre/gme/Ym2612_Emu_MAME.cpp",
    "plugins/gme/game-music-emu-0.6pre/gme/Z80_Cpu.cpp",
    "plugins/gme/game-music-emu-0.6pre/gme/blargg_common.cpp",
    "plugins/gme/game-music-emu-0.6pre/gme/blargg_errors.cpp",
    "plugins/gme/game-music-emu-0.6pre/gme/gme.cpp",
    "plugins/gme/game-music-emu-0.6pre/gme/higan/dsp/dsp.cpp",
    "plugins/gme/game-music-emu-0.6pre/gme/higan/dsp/SPC_DSP.cpp",
    "plugins/gme/game-music-emu-0.6pre/gme/higan/processor/spc700/algorithms.cpp",
    "plugins/gme/game-music-emu-0.6pre/gme/higan/processor/spc700/instructions.cpp",
    "plugins/gme/game-music-emu-0.6pre/gme/higan/processor/spc700/spc700.cpp",
    "plugins/gme/game-music-emu-0.6pre/gme/higan/smp/smp.cpp",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/ChipMapper.c",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/2151intf.c",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/2203intf.c",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/2413intf.c",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/2608intf.c",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/2610intf.c",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/2612intf.c",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/262intf.c",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/3526intf.c",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/3812intf.c",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/8950intf.c",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/adlibemu_opl2.c",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/adlibemu_opl3.c",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/ay8910.c",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/ay_intf.c",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/c140.c",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/c352.c",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/c6280.c",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/c6280intf.c",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/dac_control.c",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/emu2149.c",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/emu2413.c",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/es5503.c",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/es5506.c",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/fm.c",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/fm2612.c",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/fmopl.c",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/gb.c",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/iremga20.c",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/k051649.c",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/k053260.c",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/k054539.c",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/multipcm.c",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/nes_apu.c",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/nes_intf.c",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/np_nes_apu.c",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/np_nes_dmc.c",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/np_nes_fds.c",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/okim6258.c",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/okim6295.c",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/Ootake_PSG.c",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/panning.c",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/pokey.c",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/pwm.c",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/qsound.c",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/rf5c68.c",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/saa1099.c",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/scd_pcm.c",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/scsp.c",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/segapcm.c",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/sn76489.c",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/sn76496.c",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/sn764intf.c",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/upd7759.c",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/vsu.c",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/ws_audio.c",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/x1_010.c",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/ym2151.c",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/ym2413.c",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/ym2612.c",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/ymdeltat.c",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/ymf262.c",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/ymf271.c",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/ymf278b.c",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/ymz280b.c",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/yam.c",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/resampler.c",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/VGMPlay.c",
    "plugins/gme/game-music-emu-0.6pre/gme/Ay_Apu.h",
    "plugins/gme/game-music-emu-0.6pre/gme/Ay_Core.h",
    "plugins/gme/game-music-emu-0.6pre/gme/Ay_Emu.h",
    "plugins/gme/game-music-emu-0.6pre/gme/Blip_Buffer.h",
    "plugins/gme/game-music-emu-0.6pre/gme/Blip_Buffer_impl.h",
    "plugins/gme/game-music-emu-0.6pre/gme/Blip_Buffer_impl2.h",
    "plugins/gme/game-music-emu-0.6pre/gme/Bml_Parser.h",
    "plugins/gme/game-music-emu-0.6pre/gme/Classic_Emu.h",
    "plugins/gme/game-music-emu-0.6pre/gme/Data_Reader.h",
    "plugins/gme/game-music-emu-0.6pre/gme/Downsampler.h",
    "plugins/gme/game-music-emu-0.6pre/gme/Dual_Resampler.h",
    "plugins/gme/game-music-emu-0.6pre/gme/Effects_Buffer.h",
    "plugins/gme/game-music-emu-0.6pre/gme/Fir_Resampler.h",
    "plugins/gme/game-music-emu-0.6pre/gme/Gb_Apu.h",
    "plugins/gme/game-music-emu-0.6pre/gme/Gb_Cpu.h",
    "plugins/gme/game-music-emu-0.6pre/gme/Gb_Cpu_run.h",
    "plugins/gme/game-music-emu-0.6pre/gme/Gb_Oscs.h",
    "plugins/gme/game-music-emu-0.6pre/gme/Gbs_Core.h",
    "plugins/gme/game-music-emu-0.6pre/gme/Gbs_Emu.h",
    "plugins/gme/game-music-emu-0.6pre/gme/Gme_File.h",
    "plugins/gme/game-music-emu-0.6pre/gme/Gme_Loader.h",
    "plugins/gme/game-music-emu-0.6pre/gme/Gym_Emu.h",
    "plugins/gme/game-music-emu-0.6pre/gme/Hes_Apu.h",
    "plugins/gme/game-music-emu-0.6pre/gme/Hes_Apu_Adpcm.h",
    "plugins/gme/game-music-emu-0.6pre/gme/Hes_Core.h",
    "plugins/gme/game-music-emu-0.6pre/gme/Hes_Cpu.h",
    "plugins/gme/game-music-emu-0.6pre/gme/Hes_Cpu_run.h",
    "plugins/gme/game-music-emu-0.6pre/gme/Hes_Emu.h",
    "plugins/gme/game-music-emu-0.6pre/gme/Kss_Core.h",
    "plugins/gme/game-music-emu-0.6pre/gme/Kss_Emu.h",
    "plugins/gme/game-music-emu-0.6pre/gme/Kss_Scc_Apu.h",
    "plugins/gme/game-music-emu-0.6pre/gme/M3u_Playlist.h",
    "plugins/gme/game-music-emu-0.6pre/gme/Multi_Buffer.h",
    "plugins/gme/game-music-emu-0.6pre/gme/Music_Emu.h",
    "plugins/gme/game-music-emu-0.6pre/gme/Nes_Apu.h",
    "plugins/gme/game-music-emu-0.6pre/gme/Nes_Cpu.h",
    "plugins/gme/game-music-emu-0.6pre/gme/Nes_Cpu_run.h",
    "plugins/gme/game-music-emu-0.6pre/gme/Nes_Fds_Apu.h",
    "plugins/gme/game-music-emu-0.6pre/gme/Nes_Fme7_Apu.h",
    "plugins/gme/game-music-emu-0.6pre/gme/Nes_Mmc5_Apu.h",
    "plugins/gme/game-music-emu-0.6pre/gme/Nes_Namco_Apu.h",
    "plugins/gme/game-music-emu-0.6pre/gme/Nes_Oscs.h",
    "plugins/gme/game-music-emu-0.6pre/gme/Nes_Vrc6_Apu.h",
    "plugins/gme/game-music-emu-0.6pre/gme/Nes_Vrc7_Apu.h",
    "plugins/gme/game-music-emu-0.6pre/gme/Nsf_Core.h",
    "plugins/gme/game-music-emu-0.6pre/gme/Nsf_Emu.h",
    "plugins/gme/game-music-emu-0.6pre/gme/Nsf_Impl.h",
    "plugins/gme/game-music-emu-0.6pre/gme/Nsfe_Emu.h",
    "plugins/gme/game-music-emu-0.6pre/gme/Opl_Apu.h",
    "plugins/gme/game-music-emu-0.6pre/gme/Resampler.h",
    "plugins/gme/game-music-emu-0.6pre/gme/Rom_Data.h",
    "plugins/gme/game-music-emu-0.6pre/gme/Sap_Apu.h",
    "plugins/gme/game-music-emu-0.6pre/gme/Sap_Core.h",
    "plugins/gme/game-music-emu-0.6pre/gme/Sap_Emu.h",
    "plugins/gme/game-music-emu-0.6pre/gme/Sgc_Core.h",
    "plugins/gme/game-music-emu-0.6pre/gme/Sgc_Emu.h",
    "plugins/gme/game-music-emu-0.6pre/gme/Sgc_Impl.h",
    "plugins/gme/game-music-emu-0.6pre/gme/Sms_Apu.h",
    "plugins/gme/game-music-emu-0.6pre/gme/Sms_Fm_Apu.h",
    "plugins/gme/game-music-emu-0.6pre/gme/Spc_Emu.h",
    "plugins/gme/game-music-emu-0.6pre/gme/Spc_Filter.h",
    "plugins/gme/game-music-emu-0.6pre/gme/Spc_Sfm.h",
    "plugins/gme/game-music-emu-0.6pre/gme/Track_Filter.h",
    "plugins/gme/game-music-emu-0.6pre/gme/Upsampler.h",
    "plugins/gme/game-music-emu-0.6pre/gme/Vgm_Core.h",
    "plugins/gme/game-music-emu-0.6pre/gme/Vgm_Emu.h",
    "plugins/gme/game-music-emu-0.6pre/gme/Ym2413_Emu.h",
    "plugins/gme/game-music-emu-0.6pre/gme/Ym2612_Emu.h",
    "plugins/gme/game-music-emu-0.6pre/gme/Z80_Cpu.h",
    "plugins/gme/game-music-emu-0.6pre/gme/Z80_Cpu_run.h",
    "plugins/gme/game-music-emu-0.6pre/gme/adlib.h",
    "plugins/gme/game-music-emu-0.6pre/gme/blargg_common.h",
    "plugins/gme/game-music-emu-0.6pre/gme/blargg_config.h",
    "plugins/gme/game-music-emu-0.6pre/gme/blargg_endian.h",
    "plugins/gme/game-music-emu-0.6pre/gme/blargg_errors.h",
    "plugins/gme/game-music-emu-0.6pre/gme/blargg_source.h",
    "plugins/gme/game-music-emu-0.6pre/gme/gme.h",
    "plugins/gme/game-music-emu-0.6pre/gme/gme_custom_dprintf.h",
    "plugins/gme/game-music-emu-0.6pre/gme/higan/dsp/dsp.hpp",
    "plugins/gme/game-music-emu-0.6pre/gme/higan/dsp/SPC_DSP.h",
    "plugins/gme/game-music-emu-0.6pre/gme/higan/processor/spc700/memory.hpp",
    "plugins/gme/game-music-emu-0.6pre/gme/higan/processor/spc700/registers.hpp",
    "plugins/gme/game-music-emu-0.6pre/gme/higan/processor/spc700/spc700.hpp",
    "plugins/gme/game-music-emu-0.6pre/gme/higan/smp/smp.hpp",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/ChipMapper.h",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/2151intf.h",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/2203intf.h",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/2413intf.h",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/2413tone.h",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/2608intf.h",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/2610intf.h",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/2612intf.h",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/262intf.h",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/281btone.h",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/3526intf.h",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/3812intf.h",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/8950intf.h",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/adlibemu.h",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/ay8910.h",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/ay_intf.h",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/c140.h",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/c352.h",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/c6280.h",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/c6280intf.h",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/ChipIncl.h",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/dac_control.h",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/emu2149.h",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/emu2413.h",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/emutypes.h",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/es5503.h",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/es5506.h",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/fm.h",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/fmopl.h",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/gb.h",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/iremga20.h",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/k051649.h",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/k053260.h",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/k054539.h",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/mamedef.h",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/multipcm.h",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/nes_apu.h",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/nes_defs.h",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/nes_intf.h",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/np_nes_apu.h",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/np_nes_dmc.h",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/np_nes_fds.h",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/okim6258.h",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/okim6295.h",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/Ootake_PSG.h",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/opl.h",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/panning.h",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/pokey.h",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/pwm.h",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/qsound.h",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/rf5c68.h",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/saa1099.h",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/scd_pcm.h",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/scsp.h",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/segapcm.h",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/sn76489.h",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/sn76496.h",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/sn764intf.h",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/upd7759.h",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/vrc7tone.h",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/vsu.h",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/ws_audio.h",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/ws_initialIo.h",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/x1_010.h",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/ym2151.h",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/ym2413.h",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/ym2612.h",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/ymdeltat.h",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/ymf262.h",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/ymf271.h",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/ymf278b.h",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/ymz280b.h",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/chips/yam.h",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/resampler.h",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/stdbool.h",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/VGMFile.h",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/VGMPlay.h",
    "plugins/gme/game-music-emu-0.6pre/vgmplay/VGMPlay/VGMPlay_Intf.h",
  }
  includedirs {
    "plugins/gme/game-music-emu-0.6pre/gme/",
    "plugins/gme/game-music-emu-0.6pre"
  }
  defines {"GME_VERSION_055"}
  links {"stdc++", "z"}
  filter 'files:**.cpp'
    buildoptions {"-include cstdint"}
end

if option ("plugin-mms") then
project "mms_plugin"
  targetname "mms"
  files {
  "plugins/mms/mmsplug.c",
  "plugins/mms/libmms/mms.c",
  "plugins/mms/libmms/mmsh.c",
  "plugins/mms/libmms/mmsx.c",
  "plugins/mms/libmms/uri.c",
  "plugins/mms/libmms/asfheader.h",
  "plugins/mms/libmms/bswap.h",
  "plugins/mms/libmms/mms-common.h",
  "plugins/mms/libmms/mms.h",
  "plugins/mms/libmms/mmsh.h",
  "plugins/mms/libmms/mmsio.h",
  "plugins/mms/libmms/mmsx.h",
  "plugins/mms/libmms/uri.h"
  }
  includedirs {
    "plugins/mms/"
  }
end

if option ("plugin-notify", "dbus-1") then
project "notify_plugin"
  targetname "notify"
  files {
    "plugins/notify/notify.c"
  }
  pkgconfig ("dbus-1")
  buildoptions {"-fblocks"}
  links {"dispatch", "BlocksRuntime"}
end

if option ("plugin-oss", "PLUGIN_DISABLED") then
project "oss_plugin"
  targetname "oss"
  files {
    "plugins/oss/oss.c"
  }
end

if option ("plugin-pulse", "libpulse-simple") then
project "pulse_plugin"
  targetname "pulse"
  files {
    "plugins/pulse/pulse.c"
  }
end

if option ("plugin-sc68") then
project "sc68_plugin"
  targetname "sc68"
  files {
    "plugins/sc68/in_sc68.c",
    "plugins/sc68/file68/src/gzip68.c",
    "plugins/sc68/file68/src/vfs68.c",
    "plugins/sc68/file68/src/ice68.c",
    "plugins/sc68/file68/src/vfs68_ao.c",
    "plugins/sc68/file68/src/vfs68_null.c",
    "plugins/sc68/file68/src/replay.inc.h",
    "plugins/sc68/file68/src/msg68.c",
    "plugins/sc68/file68/src/timedb68.c",
    "plugins/sc68/file68/src/timedb.inc.h",
    "plugins/sc68/file68/src/init68.c",
    "plugins/sc68/file68/src/vfs68_z.c",
    "plugins/sc68/file68/src/replay68.c",
    "plugins/sc68/file68/src/registry68.c",
    "plugins/sc68/file68/src/uri68.c",
    "plugins/sc68/file68/src/rsc68.c",
    "plugins/sc68/file68/src/vfs68_fd.c",
    "plugins/sc68/file68/src/vfs68_file.c",
    "plugins/sc68/file68/src/vfs68_mem.c",
    "plugins/sc68/file68/src/file68.c",
    "plugins/sc68/file68/src/string68.c",
    "plugins/sc68/file68/src/endian68.c",
    "plugins/sc68/file68/src/vfs68_curl.c",
    "plugins/sc68/file68/src/option68.c",
    "plugins/sc68/file68/src/ferror68.c",
    "plugins/sc68/file68/sc68/file68_vfs_ao.h",
    "plugins/sc68/file68/sc68/file68.h",
    "plugins/sc68/file68/sc68/file68_ord.h",
    "plugins/sc68/file68/sc68/file68_tag.h",
    "plugins/sc68/file68/sc68/file68_vfs_curl.h",
    "plugins/sc68/file68/sc68/file68_str.h",
    "plugins/sc68/file68/sc68/file68_features.h",
    "plugins/sc68/file68/sc68/file68_vfs_def.h",
    "plugins/sc68/file68/sc68/file68_msg.h",
    "plugins/sc68/file68/sc68/file68_vfs_file.h",
    "plugins/sc68/file68/sc68/file68_api.h",
    "plugins/sc68/file68/sc68/file68_chk.h",
    "plugins/sc68/file68/sc68/file68_ice.h",
    "plugins/sc68/file68/sc68/file68_err.h",
    "plugins/sc68/file68/sc68/file68_opt.h",
    "plugins/sc68/file68/sc68/file68_vfs_mem.h",
    "plugins/sc68/file68/sc68/file68_rsc.h",
    "plugins/sc68/file68/sc68/file68_zip.h",
    "plugins/sc68/file68/sc68/file68_vfs_null.h",
    "plugins/sc68/file68/sc68/file68_tdb.h",
    "plugins/sc68/file68/sc68/file68_reg.h",
    "plugins/sc68/file68/sc68/file68_vfs_fd.h",
    "plugins/sc68/file68/sc68/file68_vfs.h",
    "plugins/sc68/file68/sc68/file68_uri.h",
    "plugins/sc68/file68/sc68/file68_vfs_z.h",
    "plugins/sc68/libsc68/dial68/dial68.h",
    "plugins/sc68/libsc68/dial68/dial68.c",
    "plugins/sc68/libsc68/dial68/dial_conf.c",
    "plugins/sc68/libsc68/dial68/dial_finf.c",
    "plugins/sc68/libsc68/dial68/dial_tsel.c",
    "plugins/sc68/unice68/unice68.h",
    "plugins/sc68/unice68/unice68_unpack.c",
    "plugins/sc68/unice68/unice68_pack.c",
    "plugins/sc68/unice68/unice68_version.c",
    "plugins/sc68/libsc68/conf68.c",
    "plugins/sc68/libsc68/api68.c",
    "plugins/sc68/libsc68/mixer68.c",
    "plugins/sc68/libsc68/io68/io68_api.h",
    "plugins/sc68/libsc68/io68/paulaemul.c",
    "plugins/sc68/libsc68/io68/io68.h",
    "plugins/sc68/libsc68/io68/shifter_io.h",
    "plugins/sc68/libsc68/io68/ym_envel.c",
    "plugins/sc68/libsc68/io68/mfp_io.h",
    "plugins/sc68/libsc68/io68/ym_dump.h",
    "plugins/sc68/libsc68/io68/ym_blep.h",
    "plugins/sc68/libsc68/io68/ymemul.c",
    "plugins/sc68/libsc68/io68/mwemul.h",
    "plugins/sc68/libsc68/io68/ymoutorg.h",
    "plugins/sc68/libsc68/io68/shifter_io.c",
    "plugins/sc68/libsc68/io68/paula_io.c",
    "plugins/sc68/libsc68/io68/mw_io.c",
    "plugins/sc68/libsc68/io68/mw_io.h",
    "plugins/sc68/libsc68/io68/mfpemul.h",
    "plugins/sc68/libsc68/io68/paula_io.h",
    "plugins/sc68/libsc68/io68/ym_dump.c",
    "plugins/sc68/libsc68/io68/mfp_io.c",
    "plugins/sc68/libsc68/io68/mwemul.c",
    "plugins/sc68/libsc68/io68/paulaemul.h",
    "plugins/sc68/libsc68/io68/ym_fixed_vol.h",
    "plugins/sc68/libsc68/io68/ym_io.c",
    "plugins/sc68/libsc68/io68/io68.c",
    "plugins/sc68/libsc68/io68/ym_puls.h",
    "plugins/sc68/libsc68/io68/ym_blep.c",
    "plugins/sc68/libsc68/io68/ymemul.h",
    "plugins/sc68/libsc68/io68/ymout2k9.h",
    "plugins/sc68/libsc68/io68/default.h",
    "plugins/sc68/libsc68/io68/mfpemul.c",
    "plugins/sc68/libsc68/io68/ym_io.h",
    "plugins/sc68/libsc68/io68/ym_puls.c",
    "plugins/sc68/libsc68/libsc68.c",
    "plugins/sc68/libsc68/emu68/inl68_datamove.h",
    "plugins/sc68/libsc68/emu68/lines68.c",
    "plugins/sc68/libsc68/emu68/lines68.h",
    "plugins/sc68/libsc68/emu68/inl68_bitmanip.h",
    "plugins/sc68/libsc68/emu68/inst68.h",
    "plugins/sc68/libsc68/emu68/inl68_progctrl.h",
    "plugins/sc68/libsc68/emu68/cc68.h",
    "plugins/sc68/libsc68/emu68/macro68.h",
    "plugins/sc68/libsc68/emu68/mem68.h",
    "plugins/sc68/libsc68/emu68/error68.h",
    "plugins/sc68/libsc68/emu68/ioplug68.c",
    "plugins/sc68/libsc68/emu68/inl68_logic.h",
    "plugins/sc68/libsc68/emu68/mem68.c",
    "plugins/sc68/libsc68/emu68/assert68.h",
    "plugins/sc68/libsc68/emu68/getea68.c",
    "plugins/sc68/libsc68/emu68/inl68_exception.h",
    "plugins/sc68/libsc68/emu68/inst68.c",
    "plugins/sc68/libsc68/emu68/emu68.c",
    "plugins/sc68/libsc68/emu68/srdef68.h",
    "plugins/sc68/libsc68/emu68/type68.h",
    "plugins/sc68/libsc68/emu68/struct68.h",
    "plugins/sc68/libsc68/emu68/error68.c",
    "plugins/sc68/libsc68/emu68/excep68.h",
    "plugins/sc68/libsc68/emu68/emu68.h",
    "plugins/sc68/libsc68/emu68/inl68_shifting.h",
    "plugins/sc68/libsc68/emu68/inl68_arithmetic.h",
    "plugins/sc68/libsc68/emu68/inl68_bcd.h",
    "plugins/sc68/libsc68/emu68/inl68_systctrl.h",
    "plugins/sc68/libsc68/emu68/emu68_api.h",
    "plugins/sc68/libsc68/emu68/ioplug68.h",
    "plugins/sc68/libsc68/sc68/trap68.h",
    "plugins/sc68/libsc68/sc68/conf68.h",
    "plugins/sc68/libsc68/sc68/sc68.h",
    "plugins/sc68/libsc68/sc68/mixer68.h",
    "plugins/sc68/desa68/desa68.h",
    "plugins/sc68/desa68/desa68.c"
  }
  includedirs {
    "plugins/sc68/file68",
    "plugins/sc68/file68/sc68",
    "plugins/sc68/unice68",
    "plugins/sc68/libsc68",
    "plugins/sc68/libsc68/sc68",
    "plugins/sc68/libsc68/emu68"
  }
  buildoptions {"-include stdint.h"}
  defines {
    "HAVE_GETENV=1",
    "HAVE_CONFIG_CONFIG68_H=1",
    "HAVE_STRING_H=1",
    "HAVE_STDLIB_H=1",
    "HAVE_STDINT_H=1",
    "HAVE_ASSERT_H",
    "HAVE_LIMITS_H=1",
    "HAVE_SYS_TYPES_H=1",
    "HAVE_BASENAME=1",
    "EMU68_MONOLITIC=1",
    "PACKAGE_VERNUM=300",
    "EMU68_EXPORT",
    "NDEBUG=1",
    "NDEBUG_LIBSC68=1",
    "USE_FILE68=1",
    "USE_UNICE68=1",
    "HAVE_INTMAX_T",
    "HAVE_GETENV=1",
    "HAVE_STDIO_H=1",
    "HAVE_MEMORY_H=1",
    "HAVE_MALLOC=1",
    "HAVE_FREE=1",
    "HAVE_LONG_LONG_INT=1",
    "HAVE_LONG_FILE_NAMES=1",
    "HAVE_LIMITS_H=1",
    "HAVE_LIBGEN_H=1",
    "HAVE_INTTYPES_H=1",
    "HAVE_DLFCN_H=1",
    "PACKAGE_STRING=\"libsc68 n/a\"",
    "PACKAGE_URL=\"http://sc68.atari.org\""
  }
  -- data missing?
end

if option ("plugin-shn") then
project "shn_plugin"
  targetname "shn"
  files {
    "plugins/shn/array.c",
    "plugins/shn/convert.c",
    "plugins/shn/misc.c",
    "plugins/shn/output.c",
    "plugins/shn/seek.c",
    "plugins/shn/shn.c",
    "plugins/shn/shn.h",
    "plugins/shn/shorten.c",
    "plugins/shn/shorten.h",
    "plugins/shn/sulawalaw.c",
    "plugins/shn/vario.c",
    "plugins/shn/wave.c",
    "plugins/shn/bitshift.h"
  }
  buildoptions {"-include stdint.h"}
end

if option ("plugin-vfs_zip", "libzip") then
project "vfs_zip"
  files {
    "plugins/vfs_zip/vfs_zip.c"
  }
  pkgconfig ("libzip")
end

if option ("plugin-vtx") then
project "vtx"
  files {
    "plugins/vtx/vtx.c",
    "plugins/vtx/ay8912.c",
    "plugins/vtx/ayemu_8912.h",
    "plugins/vtx/ayemu.h",
    "plugins/vtx/ayemu_vtxfile.h",
    "plugins/vtx/lh5dec.c",
    "plugins/vtx/vtxfile.c"
  }
  undefines {"VERSION"}
end

if option ("plugin-wma") then
project "wma"
  files {
    "plugins/wma/wma_plugin.c",
    "plugins/wma/asfheader.c",
    "plugins/wma/libasf/asf.c",
    "plugins/wma/libasf/asf.h",
    "plugins/wma/libwma/wmafixed.c",
    "plugins/wma/libwma/wmadeci.c",
    "plugins/wma/libwma/fft-ffmpeg.c",
    "plugins/wma/libwma/mdct_lookup.c",
    "plugins/wma/libwma/mdct.c",
    "plugins/wma/libwma/ffmpeg_bitstream.c",
    "plugins/wma/libwma/asm_arm.h",
    "plugins/wma/libwma/fft-ffmpeg_arm.h",
    "plugins/wma/libwma/types.h",
    "plugins/wma/libwma/codeclib_misc.h",
    "plugins/wma/libwma/fft.h",
    "plugins/wma/libwma/wmadata.h",
    "plugins/wma/libwma/ffmpeg_get_bits.h",
    "plugins/wma/libwma/mdct.h",
    "plugins/wma/libwma/wmadec.h",
    "plugins/wma/libwma/ffmpeg_intreadwrite.h",
    "plugins/wma/libwma/mdct_lookup.h",
    "plugins/wma/libwma/wmafixed.h",
  }
  includedirs {"plugins/wma"}
end

if option ("plugin-flac", "flac ogg") then
project "flac_plugin"
  targetname "flac"
  files {
    "plugins/flac/*.h",
    "plugins/flac/*.c"
  }
  defines {"HAVE_OGG_STREAM_FLUSH_FILL"}
  links {"FLAC", "ogg", "liboggedit"}
end

if option ("plugin-wavpack", "wavpack") then
project "wavpack_plugin"
  targetname "wavpack"
  files {
    "plugins/wavpack/*.h",
    "plugins/wavpack/*.c",
  }
  links {"wavpack"}
end

if option ("plugin-ffmpeg") then
project "ffmpeg"
  targetname "ffmpeg"
  files {
    "plugins/ffmpeg/*.h",
    "plugins/ffmpeg/*.c",
  }
  links {"avcodec", "pthread", "avformat", "avcodec", "avutil", "z", "opencore-amrnb", "opencore-amrwb", "opus"}
end

if option ("plugin-vorbis", "vorbisfile vorbis ogg") then
project "vorbis_plugin"
  targetname "vorbis"
  files {
    "plugins/vorbis/*.h",
    "plugins/vorbis/*.c"
  }
  defines {"HAVE_OGG_STREAM_FLUSH_FILL"}
  pkgconfig ("vorbisfile vorbis ogg")
  links {"m", "liboggedit"}
  -- fix linking order with liboggedit
  filter "system:Windows"
    links {"libwin"}
end

if option ("plugin-opus", "opusfile opus ogg") then
project "opus_plugin"
  targetname "opus"
  files {
    "plugins/opus/*.h",
    "plugins/opus/*.c"
  }
  defines {"HAVE_OGG_STREAM_FLUSH_FILL"}
  pkgconfig ("opusfile opus ogg")
  links {"m", "ogg", "liboggedit"}
  -- static deps
  filter "configurations:debug or release"
    includedirs {"static-deps/lib-x86-64/include/opus"}
  -- fix linking order with liboggedit
  filter "system:Windows"
    links {"libwin"}
end

if option ("plugin-ffap") then
project "ffap"
  files {
    "plugins/ffap/*.h",
    "plugins/ffap/*.c",
    "plugins/ffap/dsputil_yasm.asm",
  }
  defines {"APE_USE_ASM=yes", "ARCH_X86_64=1"}
end

if option ("plugin-hotkeys") then
project "hotkeys"
  files {
    "plugins/hotkeys/*.h",
    "plugins/hotkeys/*.c",
    "plugins/libparser/*.h",
    "plugins/libparser/*.c"
  }
  filter "system:not windows"
    links {"X11"}
end

if option ("plugin-dsp_libsrc", "samplerate") then
project "dsp_libsrc"
  files {
    "plugins/dsp_libsrc/src.c"
  }
  links {"samplerate"}
end

if option ("plugin-portaudio", "portaudio-2.0") then
project "portaudio"
  files {
    "plugins/portaudio/*.h",
    "plugins/portaudio/*.c"
  }
  buildoptions {"-fblocks"}
  links {"BlocksRuntime", "dispatch"}
  pkgconfig ("portaudio-2.0")
end

if option ("plugin-waveout") then
project "waveout"
  files {
    "plugins/waveout/*.h",
    "plugins/waveout/*.c"
  }
  links {"winmm", "ksuser"}
  removeplatforms {"Linux"}
end

if option ("plugin-gtk2", "gtk+-2.0 jansson") then
project "ddb_gui_GTK2"
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
    "utf8.c"
  }
  excludes {
    "plugins/gtkui/deadbeefapp.c",
    "plugins/gtkui/gtkui-gresources.c"
  }
  includedirs {
    "plugins/gtkui",
    "plugins/libparser"
  }
  pkgconfig ("gtk+-2.0 jansson")
  defines ("GLIB_DISABLE_DEPRECATION_WARNINGS")
end

if option ("plugin-gtk3", "gtk+-3.0 jansson") then
project "ddb_gui_GTK3"
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
    "utf8.c"
  }
  includedirs {
    "plugins/gtkui",
    "plugins/libparser"
  }

  prebuildcommands {
    "glib-compile-resources --sourcedir=plugins/gtkui --target=plugins/gtkui/gtkui-gresources.c --generate-source plugins/gtkui/gtkui.gresources.xml"
  }

  pkgconfig("gtk+-3.0 jansson")
  defines ("GLIB_DISABLE_DEPRECATION_WARNINGS")
end

if option ("plugin-rg_scanner") then
project "rg_scanner"
  files {
    "plugins/rg_scanner/*.h",
    "plugins/rg_scanner/*.c",
    "plugins/rg_scanner/ebur128/*.h",
    "plugins/rg_scanner/ebur128/*.c"
  }
end

if option ("plugin-converter") then
project "converter"
  files {
    "plugins/converter/converter.c",
    "shared/mp4tagutil.h",
    "shared/mp4tagutil.c"
  }
  links {"mp4p"}
end

if option ("plugin-shellexec", "jansson") then
project "shellexec"
  files {
    "plugins/shellexec/shellexec.c",
    "plugins/shellexec/shellexec.h",
    "plugins/shellexec/shellexecutil.c",
    "plugins/shellexec/shellexecutil.h"
  }
  pkgconfig("jansson")
end

if option ("plugin-sndfile", "sndfile") then
project "sndfile_plugin"
  targetname "sndfile"
  files {
    "plugins/sndfile/*.c",
    "plugins/sndfile/*.h",
  }
  links {"sndfile"}
end

if option ("plugin-sid") then
project "sid"
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
    "PACKAGE=\"libsidplay2\""
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
  links {"stdc++"}
end

if option ("plugin-psf", "zlib") then
project "psf"
  includedirs {
    "plugins/psf",
    "plugins/psf/eng_ssf",
    "plugins/psf/eng_qsf",
    "plugins/psf/eng_dsf"
  }
  defines {
    "HAS_PSXCPU=1"
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
  links {"z", "m"}
end

if option ("plugin-m3u") then
project "m3u"
  files {
    "plugins/m3u/*.c",
    "plugins/m3u/*.h",
  }
end

if option ("plugin-vfs_curl", "libcurl") then
project "vfs_curl"
  files {
    "plugins/vfs_curl/*.c",
    "plugins/vfs_curl/*.h",
  }
  links {"curl"}
end

if option ("plugin-converter_gtk2", "gtk+-2.0") then
project "converter_gtk2"
  files {
    "plugins/converter/convgui.c",
    "plugins/converter/callbacks.c",
    "plugins/converter/interface.c",
    "plugins/converter/support.c"
  }
  pkgconfig ("gtk+-2.0")
end

if option ("plugin-converter_gtk3", "gtk+-3.0") then
project "converter_gtk3"
  files {
    "plugins/converter/convgui.c",
    "plugins/converter/callbacks.c",
    "plugins/converter/interface.c",
    "plugins/converter/support.c"
  }
  includedirs {
    "plugins/gtkui",
    "plugins/libparser"
  }
  pkgconfig ("gtk+-3.0")
end

if option ("plugin-pltbrowser_gtk2", "gtk+-2.0") then
project "pltbrowser_gtk2"
  files {
    "plugins/pltbrowser/pltbrowser.c",
    "plugins/pltbrowser/support.c"
  }
  includedirs {
    "plugins/gtkui",
    "plugins/libparser"
  }
  pkgconfig ("gtk+-2.0")
end

if option ("plugin-pltbrowser_gtk3", "gtk+-3.0") then
project "pltbrowser_gtk3"
  files {
    "plugins/pltbrowser/pltbrowser.c",
    "plugins/pltbrowser/support.c"
  }
  includedirs {
    "plugins/gtkui",
    "plugins/libparser"
  }
  pkgconfig ("gtk+-3.0")
end

if option ("plugin-shellexecui_gtk2", "gtk+-2.0 jansson") then
project "shellexecui_gtk2"
  files {
    "plugins/shellexecui/shellexecui.c",
    "plugins/shellexecui/interface.c",
    "plugins/shellexecui/support.c",
    "plugins/shellexecui/callbacks.c",
    "plugins/shellexecui/interface.h",
    "plugins/shellexecui/support.h",
    "plugins/shellexecui/callbacks.h"
  }
  pkgconfig ("gtk+-2.0 jansson")
end

if option ("plugin-shellexecui_gtk3", "gtk+-3.0 jansson") then
project "shellexecui_gtk3"
  files {
    "plugins/shellexecui/shellexecui.c",
    "plugins/shellexecui/interface.c",
    "plugins/shellexecui/support.c",
    "plugins/shellexecui/callbacks.c",
    "plugins/shellexecui/interface.h",
    "plugins/shellexecui/support.h",
    "plugins/shellexecui/callbacks.h"
  }
  pkgconfig ("gtk+-3.0 jansson")
end

if option ("plugin-wildmidi") then
project "wildmidi_plugin"
  targetname "wildmidi"
  files {
    "plugins/wildmidi/*.h",
    "plugins/wildmidi/*.c",
    "plugins/wildmidi/src/*.h",
    "plugins/wildmidi/src/*.c"
  }
  excludes {
    "plugins/wildmidi/src/wildmidi.c"
  }
  includedirs { "plugins/wildmidi/include" }
  defines {"WILDMIDI_VERSION=\"0.2.2\"", "WILDMIDILIB_VERSION=\"0.2.2\"", "TIMIDITY_CFG=\"/etc/timidity.conf\""}
  links {"m"}
end

if option ("plugin-musepack") then
project "musepack_plugin"
  targetname "musepack"
  files {
    "plugins/musepack/musepack.c",
    "plugins/musepack/huffman.c",
    "plugins/musepack/mpc_bits_reader.c",
    "plugins/musepack/mpc_decoder.c",
    "plugins/musepack/mpc_demux.c",
    "plugins/musepack/mpc_reader.c",
    "plugins/musepack/requant.c",
    "plugins/musepack/streaminfo.c",
    "plugins/musepack/synth_filter.c",
    "plugins/musepack/crc32.c",
    "plugins/musepack/decoder.h",
    "plugins/musepack/huffman.h",
    "plugins/musepack/internal.h",
    "plugins/musepack/mpc_bits_reader.h",
    "plugins/musepack/mpc/mpcdec.h",
    "plugins/musepack/mpcdec_math.h",
    "plugins/musepack/mpc/reader.h",
    "plugins/musepack/requant.h",
    "plugins/musepack/mpc/streaminfo.h",
    "plugins/musepack/mpc/mpc_types.h",
    "plugins/musepack/mpc/minimax.h"
  }
  includedirs {"plugins/musepack"}
  links {"m"}
end

if option ("plugin-artwork", "libjpeg libpng zlib flac ogg") then
project "artwork_plugin"
  targetname "artwork"
  files {
    "plugins/artwork-legacy/*.c",
    "shared/mp4tagutil.h",
    "shared/mp4tagutil.c"
  }
  includedirs {"../libmp4ff"}
  defines {"USE_OGG=1", "USE_VFS_CURL", "USE_METAFLAC", "USE_MP4FF", "USE_TAGGING=1"}
  links {"jpeg", "png", "z", "FLAC", "ogg", "mp4p"}
end

if option ("plugin-supereq") then
project "supereq_plugin"
  targetname "supereq"
  files {
    "plugins/supereq/*.c",
    "plugins/supereq/*.cpp"
  }
  defines {"USE_OOURA"}
  links {"m", "stdc++"}
end

if option ("plugin-mono2stereo") then
project "ddb_mono2stereo"
  files {
    "plugins/mono2stereo/*.c"
  }
end

if option ("plugin-nullout") then
project "nullout"
  files {
    "plugins/nullout/*.h",
    "plugins/nullout/*.c"
  }
end

if option ("plugin-lastfm", "libcurl") then
project "lastfm"
  files {
    "plugins/lastfm/*.h",
    "plugins/lastfm/*.c"
  }
  buildoptions {"-fblocks"}
  links {"dispatch", "BlocksRuntime"}
  pkgconfig ("libcurl")
end

if option ("plugin-soundtouch") then
project "ddb_soundtouch"
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
end

if option ("plugin-tta") then
project "tta"
  files {
    "plugins/tta/ttaplug.c",
    "plugins/tta/filter.h",
    "plugins/tta/ttadec.c",
    "plugins/tta/ttadec.h"
  }
end

if option ("plugin-medialib") then
project "medialib"
  files {
    "plugins/medialib/medialib.c",
    "plugins/medialib/medialib.h"
  }
  pkgconfig ("jansson")
end

project "translations"
  kind "Utility"
  files {
    "po/*.po"
  }
  removelinks {"libwin"}
  filter 'files:**.po'
    buildmessage 'msgfmt -o "%{file.directory}/%{file.basename}.gmo" "%{file.relpath}"'
    buildcommands {'msgfmt -o "%{file.directory}/%{file.basename}.gmo" "%{file.relpath}"'}
    buildoutputs {'%{file.directory}/%{file.basename}.gmo'}

project "resources"
  kind "Utility"
  files {"main.c"}
  filter 'files:**.c'
    buildcommands {
      "mkdir -p bin/%{cfg.buildcfg}/pixmaps",
      "cp icons/32x32/deadbeef.png bin/%{cfg.buildcfg}",
      "cp pixmaps/*.png bin/%{cfg.buildcfg}/pixmaps/",
      "mkdir -p bin/%{cfg.buildcfg}/plugins/convpresets",
      "cp -r plugins/converter/convpresets bin/%{cfg.buildcfg}/plugins/",
    }
    buildoutputs {'%{file.directory}/%{file.basename}.c_fake'}

project "resources_windows"
  kind "Utility"
  removeplatforms {"Linux"}
  dependson {"translations", "ddb_gui_GTK3", "ddb_gui_GTK2"}
  files {"main.c"}
  filter 'files:**.c'
    buildcommands {"./scripts/windows_postbuild.sh bin/%{cfg.buildcfg}"}
    buildoutputs {'%{cfg.objdir}/%{file.basename}.c_fake'}

-- Done with projects, print current build settings:
print_options()
