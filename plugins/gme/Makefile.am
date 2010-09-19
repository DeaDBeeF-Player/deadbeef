if HAVE_GME

#gmedir=Game_Music_Emu-0.5.2
gmeversion=game-music-emu-0.5.5
gmepath=@top_srcdir@/plugins/gme/$(gmeversion)

EXTRA_DIST = $(gmepath)/changes.txt $(gmepath)/design.txt $(gmepath)/gme.txt $(gmepath)/license.txt $(gmepath)/readme.txt 

gmedir = $(libdir)/$(PACKAGE)

pkglib_LTLIBRARIES = gme.la

gme_la_SOURCES = cgme.c\
game-music-emu-0.5.5/gme/Ay_Apu.cpp\
game-music-emu-0.5.5/gme/Gb_Apu.cpp\
game-music-emu-0.5.5/gme/Hes_Emu.cpp\
game-music-emu-0.5.5/gme/Nes_Fme7_Apu.cpp\
game-music-emu-0.5.5/gme/Sms_Apu.cpp\
game-music-emu-0.5.5/gme/Ay_Cpu.cpp\
game-music-emu-0.5.5/gme/Gb_Cpu.cpp\
game-music-emu-0.5.5/gme/Kss_Cpu.cpp\
game-music-emu-0.5.5/gme/Nes_Namco_Apu.cpp\
game-music-emu-0.5.5/gme/Snes_Spc.cpp\
game-music-emu-0.5.5/gme/Ay_Emu.cpp\
game-music-emu-0.5.5/gme/Gb_Oscs.cpp\
game-music-emu-0.5.5/gme/Kss_Emu.cpp\
game-music-emu-0.5.5/gme/Nes_Oscs.cpp\
game-music-emu-0.5.5/gme/Spc_Cpu.cpp\
game-music-emu-0.5.5/gme/Blip_Buffer.cpp\
game-music-emu-0.5.5/gme/Gbs_Emu.cpp\
game-music-emu-0.5.5/gme/Kss_Scc_Apu.cpp\
game-music-emu-0.5.5/gme/Nes_Vrc6_Apu.cpp\
game-music-emu-0.5.5/gme/Spc_Dsp.cpp\
game-music-emu-0.5.5/gme/Classic_Emu.cpp\
game-music-emu-0.5.5/gme/gme.cpp\
game-music-emu-0.5.5/gme/M3u_Playlist.cpp\
game-music-emu-0.5.5/gme/Nsfe_Emu.cpp\
game-music-emu-0.5.5/gme/Spc_Emu.cpp\
game-music-emu-0.5.5/gme/Data_Reader.cpp\
game-music-emu-0.5.5/gme/Gme_File.cpp\
game-music-emu-0.5.5/gme/Multi_Buffer.cpp\
game-music-emu-0.5.5/gme/Nsf_Emu.cpp\
game-music-emu-0.5.5/gme/Vgm_Emu.cpp\
game-music-emu-0.5.5/gme/Dual_Resampler.cpp\
game-music-emu-0.5.5/gme/Gym_Emu.cpp\
game-music-emu-0.5.5/gme/Music_Emu.cpp\
game-music-emu-0.5.5/gme/Sap_Apu.cpp\
game-music-emu-0.5.5/gme/Vgm_Emu_Impl.cpp\
game-music-emu-0.5.5/gme/Effects_Buffer.cpp\
game-music-emu-0.5.5/gme/Hes_Apu.cpp\
game-music-emu-0.5.5/gme/Nes_Apu.cpp\
game-music-emu-0.5.5/gme/Sap_Cpu.cpp\
game-music-emu-0.5.5/gme/Ym2413_Emu.cpp\
game-music-emu-0.5.5/gme/Fir_Resampler.cpp\
game-music-emu-0.5.5/gme/Hes_Cpu.cpp\
game-music-emu-0.5.5/gme/Nes_Cpu.cpp\
game-music-emu-0.5.5/gme/Sap_Emu.cpp\
game-music-emu-0.5.5/gme/Ym2612_Emu.cpp\
game-music-emu-0.5.5/gme/Ay_Apu.h\
game-music-emu-0.5.5/gme/Ay_Cpu.h\
game-music-emu-0.5.5/gme/Ay_Emu.h\
game-music-emu-0.5.5/gme/blargg_common.h\
game-music-emu-0.5.5/gme/blargg_config.h\
game-music-emu-0.5.5/gme/blargg_endian.h\
game-music-emu-0.5.5/gme/blargg_source.h\
game-music-emu-0.5.5/gme/Blip_Buffer.h\
game-music-emu-0.5.5/gme/Classic_Emu.h\
game-music-emu-0.5.5/gme/Data_Reader.h\
game-music-emu-0.5.5/gme/Dual_Resampler.h\
game-music-emu-0.5.5/gme/Effects_Buffer.h\
game-music-emu-0.5.5/gme/Fir_Resampler.h\
game-music-emu-0.5.5/gme/Gb_Apu.h\
game-music-emu-0.5.5/gme/Gb_Cpu.h\
game-music-emu-0.5.5/gme/gb_cpu_io.h\
game-music-emu-0.5.5/gme/Gb_Oscs.h\
game-music-emu-0.5.5/gme/Gbs_Emu.h\
game-music-emu-0.5.5/gme/Gme_File.h\
game-music-emu-0.5.5/gme/gme.h\
game-music-emu-0.5.5/gme/Gym_Emu.h\
game-music-emu-0.5.5/gme/Hes_Apu.h\
game-music-emu-0.5.5/gme/Hes_Cpu.h\
game-music-emu-0.5.5/gme/hes_cpu_io.h\
game-music-emu-0.5.5/gme/Hes_Emu.h\
game-music-emu-0.5.5/gme/Kss_Cpu.h\
game-music-emu-0.5.5/gme/Kss_Emu.h\
game-music-emu-0.5.5/gme/Kss_Scc_Apu.h\
game-music-emu-0.5.5/gme/M3u_Playlist.h\
game-music-emu-0.5.5/gme/Multi_Buffer.h\
game-music-emu-0.5.5/gme/Music_Emu.h\
game-music-emu-0.5.5/gme/Nes_Apu.h\
game-music-emu-0.5.5/gme/Nes_Cpu.h\
game-music-emu-0.5.5/gme/nes_cpu_io.h\
game-music-emu-0.5.5/gme/Nes_Fme7_Apu.h\
game-music-emu-0.5.5/gme/Nes_Namco_Apu.h\
game-music-emu-0.5.5/gme/Nes_Oscs.h\
game-music-emu-0.5.5/gme/Nes_Vrc6_Apu.h\
game-music-emu-0.5.5/gme/Nsfe_Emu.h\
game-music-emu-0.5.5/gme/Nsf_Emu.h\
game-music-emu-0.5.5/gme/Sap_Apu.h\
game-music-emu-0.5.5/gme/Sap_Cpu.h\
game-music-emu-0.5.5/gme/sap_cpu_io.h\
game-music-emu-0.5.5/gme/Sap_Emu.h\
game-music-emu-0.5.5/gme/Sms_Apu.h\
game-music-emu-0.5.5/gme/Sms_Oscs.h\
game-music-emu-0.5.5/gme/Snes_Spc.h\
game-music-emu-0.5.5/gme/Spc_Cpu.h\
game-music-emu-0.5.5/gme/Spc_Dsp.h\
game-music-emu-0.5.5/gme/Spc_Emu.h\
game-music-emu-0.5.5/gme/Vgm_Emu.h\
game-music-emu-0.5.5/gme/Vgm_Emu_Impl.h\
game-music-emu-0.5.5/gme/Ym2413_Emu.h\
game-music-emu-0.5.5/gme/Ym2612_Emu.h\
game-music-emu-0.5.5/gme/gme_types.h

gme_la_LDFLAGS = -module

gme_la_LIBADD = $(LDADD) -lstdc++
AM_CFLAGS = $(CFLAGS) -I$(gmepath) -std=c99 -DGME_VERSION_055
endif
