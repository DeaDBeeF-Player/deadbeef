include "premake5-tools.lua"

workspace "deadbeef"
   configurations { "debug", "release" }
   platforms { "Windows", "Linux" }
   toolset "clang"
   filter "system:Windows"
     defaultplatform "Windows"
     removeplatforms "Linux"
   filter "system:not Windows"
     defaultplatform "Linux"
     removeplatforms "Windows"

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
                        "plugin-converter_gtk3", "plugin-waveout",
                        "plugin-wildmidi", "plugin-soundtouch", "plugin-sid", "plugin-gme",
                        "plugin-mms", "plugin-cdda", "plugin-sc68", "plugin-vtx",
                        "plugin-notify"}
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
    "include",
    ".",
    "static-deps/lib-x86-64/include/x86_64-linux-gnu",
    "static-deps/lib-x86-64/include",
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
  WINDOWS_SYSTEM_PREFIX = (os.getenv("MSYSTEM_PREFIX") or '')
  buildoptions {
    "-include shared/windows/mingw32_layer.h",
    "-fno-builtin"
  }
  linkoptions {
    "-Wl,--export-all-symbols"
  }
  includedirs {
    "shared/windows/include",
    WINDOWS_SYSTEM_PREFIX .. "/include/opus",
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
  buildcommands {'windres --define VERSION=\"' .. get_version() .. '\" -O coff -o "%{cfg.objdir}/%{file.basename}.o" "%{file.relpath}"'}
  buildoutputs {'%{cfg.objdir}/%{file.basename}.o'}

-- YASM compiling for ffap
filter 'files:**.asm'
  buildmessage 'YASM Assembling : %{file.relpath}'
  buildcommands {'mkdir -p obj/%{cfg.buildcfg}/ffap/'}
  buildcommands {'yasm -f win64 -D ARCH_X86_64 -m amd64 -DPIC -DPREFIX -o "obj/%{cfg.buildcfg}/ffap/%{file.basename}.o" "%{file.relpath}"'}
  buildoutputs {"obj/%{cfg.buildcfg}/ffap/%{file.basename}.o"}

-- Libraries

project "libwin"
  kind "StaticLib"
  language "C"
  targetdir "."
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
  targetdir "."
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
  targetdir "."
  targetprefix ""
  files {
    "plugins/liboggedit/*.c"
  }
  filter "platforms:not Windows"
    buildoptions {"-fPIC"}

project "libdeletefromdisk"
  kind "StaticLib"
  language "C"
  targetdir "."
  targetprefix ""
  files {
    "shared/deletefromdisk.c"
  }
  filter "platforms:not Windows"
    buildoptions {"-fPIC"}

project "libtftintutil"
  kind "StaticLib"
  language "C"
  targetdir "."
  targetprefix ""
  files {
    "shared/tftintutil.c"
  }
  filter "platforms:not Windows"
    buildoptions {"-fPIC"}

-- DeaDBeeF

project "deadbeef"
  if (_OPTIONS["debug-console"]) then ddb_type = "ConsoleApp" else ddb_type = "WindowedApp" end
  kind (ddb_type)
  targetdir "bin/%{cfg.buildcfg}"
  files {
    "src/md5/*.c",
    "src/undo/*.c",
    "shared/undo/*.c",
    "shared/filereader/*.c",
    "src/*.c",
    "plugins/libparser/*.c",
    "external/wcwidth/wcwidth.c",
    "shared/ctmap.c",
  }
  includedirs {
    "shared"
  }
  defines {
    "PORTABLE=1",
    "STATICLINK=1",
    "PREFIX=\"donotuse\"",
    "LIBDIR=\"donotuse\"",
    "DOCDIR=\"donotuse\"",
    "LOCALEDIR=\"donotuse\""
  }
  buildoptions {"-fblocks"}
  links { "m", "pthread", "dl","dispatch", "BlocksRuntime"}
  filter "platforms:Windows"
    files {
      "icons/deadbeef-icon.rc",
      "shared/windows/Resources.rc"
    }

local mp3_v = option ("plugin-mp3", "libmpg123", "mad")
if mp3_v then
project "mp3"
  files {
    "plugins/mp3/mp3.c",
    "plugins/mp3/mp3parser.c"
  }
  if mp3_v["libmpg123"] then
    files {
      "plugins/mp3/mp3_mpg123.c"
    }
    defines { "USE_LIBMPG123=1" }
    links {"mpg123"}
  end
  if mp3_v["mad"] then
    files {
      "plugins/mp3/mp3_mad.c"
    }
    defines {"USE_LIBMAD=1"}
    links {"mad"}
   end
end

-- no pkgconfig for libfaad, do checking manually
if _OPTIONS["plugin-aac"] == "auto" or _OPTIONS["plugin-aac"] == nil then
  -- hard-coded :(
  -- todo: linuxify
  if os.outputof("ls " .. WINDOWS_SYSTEM_PREFIX .. "/include/neaacdec.h") == nil  then
    print ("\27[93m" .. "neaacdec.h not found in \"" .. WINDOWS_SYSTEM_PREFIX .. "/include/\", run premake5 with \"--plugin-aac=enabled\" to force enable aac plugin" ..  "\27[39m")
    _OPTIONS["plugin-aac"] = "disabled"
    _OPTIONS["plugin-alac"] = "disabled"
  else
    _OPTIONS["plugin-aac"] = "enabled"
    _OPTIONS["plugin-alac"] = "enabled"
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
    "shared/mp4tagutil.c"
  }
  includedirs { "external/mp4p/include" }
  links { "faad", "mp4p" }
end

if option ("plugin-adplug") then
project "adplug_plugin"
  targetname "adplug"
  files {
    "plugins/adplug/plugin.c",
    "plugins/adplug/adplug-db.cpp",
    "plugins/adplug/libbinio/*.c",
    "plugins/adplug/libbinio/*.cpp",
    "plugins/adplug/adplug/*.c",
    "plugins/adplug/adplug/*.cpp"
  }
  defines {"stricmp=strcasecmp"}
  includedirs {"plugins/adplug/adplug", "plugins/adplug/libbinio"}
  links {"stdc++"}

  filter "files:**.cpp"
    buildoptions {
        "-std=c++11"
    }
end

if option ("plugin-alac") then
project "alac_plugin"
  targetname "alac"
  files {
    "plugins/alac/alac_plugin.c",
    "plugins/alac/alac.c",
    "shared/mp4tagutil.c"
  }
  includedirs { "external/mp4p/include" }
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
    "plugins/dca/libdca/extract_dca.c",
    "plugins/dca/libdca/gettimeofday.c",
    "plugins/dca/libdca/parse.c",
    "plugins/dca/libdca/bitstream.c",
    "plugins/dca/libdca/downmix.c"
  }
  includedirs {
    "plugins/dca/libdca"
  }
