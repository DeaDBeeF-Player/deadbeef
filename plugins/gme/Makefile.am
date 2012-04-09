if HAVE_GME

#gmeversion=Game_Music_Emu-0.5.2
#gmeversion=game-music-emu-0.5.5
gmeversion=game-music-emu-svn
gmepath=@top_srcdir@/plugins/gme/$(gmeversion)

EXTRA_DIST = $(gmepath)/changes.txt $(gmepath)/design.txt $(gmepath)/gme.txt $(gmepath)/license.txt $(gmepath)/readme.txt 

gmedir = $(libdir)/$(PACKAGE)

pkglib_LTLIBRARIES = gme.la

gme_la_SOURCES = cgme.c\
	$(gmeversion)/gme/Ay_Apu.cpp\
	$(gmeversion)/gme/Gb_Apu.cpp\
	$(gmeversion)/gme/Hes_Emu.cpp\
	$(gmeversion)/gme/Nes_Fme7_Apu.cpp\
	$(gmeversion)/gme/Sms_Apu.cpp\
	$(gmeversion)/gme/Ay_Cpu.cpp\
	$(gmeversion)/gme/Gb_Cpu.cpp\
	$(gmeversion)/gme/Kss_Cpu.cpp\
	$(gmeversion)/gme/Nes_Namco_Apu.cpp\
	$(gmeversion)/gme/Snes_Spc.cpp\
	$(gmeversion)/gme/Ay_Emu.cpp\
	$(gmeversion)/gme/Gb_Oscs.cpp\
	$(gmeversion)/gme/Kss_Emu.cpp\
	$(gmeversion)/gme/Nes_Oscs.cpp\
	$(gmeversion)/gme/Spc_Cpu.cpp\
	$(gmeversion)/gme/Blip_Buffer.cpp\
	$(gmeversion)/gme/Gbs_Emu.cpp\
	$(gmeversion)/gme/Kss_Scc_Apu.cpp\
	$(gmeversion)/gme/Nes_Vrc6_Apu.cpp\
	$(gmeversion)/gme/Spc_Dsp.cpp\
	$(gmeversion)/gme/Classic_Emu.cpp\
	$(gmeversion)/gme/gme.cpp\
	$(gmeversion)/gme/M3u_Playlist.cpp\
	$(gmeversion)/gme/Nsfe_Emu.cpp\
	$(gmeversion)/gme/Spc_Emu.cpp\
	$(gmeversion)/gme/Data_Reader.cpp\
	$(gmeversion)/gme/Gme_File.cpp\
	$(gmeversion)/gme/Multi_Buffer.cpp\
	$(gmeversion)/gme/Nsf_Emu.cpp\
	$(gmeversion)/gme/Vgm_Emu.cpp\
	$(gmeversion)/gme/Dual_Resampler.cpp\
	$(gmeversion)/gme/Gym_Emu.cpp\
	$(gmeversion)/gme/Music_Emu.cpp\
	$(gmeversion)/gme/Sap_Apu.cpp\
	$(gmeversion)/gme/Vgm_Emu_Impl.cpp\
	$(gmeversion)/gme/Effects_Buffer.cpp\
	$(gmeversion)/gme/Hes_Apu.cpp\
	$(gmeversion)/gme/Nes_Apu.cpp\
	$(gmeversion)/gme/Sap_Cpu.cpp\
	$(gmeversion)/gme/Ym2413_Emu.cpp\
	$(gmeversion)/gme/Fir_Resampler.cpp\
	$(gmeversion)/gme/Hes_Cpu.cpp\
	$(gmeversion)/gme/Nes_Cpu.cpp\
	$(gmeversion)/gme/Sap_Emu.cpp\
	$(gmeversion)/gme/Ym2612_Emu.cpp\
	$(gmeversion)/gme/Ay_Apu.h\
	$(gmeversion)/gme/Ay_Cpu.h\
	$(gmeversion)/gme/Ay_Emu.h\
	$(gmeversion)/gme/blargg_common.h\
	$(gmeversion)/gme/blargg_config.h\
	$(gmeversion)/gme/blargg_endian.h\
	$(gmeversion)/gme/blargg_source.h\
	$(gmeversion)/gme/Blip_Buffer.h\
	$(gmeversion)/gme/Classic_Emu.h\
	$(gmeversion)/gme/Data_Reader.h\
	$(gmeversion)/gme/Dual_Resampler.h\
	$(gmeversion)/gme/Effects_Buffer.h\
	$(gmeversion)/gme/Fir_Resampler.h\
	$(gmeversion)/gme/Gb_Apu.h\
	$(gmeversion)/gme/Gb_Cpu.h\
	$(gmeversion)/gme/gb_cpu_io.h\
	$(gmeversion)/gme/Gb_Oscs.h\
	$(gmeversion)/gme/Gbs_Emu.h\
	$(gmeversion)/gme/Gme_File.h\
	$(gmeversion)/gme/gme.h\
	$(gmeversion)/gme/Gym_Emu.h\
	$(gmeversion)/gme/Hes_Apu.h\
	$(gmeversion)/gme/Hes_Cpu.h\
	$(gmeversion)/gme/hes_cpu_io.h\
	$(gmeversion)/gme/Hes_Emu.h\
	$(gmeversion)/gme/Kss_Cpu.h\
	$(gmeversion)/gme/Kss_Emu.h\
	$(gmeversion)/gme/Kss_Scc_Apu.h\
	$(gmeversion)/gme/M3u_Playlist.h\
	$(gmeversion)/gme/Multi_Buffer.h\
	$(gmeversion)/gme/Music_Emu.h\
	$(gmeversion)/gme/Nes_Apu.h\
	$(gmeversion)/gme/Nes_Cpu.h\
	$(gmeversion)/gme/nes_cpu_io.h\
	$(gmeversion)/gme/Nes_Fme7_Apu.h\
	$(gmeversion)/gme/Nes_Namco_Apu.h\
	$(gmeversion)/gme/Nes_Oscs.h\
	$(gmeversion)/gme/Nes_Vrc6_Apu.h\
	$(gmeversion)/gme/Nsfe_Emu.h\
	$(gmeversion)/gme/Nsf_Emu.h\
	$(gmeversion)/gme/Sap_Apu.h\
	$(gmeversion)/gme/Sap_Cpu.h\
	$(gmeversion)/gme/sap_cpu_io.h\
	$(gmeversion)/gme/Sap_Emu.h\
	$(gmeversion)/gme/Sms_Apu.h\
	$(gmeversion)/gme/Sms_Oscs.h\
	$(gmeversion)/gme/Snes_Spc.h\
	$(gmeversion)/gme/Spc_Cpu.h\
	$(gmeversion)/gme/Spc_Dsp.h\
	$(gmeversion)/gme/Spc_Emu.h\
	$(gmeversion)/gme/Vgm_Emu.h\
	$(gmeversion)/gme/Vgm_Emu_Impl.h\
	$(gmeversion)/gme/Ym2413_Emu.h\
	$(gmeversion)/gme/Ym2612_Emu.h\
	$(gmeversion)/gme/gme_types.h\
	$(gmeversion)/gme/Spc_Filter.cpp\
	$(gmeversion)/gme/Spc_Filter.h

gme_la_LDFLAGS = -module -nostdlib -lsupc++
gme_la_LIBADD = $(ZLIB_LIBS)

AM_CFLAGS = $(CFLAGS) $(ZLIB_CFLAGS) -I$(gmepath) -std=c99 -DGME_VERSION_055
AM_CPPFLAGS = $(CXXFLAGS) -fno-exceptions -fno-rtti -nostdlib -fno-unwind-tables
endif
