if HAVE_GME

#gmeversion=Game_Music_Emu-0.5.2
#gmeversion=game-music-emu-0.5.5
#gmeversion=game-music-emu-svn
#gmeversion=game-music-emu-0.6.0
gmeversion=game-music-emu-0.6pre
gmepath=@top_srcdir@/plugins/gme/$(gmeversion)

EXTRA_DIST = $(gmepath)/changes.txt $(gmepath)/design.txt $(gmepath)/gme.txt $(gmepath)/license.txt $(gmepath)/readme.txt 

gmedir = $(libdir)/$(PACKAGE)

pkglib_LTLIBRARIES = gme.la

# 0.6pre (foo_gep) files
gme_la_SOURCES = cgme.c\
	$(gmeversion)/gme/Ay_Apu.cpp\
	$(gmeversion)/gme/Ay_Core.cpp\
	$(gmeversion)/gme/Ay_Cpu.cpp\
	$(gmeversion)/gme/Ay_Emu.cpp\
	$(gmeversion)/gme/blargg_common.cpp\
	$(gmeversion)/gme/blargg_errors.cpp\
	$(gmeversion)/gme/Blip_Buffer.cpp\
	$(gmeversion)/gme/Classic_Emu.cpp\
	$(gmeversion)/gme/Downsampler.cpp\
	$(gmeversion)/gme/Dual_Resampler.cpp\
	$(gmeversion)/gme/Effects_Buffer.cpp\
	$(gmeversion)/gme/emu2413.cpp\
	$(gmeversion)/gme/Fir_Resampler.cpp\
	$(gmeversion)/gme/fmopl.cpp\
	$(gmeversion)/gme/Gb_Apu.cpp\
	$(gmeversion)/gme/Gb_Cpu.cpp\
	$(gmeversion)/gme/Gb_Oscs.cpp\
	$(gmeversion)/gme/Gbs_Core.cpp\
	$(gmeversion)/gme/Gbs_Cpu.cpp\
	$(gmeversion)/gme/Gbs_Emu.cpp\
	$(gmeversion)/gme/gme.cpp\
	$(gmeversion)/gme/Gme_File.cpp\
	$(gmeversion)/gme/Gme_Loader.cpp\
	$(gmeversion)/gme/Gym_Emu.cpp\
	$(gmeversion)/gme/Hes_Apu_Adpcm.cpp\
	$(gmeversion)/gme/Hes_Apu.cpp\
	$(gmeversion)/gme/Hes_Core.cpp\
	$(gmeversion)/gme/Hes_Cpu.cpp\
	$(gmeversion)/gme/Hes_Emu.cpp\
	$(gmeversion)/gme/Kss_Core.cpp\
	$(gmeversion)/gme/Kss_Cpu.cpp\
	$(gmeversion)/gme/Kss_Emu.cpp\
	$(gmeversion)/gme/Kss_Scc_Apu.cpp\
	$(gmeversion)/gme/M3u_Playlist.cpp\
	$(gmeversion)/gme/Multi_Buffer.cpp\
	$(gmeversion)/gme/Music_Emu.cpp\
	$(gmeversion)/gme/Nes_Apu.cpp\
	$(gmeversion)/gme/Nes_Cpu.cpp\
	$(gmeversion)/gme/Nes_Fds_Apu.cpp\
	$(gmeversion)/gme/Nes_Fme7_Apu.cpp\
	$(gmeversion)/gme/Nes_Namco_Apu.cpp\
	$(gmeversion)/gme/Nes_Oscs.cpp\
	$(gmeversion)/gme/Nes_Vrc6_Apu.cpp\
	$(gmeversion)/gme/Nes_Vrc7_Apu.cpp\
	$(gmeversion)/gme/Nsf_Core.cpp\
	$(gmeversion)/gme/Nsf_Cpu.cpp\
	$(gmeversion)/gme/Nsfe_Emu.cpp\
	$(gmeversion)/gme/Nsf_Emu.cpp\
	$(gmeversion)/gme/Nsf_Impl.cpp\
	$(gmeversion)/gme/Opl_Apu.cpp\
	$(gmeversion)/gme/Resampler.cpp\
	$(gmeversion)/gme/Rom_Data.cpp\
	$(gmeversion)/gme/Sap_Apu.cpp\
	$(gmeversion)/gme/Sap_Core.cpp\
	$(gmeversion)/gme/Sap_Cpu.cpp\
	$(gmeversion)/gme/Sap_Emu.cpp\
	$(gmeversion)/gme/Sgc_Core.cpp\
	$(gmeversion)/gme/Sgc_Cpu.cpp\
	$(gmeversion)/gme/Sgc_Emu.cpp\
	$(gmeversion)/gme/Sgc_Impl.cpp\
	$(gmeversion)/gme/Sms_Apu.cpp\
	$(gmeversion)/gme/Sms_Fm_Apu.cpp\
	$(gmeversion)/gme/Snes_Spc.cpp\
	$(gmeversion)/gme/Spc_Cpu.cpp\
	$(gmeversion)/gme/Spc_Dsp.cpp\
	$(gmeversion)/gme/Spc_Emu.cpp\
	$(gmeversion)/gme/SPC_Filter.cpp\
	$(gmeversion)/gme/Track_Filter.cpp\
	$(gmeversion)/gme/Upsampler.cpp\
	$(gmeversion)/gme/Vgm_Core.cpp\
	$(gmeversion)/gme/Vgm_Emu.cpp\
	$(gmeversion)/gme/Ym2413_Emu.cpp\
	$(gmeversion)/gme/Ym2612_Emu.cpp\
	$(gmeversion)/gme/ymdeltat.cpp\
	$(gmeversion)/gme/Z80_Cpu.cpp\
	$(gmeversion)/gme/Data_Reader.cpp\
	$(gmeversion)/gme/s_deltat.c\
	$(gmeversion)/gme/s_logtbl.c\
	$(gmeversion)/gme/s_opl.c\
	$(gmeversion)/gme/s_opltbl.c\
	$(gmeversion)/gme/ym2413.c\
	$(gmeversion)/gme/Ay_Apu.h\
	$(gmeversion)/gme/Ay_Core.h\
	$(gmeversion)/gme/Ay_Emu.h\
	$(gmeversion)/gme/blargg_common.h\
	$(gmeversion)/gme/blargg_config.h\
	$(gmeversion)/gme/blargg_endian.h\
	$(gmeversion)/gme/blargg_errors.h\
	$(gmeversion)/gme/blargg_source.h\
	$(gmeversion)/gme/Blip_Buffer.h\
	$(gmeversion)/gme/Blip_Buffer_impl2.h\
	$(gmeversion)/gme/Blip_Buffer_impl.h\
	$(gmeversion)/gme/Classic_Emu.h\
	$(gmeversion)/gme/divfix.h\
	$(gmeversion)/gme/Data_Reader.h\
	$(gmeversion)/gme/Downsampler.h\
	$(gmeversion)/gme/Dual_Resampler.h\
	$(gmeversion)/gme/Effects_Buffer.h\
	$(gmeversion)/gme/emu2413.h\
	$(gmeversion)/gme/Fir_Resampler.h\
	$(gmeversion)/gme/fmopl.h\
	$(gmeversion)/gme/Gb_Apu.h\
	$(gmeversion)/gme/Gb_Cpu.h\
	$(gmeversion)/gme/Gb_Cpu_run.h\
	$(gmeversion)/gme/Gb_Oscs.h\
	$(gmeversion)/gme/Gbs_Core.h\
	$(gmeversion)/gme/Gbs_Emu.h\
	$(gmeversion)/gme/Gme_File.h\
	$(gmeversion)/gme/gme.h\
	$(gmeversion)/gme/Gme_Loader.h\
	$(gmeversion)/gme/Gym_Emu.h\
	$(gmeversion)/gme/Hes_Apu_Adpcm.h\
	$(gmeversion)/gme/Hes_Apu.h\
	$(gmeversion)/gme/Hes_Core.h\
	$(gmeversion)/gme/Hes_Cpu.h\
	$(gmeversion)/gme/Hes_Cpu_run.h\
	$(gmeversion)/gme/Hes_Emu.h\
	$(gmeversion)/gme/i_fmpac.h\
	$(gmeversion)/gme/i_fmunit.h\
	$(gmeversion)/gme/i_vrc7.h\
	$(gmeversion)/gme/kmsnddev.h\
	$(gmeversion)/gme/Kss_Core.h\
	$(gmeversion)/gme/Kss_Emu.h\
	$(gmeversion)/gme/Kss_Scc_Apu.h\
	$(gmeversion)/gme/M3u_Playlist.h\
	$(gmeversion)/gme/Multi_Buffer.h\
	$(gmeversion)/gme/Music_Emu.h\
	$(gmeversion)/gme/Nes_Apu.h\
	$(gmeversion)/gme/Nes_Cpu.h\
	$(gmeversion)/gme/Nes_Cpu_run.h\
	$(gmeversion)/gme/Nes_Fds_Apu.h\
	$(gmeversion)/gme/Nes_Fme7_Apu.h\
	$(gmeversion)/gme/Nes_Mmc5_Apu.h\
	$(gmeversion)/gme/Nes_Namco_Apu.h\
	$(gmeversion)/gme/Nes_Oscs.h\
	$(gmeversion)/gme/nestypes.h\
	$(gmeversion)/gme/Nes_Vrc6_Apu.h\
	$(gmeversion)/gme/Nes_Vrc7_Apu.h\
	$(gmeversion)/gme/Nsf_Core.h\
	$(gmeversion)/gme/Nsfe_Emu.h\
	$(gmeversion)/gme/Nsf_Emu.h\
	$(gmeversion)/gme/Nsf_Impl.h\
	$(gmeversion)/gme/Opl_Apu.h\
	$(gmeversion)/gme/Resampler.h\
	$(gmeversion)/gme/Rom_Data.h\
	$(gmeversion)/gme/Sap_Apu.h\
	$(gmeversion)/gme/Sap_Core.h\
	$(gmeversion)/gme/Sap_Emu.h\
	$(gmeversion)/gme/s_deltat.h\
	$(gmeversion)/gme/Sgc_Core.h\
	$(gmeversion)/gme/Sgc_Emu.h\
	$(gmeversion)/gme/Sgc_Impl.h\
	$(gmeversion)/gme/s_logtbl.h\
	$(gmeversion)/gme/Sms_Apu.h\
	$(gmeversion)/gme/Sms_Fm_Apu.h\
	$(gmeversion)/gme/Snes_Spc.h\
	$(gmeversion)/gme/s_opl.h\
	$(gmeversion)/gme/s_opltbl.h\
	$(gmeversion)/gme/Spc_Cpu.h\
	$(gmeversion)/gme/Spc_Dsp.h\
	$(gmeversion)/gme/Spc_Emu.h\
	$(gmeversion)/gme/Spc_Filter.h\
	$(gmeversion)/gme/Track_Filter.h\
	$(gmeversion)/gme/Upsampler.h\
	$(gmeversion)/gme/Vgm_Core.h\
	$(gmeversion)/gme/Vgm_Emu.h\
	$(gmeversion)/gme/Ym2413_Emu.h\
	$(gmeversion)/gme/ym2413.h\
	$(gmeversion)/gme/Ym2612_Emu.h\
	$(gmeversion)/gme/ymdeltat.h\
	$(gmeversion)/gme/Z80_Cpu.h\
	$(gmeversion)/gme/Z80_Cpu_run.h