end

if option ("plugin-dumb") then
project "dumb_plugin"
  targetname "ddb_dumb"
  files {
    "plugins/dumb/dumb-kode54/src/it/*.c",
    "plugins/dumb/dumb-kode54/src/helpers/*.c",
    "plugins/dumb/dumb-kode54/src/core/*.c",
    "plugins/dumb/dumb-kode54/src/helpers/resample.inc",
    "plugins/dumb/dumb-kode54/src/helpers/resamp2.inc",
    "plugins/dumb/dumb-kode54/src/helpers/resamp3.inc",
    "plugins/dumb/modloader.cpp",
    "plugins/dumb/unrealfmt.cpp",
    "plugins/dumb/unrealfmtdata.cpp",
    "plugins/dumb/cdumb.c",
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
    "plugins/gme/game-music-emu-0.6pre/gme/higan/dsp/dsp.hpp",
    "plugins/gme/game-music-emu-0.6pre/gme/higan/processor/spc700/memory.hpp",
    "plugins/gme/game-music-emu-0.6pre/gme/higan/processor/spc700/registers.hpp",
    "plugins/gme/game-music-emu-0.6pre/gme/higan/processor/spc700/spc700.hpp",
    "plugins/gme/game-music-emu-0.6pre/gme/higan/smp/smp.hpp"
  }
  includedirs {
    "plugins/gme/game-music-emu-0.6pre/gme/",
    "plugins/gme/game-music-emu-0.6pre"
  }
  defines {"GME_VERSION_055","HAVE_STDINT_H"}
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
  "plugins/mms/libmms/uri.c"
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
  pkgconfig ("dbus-1", "glib-2.0", "gio-2.0", "gdk-pixbuf-2.0")
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
  pkgconfig ("libpulse-simple")
end

if option ("plugin-sc68") then
project "sc68_plugin"
  targetname "in_sc68"
  files {
    "plugins/sc68/in_sc68.c",
    "plugins/sc68/libsc68/file68/src/*.c",
    "plugins/sc68/libsc68/libsc68/dial68/*.c",
    "plugins/sc68/libsc68/unice68/unice68_unpack.c",
    "plugins/sc68/libsc68/unice68/unice68_pack.c",
    "plugins/sc68/libsc68/unice68/unice68_version.c",
    "plugins/sc68/libsc68/libsc68/conf68.c",
    "plugins/sc68/libsc68/libsc68/api68.c",
    "plugins/sc68/libsc68/libsc68/mixer68.c",
    "plugins/sc68/libsc68/libsc68/io68/*.c",
    "plugins/sc68/libsc68/libsc68/libsc68.c",
    "plugins/sc68/libsc68/libsc68/emu68/lines68.c",
    "plugins/sc68/libsc68/libsc68/emu68/ioplug68.c",
    "plugins/sc68/libsc68/libsc68/emu68/mem68.c",
    "plugins/sc68/libsc68/libsc68/emu68/getea68.c",
    "plugins/sc68/libsc68/libsc68/emu68/inst68.c",
    "plugins/sc68/libsc68/libsc68/emu68/emu68.c",
    "plugins/sc68/libsc68/libsc68/emu68/error68.c",
    "plugins/sc68/libsc68/desa68/desa68.c"
  }
  excludes {
    "plugins/sc68/libsc68/libsc68/io68/ym_atarist_table.c",
    "plugins/sc68/libsc68/libsc68/io68/ym_linear_table.c"
  }
  includedirs {
    "plugins/sc68/libsc68/file68",
    "plugins/sc68/libsc68/file68/sc68",
    "plugins/sc68/libsc68/unice68",
    "plugins/sc68/libsc68/libsc68",
    "plugins/sc68/libsc68/libsc68/sc68",
    "plugins/sc68/libsc68/libsc68/emu68"
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
  targetname "ddb_shn"
  files {
    "plugins/shn/shnplugin.c",
    "plugins/shn/libshn/array.c",
    "plugins/shn/libshn/convert.c",
    "plugins/shn/libshn/misc.c",
    "plugins/shn/libshn/output.c",
    "plugins/shn/libshn/seek.c",
    "plugins/shn/libshn/shn.h",
    "plugins/shn/libshn/shorten.c",
    "plugins/shn/libshn/sulawalaw.c",
    "plugins/shn/libshn/vario.c",
    "plugins/shn/libshn/wave.c"
  }
  buildoptions {"-include stdint.h"}
  includedirs {"plugins/shn/libshn"}
  links {"m"}
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
    "plugins/vtx/libayemu/ay8912.c",
    "plugins/vtx/libayemu/lh5dec.c",
    "plugins/vtx/libayemu/vtxfile.c"
  }
  includedirs {"plugins/vtx/libayemu"}
  undefines {"VERSION"}
end