# 0.6.0 files
#gme_la_SOURCES = cgme.c\
#	$(gmeversion)/gme/Ay_Apu.cpp\
#	$(gmeversion)/gme/Ay_Cpu.cpp\
#	$(gmeversion)/gme/Ay_Emu.cpp\
#	$(gmeversion)/gme/Blip_Buffer.cpp\
#	$(gmeversion)/gme/Classic_Emu.cpp\
#	$(gmeversion)/gme/Data_Reader.cpp\
#	$(gmeversion)/gme/Dual_Resampler.cpp\
#	$(gmeversion)/gme/Effects_Buffer.cpp\
#	$(gmeversion)/gme/Fir_Resampler.cpp\
#	$(gmeversion)/gme/Gb_Apu.cpp\
#	$(gmeversion)/gme/Gb_Cpu.cpp\
#	$(gmeversion)/gme/Gb_Oscs.cpp\
#	$(gmeversion)/gme/Gbs_Emu.cpp\
#	$(gmeversion)/gme/gme.cpp\
#	$(gmeversion)/gme/Gme_File.cpp\
#	$(gmeversion)/gme/Gym_Emu.cpp\
#	$(gmeversion)/gme/Hes_Apu.cpp\
#	$(gmeversion)/gme/Hes_Cpu.cpp\
#	$(gmeversion)/gme/Hes_Emu.cpp\
#	$(gmeversion)/gme/Kss_Cpu.cpp\
#	$(gmeversion)/gme/Kss_Emu.cpp\
#	$(gmeversion)/gme/Kss_Scc_Apu.cpp\
#	$(gmeversion)/gme/M3u_Playlist.cpp\
#	$(gmeversion)/gme/Multi_Buffer.cpp\
#	$(gmeversion)/gme/Music_Emu.cpp\
#	$(gmeversion)/gme/Nes_Apu.cpp\
#	$(gmeversion)/gme/Nes_Cpu.cpp\
#	$(gmeversion)/gme/Nes_Fme7_Apu.cpp\
#	$(gmeversion)/gme/Nes_Namco_Apu.cpp\
#	$(gmeversion)/gme/Nes_Oscs.cpp\
#	$(gmeversion)/gme/Nes_Vrc6_Apu.cpp\
#	$(gmeversion)/gme/Nsfe_Emu.cpp\
#	$(gmeversion)/gme/Nsf_Emu.cpp\
#	$(gmeversion)/gme/Sap_Apu.cpp\
#	$(gmeversion)/gme/Sap_Cpu.cpp\
#	$(gmeversion)/gme/Sap_Emu.cpp\
#	$(gmeversion)/gme/Sms_Apu.cpp\
#	$(gmeversion)/gme/Snes_Spc.cpp\
#	$(gmeversion)/gme/Spc_Cpu.cpp\
#	$(gmeversion)/gme/Spc_Dsp.cpp\
#	$(gmeversion)/gme/Spc_Emu.cpp\
#	$(gmeversion)/gme/Spc_Filter.cpp\
#	$(gmeversion)/gme/Vgm_Emu.cpp\
#	$(gmeversion)/gme/Vgm_Emu_Impl.cpp\
#	$(gmeversion)/gme/Ym2413_Emu.cpp\
#	$(gmeversion)/gme/Ym2612_Emu.cpp\
#	$(gmeversion)/gme/Ay_Apu.h\
#	$(gmeversion)/gme/Ay_Cpu.h\
#	$(gmeversion)/gme/Ay_Emu.h\
#	$(gmeversion)/gme/blargg_common.h\
#	$(gmeversion)/gme/blargg_config.h\
#	$(gmeversion)/gme/blargg_endian.h\
#	$(gmeversion)/gme/blargg_source.h\
#	$(gmeversion)/gme/Blip_Buffer.h\
#	$(gmeversion)/gme/Classic_Emu.h\
#	$(gmeversion)/gme/Data_Reader.h\
#	$(gmeversion)/gme/Dual_Resampler.h\
#	$(gmeversion)/gme/Effects_Buffer.h\
#	$(gmeversion)/gme/Fir_Resampler.h\
#	$(gmeversion)/gme/Gb_Apu.h\
#	$(gmeversion)/gme/Gb_Cpu.h\
#	$(gmeversion)/gme/gb_cpu_io.h\
#	$(gmeversion)/gme/Gb_Oscs.h\
#	$(gmeversion)/gme/Gbs_Emu.h\
#	$(gmeversion)/gme/Gme_File.h\
#	$(gmeversion)/gme/gme.h\
#	$(gmeversion)/gme/gme_types.h\
#	$(gmeversion)/gme/Gym_Emu.h\
#	$(gmeversion)/gme/Hes_Apu.h\
#	$(gmeversion)/gme/Hes_Cpu.h\
#	$(gmeversion)/gme/hes_cpu_io.h\
#	$(gmeversion)/gme/Hes_Emu.h\
#	$(gmeversion)/gme/Kss_Cpu.h\
#	$(gmeversion)/gme/Kss_Emu.h\
#	$(gmeversion)/gme/Kss_Scc_Apu.h\
#	$(gmeversion)/gme/M3u_Playlist.h\
#	$(gmeversion)/gme/Multi_Buffer.h\
#	$(gmeversion)/gme/Music_Emu.h\
#	$(gmeversion)/gme/Nes_Apu.h\
#	$(gmeversion)/gme/Nes_Cpu.h\
#	$(gmeversion)/gme/nes_cpu_io.h\
#	$(gmeversion)/gme/Nes_Fme7_Apu.h\
#	$(gmeversion)/gme/Nes_Namco_Apu.h\
#	$(gmeversion)/gme/Nes_Oscs.h\
#	$(gmeversion)/gme/Nes_Vrc6_Apu.h\
#	$(gmeversion)/gme/Nsfe_Emu.h\
#	$(gmeversion)/gme/Nsf_Emu.h\
#	$(gmeversion)/gme/Sap_Apu.h\
#	$(gmeversion)/gme/Sap_Cpu.h\
#	$(gmeversion)/gme/sap_cpu_io.h\
#	$(gmeversion)/gme/Sap_Emu.h\
#	$(gmeversion)/gme/Sms_Apu.h\
#	$(gmeversion)/gme/Sms_Oscs.h\
#	$(gmeversion)/gme/Snes_Spc.h\
#	$(gmeversion)/gme/Spc_Cpu.h\
#	$(gmeversion)/gme/Spc_Dsp.h\
#	$(gmeversion)/gme/Spc_Emu.h\
#	$(gmeversion)/gme/Spc_Filter.h\
#	$(gmeversion)/gme/Vgm_Emu.h\
#	$(gmeversion)/gme/Vgm_Emu_Impl.h\
#	$(gmeversion)/gme/Ym2413_Emu.h\
#	$(gmeversion)/gme/Ym2612_Emu.h