if option ("plugin-wma") then
project "wma"
  files {
    "plugins/wma/wma_plugin.c",
    "plugins/wma/asfheader.c",
    "plugins/wma/libasf/asf.c",
    "plugins/wma/libwma/wmafixed.c",
    "plugins/wma/libwma/wmadeci.c",
    "plugins/wma/libwma/fft-ffmpeg.c",
    "plugins/wma/libwma/mdct_lookup.c",
    "plugins/wma/libwma/mdct.c",
    "plugins/wma/libwma/ffmpeg_bitstream.c",
  }
  includedirs {"plugins/wma"}
end

if option ("plugin-flac", "flac ogg") then
project "flac_plugin"
  targetname "flac"
  files {
    "plugins/flac/*.c"
  }
  defines {"HAVE_OGG_STREAM_FLUSH_FILL"}
  links {"FLAC", "ogg", "liboggedit"}
end

if option ("plugin-wavpack", "wavpack") then
project "wavpack_plugin"
  targetname "wavpack"
  files {
    "plugins/wavpack/*.c",
  }
  links {"wavpack"}
end

if option ("plugin-ffmpeg", "libavformat") then
project "ffmpeg"
  targetname "ffmpeg"
  files {
    "plugins/ffmpeg/*.c",
  }
  pkgconfig ("libavformat")
  pkgconfig ("libavcodec")
  pkgconfig ("libavutil")
end

if option ("plugin-vorbis", "vorbisfile vorbis ogg") then
project "vorbis_plugin"
  targetname "vorbis"
  files {
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
    "plugins/ffap/*.c",
    "plugins/ffap/dsputil_yasm.asm",
  }
  defines {"APE_USE_ASM=yes", "ARCH_X86_64=1"}
end

if option ("plugin-hotkeys") then
project "hotkeys"
  files {
    "plugins/hotkeys/*.c",
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
    "plugins/portaudio/*.c"
  }
  buildoptions {"-fblocks"}
  links {"BlocksRuntime", "dispatch"}
  pkgconfig ("portaudio-2.0")
end

if option ("plugin-waveout") then
project "waveout"
  files {
    "plugins/waveout/*.c"
  }
  links {"winmm", "ksuser"}
  removeplatforms {"Linux"}
end

if option ("plugin-gtk2", "gtk+-2.0 jansson") then
project "ddb_gui_GTK2"
  files {
    "plugins/gtkui/*.c",
    "plugins/gtkui/medialib/*.c",
    "plugins/gtkui/prefwin/*.c",
    "plugins/gtkui/covermanager/*.c",
    "plugins/gtkui/playlist/*.c",
    "plugins/gtkui/scriptable/*.c",
    "shared/eqpreset.c",
    "shared/pluginsettings.c",
    "shared/trkproperties_shared.c",
    "shared/analyzer/analyzer.c",
    "shared/scope/scope.c",
    "shared/scriptable/*.c",
    "plugins/libparser/parser.c",
    "src/utf8.c"
  }
  excludes {
    "plugins/gtkui/deadbeefapp.c",
    "plugins/gtkui/gtkui-gresources.c"
  }
  includedirs {
    "plugins/gtkui",
    "plugins/libparser",
    "shared"
  }
  buildoptions {"-fblocks"}
  pkgconfig ("gtk+-2.0 jansson")
  links {"libdeletefromdisk", "libtftintutil", "dispatch", "BlocksRuntime"}
  defines ("GLIB_DISABLE_DEPRECATION_WARNINGS")
end

if option ("plugin-gtk3", "gtk+-3.0 jansson") then
project "ddb_gui_GTK3"
  files {
    "plugins/gtkui/*.c",
    "plugins/gtkui/medialib/*.c",
    "plugins/gtkui/prefwin/*.c",
    "plugins/gtkui/covermanager/*.c",
    "plugins/gtkui/playlist/*.c",
    "plugins/gtkui/scriptable/*.c",
    "shared/eqpreset.c",
    "shared/pluginsettings.c",
    "shared/trkproperties_shared.c",
    "shared/analyzer/analyzer.c",
    "shared/scope/scope.c",
    "shared/scriptable/*.c",
    "plugins/libparser/parser.c",
    "src/utf8.c"
  }
  includedirs {
    "plugins/gtkui",
    "plugins/libparser",
    "shared"
  }

  prebuildcommands {
    "glib-compile-resources --sourcedir=plugins/gtkui --target=plugins/gtkui/gtkui-gresources.c --generate-source plugins/gtkui/gtkui.gresources.xml"
  }

  buildoptions {"-fblocks"}
  pkgconfig("gtk+-3.0 jansson")
  links {"libdeletefromdisk", "libtftintutil", "dispatch", "BlocksRuntime"}
  defines ("GLIB_DISABLE_DEPRECATION_WARNINGS")