# 0.5.2 and 0.5.5 files
#gme_la_SOURCES = cgme.c\
#	$(gmeversion)/gme/Ay_Apu.cpp\
#	$(gmeversion)/gme/Gb_Apu.cpp\
#	$(gmeversion)/gme/Hes_Emu.cpp\
#	$(gmeversion)/gme/Nes_Fme7_Apu.cpp\
#	$(gmeversion)/gme/Sms_Apu.cpp\
#	$(gmeversion)/gme/Ay_Cpu.cpp\
#	$(gmeversion)/gme/Gb_Cpu.cpp\
#	$(gmeversion)/gme/Kss_Cpu.cpp\
#	$(gmeversion)/gme/Nes_Namco_Apu.cpp\
#	$(gmeversion)/gme/Snes_Spc.cpp\
#	$(gmeversion)/gme/Ay_Emu.cpp\
#	$(gmeversion)/gme/Gb_Oscs.cpp\
#	$(gmeversion)/gme/Kss_Emu.cpp\
#	$(gmeversion)/gme/Nes_Oscs.cpp\
#	$(gmeversion)/gme/Spc_Cpu.cpp\
#	$(gmeversion)/gme/Blip_Buffer.cpp\
#	$(gmeversion)/gme/Gbs_Emu.cpp\
#	$(gmeversion)/gme/Kss_Scc_Apu.cpp\
#	$(gmeversion)/gme/Nes_Vrc6_Apu.cpp\
#	$(gmeversion)/gme/Spc_Dsp.cpp\
#	$(gmeversion)/gme/Classic_Emu.cpp\
#	$(gmeversion)/gme/gme.cpp\
#	$(gmeversion)/gme/M3u_Playlist.cpp\
#	$(gmeversion)/gme/Nsfe_Emu.cpp\
#	$(gmeversion)/gme/Spc_Emu.cpp\
#	$(gmeversion)/gme/Data_Reader.cpp\
#	$(gmeversion)/gme/Gme_File.cpp\
#	$(gmeversion)/gme/Multi_Buffer.cpp\
#	$(gmeversion)/gme/Nsf_Emu.cpp\
#	$(gmeversion)/gme/Vgm_Emu.cpp\
#	$(gmeversion)/gme/Dual_Resampler.cpp\
#	$(gmeversion)/gme/Gym_Emu.cpp\
#	$(gmeversion)/gme/Music_Emu.cpp\
#	$(gmeversion)/gme/Sap_Apu.cpp\
#	$(gmeversion)/gme/Vgm_Emu_Impl.cpp\
#	$(gmeversion)/gme/Effects_Buffer.cpp\
#	$(gmeversion)/gme/Hes_Apu.cpp\
#	$(gmeversion)/gme/Nes_Apu.cpp\
#	$(gmeversion)/gme/Sap_Cpu.cpp\
#	$(gmeversion)/gme/Ym2413_Emu.cpp\
#	$(gmeversion)/gme/Fir_Resampler.cpp\
#	$(gmeversion)/gme/Hes_Cpu.cpp\
#	$(gmeversion)/gme/Nes_Cpu.cpp\
#	$(gmeversion)/gme/Sap_Emu.cpp\
#	$(gmeversion)/gme/Ym2612_Emu.cpp\
#	$(gmeversion)/gme/Ay_Apu.h\
#	$(gmeversion)/gme/Ay_Cpu.h\
#	$(gmeversion)/gme/Ay_Emu.h\
#	$(gmeversion)/gme/blargg_common.h\
#	$(gmeversion)/gme/blargg_config.h\
#	$(gmeversion)/gme/blargg_endian.h\
#	$(gmeversion)/gme/blargg_source.h\
#	$(gmeversion)/gme/Blip_Buffer.h\
#	$(gmeversion)/gme/Classic_Emu.h\
#	$(gmeversion)/gme/Data_Reader.h\
#	$(gmeversion)/gme/Dual_Resampler.h\
#	$(gmeversion)/gme/Effects_Buffer.h\
#	$(gmeversion)/gme/Fir_Resampler.h\
#	$(gmeversion)/gme/Gb_Apu.h\
#	$(gmeversion)/gme/Gb_Cpu.h\
#	$(gmeversion)/gme/gb_cpu_io.h\
#	$(gmeversion)/gme/Gb_Oscs.h\
#	$(gmeversion)/gme/Gbs_Emu.h\
#	$(gmeversion)/gme/Gme_File.h\
#	$(gmeversion)/gme/gme.h\
#	$(gmeversion)/gme/Gym_Emu.h\
#	$(gmeversion)/gme/Hes_Apu.h\
#	$(gmeversion)/gme/Hes_Cpu.h\
#	$(gmeversion)/gme/hes_cpu_io.h\
#	$(gmeversion)/gme/Hes_Emu.h\
#	$(gmeversion)/gme/Kss_Cpu.h\
#	$(gmeversion)/gme/Kss_Emu.h\
#	$(gmeversion)/gme/Kss_Scc_Apu.h\
#	$(gmeversion)/gme/M3u_Playlist.h\
#	$(gmeversion)/gme/Multi_Buffer.h\
#	$(gmeversion)/gme/Music_Emu.h\
#	$(gmeversion)/gme/Nes_Apu.h\
#	$(gmeversion)/gme/Nes_Cpu.h\
#	$(gmeversion)/gme/nes_cpu_io.h\
#	$(gmeversion)/gme/Nes_Fme7_Apu.h\
#	$(gmeversion)/gme/Nes_Namco_Apu.h\
#	$(gmeversion)/gme/Nes_Oscs.h\
#	$(gmeversion)/gme/Nes_Vrc6_Apu.h\
#	$(gmeversion)/gme/Nsfe_Emu.h\
#	$(gmeversion)/gme/Nsf_Emu.h\
#	$(gmeversion)/gme/Sap_Apu.h\
#	$(gmeversion)/gme/Sap_Cpu.h\
#	$(gmeversion)/gme/sap_cpu_io.h\
#	$(gmeversion)/gme/Sap_Emu.h\
#	$(gmeversion)/gme/Sms_Apu.h\
#	$(gmeversion)/gme/Sms_Oscs.h\
#	$(gmeversion)/gme/Snes_Spc.h\
#	$(gmeversion)/gme/Spc_Cpu.h\
#	$(gmeversion)/gme/Spc_Dsp.h\
#	$(gmeversion)/gme/Spc_Emu.h\
#	$(gmeversion)/gme/Vgm_Emu.h\
#	$(gmeversion)/gme/Vgm_Emu_Impl.h\
#	$(gmeversion)/gme/Ym2413_Emu.h\
#	$(gmeversion)/gme/Ym2612_Emu.h\
#	$(gmeversion)/gme/gme_types.h\
#	$(gmeversion)/gme/Spc_Filter.cpp\
#	$(gmeversion)/gme/Spc_Filter.h

gme_la_LDFLAGS = -module -nostdlib -lsupc++
gme_la_LIBADD = $(ZLIB_LIBS)

AM_CFLAGS = $(CFLAGS) $(ZLIB_CFLAGS) -I$(gmepath) -std=c99 -DGME_VERSION_055
AM_CPPFLAGS = $(CXXFLAGS) -fno-exceptions -fno-rtti -nostdlib -fno-unwind-tables
endif