end

if option ("plugin-rg_scanner") then
project "rg_scanner"
  files {
    "plugins/rg_scanner/*.c",
    "plugins/rg_scanner/ebur128/*.c"
  }
  buildoptions {"-fblocks"}
  links {"dispatch", "BlocksRuntime"}
end

if option ("plugin-converter") then
project "converter"
  files {
    "plugins/converter/converter.c",
    "shared/mp4tagutil.c"
  }
  includedirs {"external/mp4p/include", "shared"}
  links {"mp4p"}
end

if option ("plugin-shellexec", "jansson") then
project "shellexec"
  files {
    "plugins/shellexec/shellexec.c",
    "plugins/shellexec/shellexecutil.c"
  }
  pkgconfig("jansson")
end

if option ("plugin-sndfile", "sndfile") then
project "sndfile_plugin"
  targetname "sndfile"
  files {
    "plugins/sndfile/*.c"
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
    "plugins/psf/aosdk",
    "plugins/psf/aosdk/eng_ssf",
    "plugins/psf/aosdk/eng_qsf",
    "plugins/psf/aosdk/eng_dsf"
  }
  defines {
    "HAS_PSXCPU=1"
  }
  files {
    "plugins/psf/plugin.c",
    "plugins/psf/aosdk/psfmain.c",
    "plugins/psf/aosdk/corlett.c",
    "plugins/psf/aosdk/eng_dsf/eng_dsf.c",
    "plugins/psf/aosdk/eng_dsf/dc_hw.c",
    "plugins/psf/aosdk/eng_dsf/aica.c",
    "plugins/psf/aosdk/eng_dsf/aicadsp.c",
    "plugins/psf/aosdk/eng_dsf/arm7.c",
    "plugins/psf/aosdk/eng_dsf/arm7i.c",
    "plugins/psf/aosdk/eng_ssf/m68kcpu.c",
    "plugins/psf/aosdk/eng_ssf/m68kopac.c",
    "plugins/psf/aosdk/eng_ssf/m68kopdm.c",
    "plugins/psf/aosdk/eng_ssf/m68kopnz.c",
    "plugins/psf/aosdk/eng_ssf/m68kops.c",
    "plugins/psf/aosdk/eng_ssf/scsp.c",
    "plugins/psf/aosdk/eng_ssf/scspdsp.c",
    "plugins/psf/aosdk/eng_ssf/sat_hw.c",
    "plugins/psf/aosdk/eng_ssf/eng_ssf.c",
    "plugins/psf/aosdk/eng_qsf/eng_qsf.c",
    "plugins/psf/aosdk/eng_qsf/kabuki.c",
    "plugins/psf/aosdk/eng_qsf/qsound.c",
    "plugins/psf/aosdk/eng_qsf/z80.c",
    "plugins/psf/aosdk/eng_qsf/z80dasm.c",
    "plugins/psf/aosdk/eng_psf/eng_psf.c",
    "plugins/psf/aosdk/eng_psf/psx.c",
    "plugins/psf/aosdk/eng_psf/psx_hw.c",
    "plugins/psf/aosdk/eng_psf/peops/spu.c",
    "plugins/psf/aosdk/eng_psf/eng_psf2.c",
    "plugins/psf/aosdk/eng_psf/peops2/spu2.c",
    "plugins/psf/aosdk/eng_psf/peops2/dma2.c",
    "plugins/psf/aosdk/eng_psf/peops2/registers2.c",
    "plugins/psf/aosdk/eng_psf/eng_spu.c",
  }
  links {"z", "m"}
end

if option ("plugin-m3u") then
project "m3u"
  files {
    "plugins/m3u/*.c",
  }
end

if option ("plugin-vfs_curl", "libcurl") then
project "vfs_curl"
  files {
    "plugins/vfs_curl/*.c",
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
    "plugins/shellexecui/callbacks.c"
  }
  pkgconfig ("gtk+-2.0 jansson")
end

if option ("plugin-shellexecui_gtk3", "gtk+-3.0 jansson") then
project "shellexecui_gtk3"
  files {
    "plugins/shellexecui/shellexecui.c",
    "plugins/shellexecui/interface.c",
    "plugins/shellexecui/support.c",
    "plugins/shellexecui/callbacks.c"
  }
  pkgconfig ("gtk+-3.0 jansson")
end

if option ("plugin-wildmidi") then
project "wildmidi_plugin"
  targetname "wildmidi"
  files {
    "plugins/wildmidi/*.c",
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
    "plugins/musepack/crc32.c"
  }
  includedirs {"plugins/musepack"}
  links {"m"}
end

if option ("plugin-artwork", "flac ogg vorbisfile") then
project "artwork_plugin"
  targetname "artwork"
  files {
    "plugins/artwork/*.c",
    "shared/mp4tagutil.c"
  }
  includedirs {"external/mp4p/include", "shared"}
  buildoptions {"-fblocks"}
  defines {"USE_OGG=1", "USE_VFS_CURL", "USE_METAFLAC", "USE_MP4FF", "USE_TAGGING=1"}
  pkgconfig ("flac ogg vorbisfile")
  links {"FLAC", "ogg", "vorbisfile", "mp4p", "dispatch", "BlocksRuntime"}
else
  options_dic["plugin-artwork"] = "no"
end

if option ("plugin-supereq") then
project "supereq_plugin"
  targetname "supereq"
  files {
    "plugins/supereq/*.c",
    "plugins/supereq/libsupereq/*.cpp",
    "plugins/supereq/libsupereq/*.c"
  }
  includedirs {"plugins/supereq/libsupereq"}
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
    "plugins/nullout/*.c"
  }
end

if option ("plugin-lastfm", "libcurl") then
project "lastfm"
  files {
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
    "plugins/soundtouch/soundtouch/source/SoundTouch/*.cpp"
  }
end

if option ("plugin-tta") then
project "tta"
  files {
    "plugins/tta/ttaplug.c",
    "plugins/tta/libtta/ttadec.c"
  }
  includedirs { "plugins/tta/libtta" }
end

if option ("plugin-libretro") then
project "ddb_dsp_libretro"
  files {
    "external/ddb_dsp_libretro/libretro.cpp"
  }
  buildoptions {
    "-msse3"
  }
end

if option ("plugin-medialib") then
project "medialib"
  files {
    "plugins/medialib/medialib.c",
    "plugins/medialib/medialibcommon.c",
    "plugins/medialib/medialibdb.c",
    "plugins/medialib/medialibfilesystem_stub.c",
    "plugins/medialib/medialibscanner.c",
    "plugins/medialib/medialibsource.c",
    "plugins/medialib/medialibstate.c",
    "plugins/medialib/medialibtree.c",
    "plugins/medialib/scriptable_tfquery.c",
    "shared/scriptable/*.c"
  }
  includedirs {
    "shared"
  }
  pkgconfig ("jansson")
  buildoptions {"-fblocks"}
  links {"dispatch", "BlocksRuntime"}
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
  files {"src/main.c"}
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
  files {"src/main.c"}
  filter 'files:**.c'
    buildcommands {"./scripts/windows_postbuild.sh bin/%{cfg.buildcfg}"}
    buildoutputs {'%{cfg.objdir}/%{file.basename}.c_fake'}

-- Done with projects, print current build settings:
print_options()
