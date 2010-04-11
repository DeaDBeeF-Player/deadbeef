noinst_LIBRARIES = libsidplay2.a

sidpath=@top_srcdir@/plugins/sid/sidplay-libs

EXTRA_DIST = \
	$(sidpath)/libsidutils/README $(sidpath)/libsidplay/README $(sidpath)/resid/README\
	$(sidpath)/libsidutils/COPYING $(sidpath)/libsidplay/COPYING $(sidpath)/resid/COPYING\
	$(sidpath)/libsidutils/AUTHORS $(sidpath)/libsidplay/AUTHORS $(sidpath)/resid/AUTHORS

AM_CPPFLAGS = $(INSANE_CXXFLAGS) -DHAVE_UNIX -I$(sidpath)/unix -I$(sidpath)/libsidplay -I$(sidpath)/libsidplay/include -I$(sidpath)/libsidplay/include/sidplay -I$(sidpath)/libsidutils/include/sidplay/utils -I$(sidpath)/builders/resid-builder/include/sidplay/builders -I$(sidpath)/builders/resid-builder/include -DHAVE_STRCASECMP -DHAVE_STRNCASECMP -fPIC

libsidplay2_a_SOURCES = \
libsidplay/src/mixer.cpp\
libsidplay/src/player.cpp\
libsidplay/src/mos656x/mos656x.cpp\
libsidplay/src/mos6510/mos6510.cpp\
libsidplay/src/sid6526/sid6526.cpp\
libsidplay/src/sidtune/MUS.cpp\
libsidplay/src/sidtune/SidTuneTools.cpp\
libsidplay/src/sidtune/InfoFile.cpp\
libsidplay/src/sidtune/SidTune.cpp\
libsidplay/src/sidtune/PSID.cpp\
libsidplay/src/sidtune/PP20.cpp\
libsidplay/src/sidtune/IconInfo.cpp\
libsidplay/src/event.cpp\
libsidplay/src/psiddrv.cpp\
libsidplay/src/config.cpp\
libsidplay/src/sidplay2.cpp\
libsidplay/src/xsid/xsid.cpp\
libsidplay/src/mos6526/mos6526.cpp\
builders/resid-builder/src/resid.cpp\
builders/resid-builder/src/resid-builder.cpp\
resid/wave8580_PS_.cc\
resid/filter.cc\
resid/pot.cc\
resid/wave.cc\
resid/version.cc\
resid/wave6581__ST.cc\
resid/extfilt.cc\
resid/wave8580_PST.cc\
resid/wave6581_PST.cc\
resid/wave6581_P_T.cc\
resid/wave6581_PS_.cc\
resid/envelope.cc\
resid/voice.cc\
resid/sid.cc\
resid/wave8580__ST.cc\
resid/wave8580_P_T.cc\
libsidplay/src/reloc65.c\
unix/config.h\
libsidplay/win/VC/sidconfig.h\
libsidplay/win/VC/sidint.h\
libsidplay/win/VC/config.h\
libsidplay/unix/sidconfig.h\
libsidplay/unix/sidint.h\
libsidplay/src/xsid/xsid.h\
libsidplay/src/mos6526/mos6526.h\
libsidplay/src/sid6526/sid6526.h\
libsidplay/src/mos656x/mos656x.h\
libsidplay/src/c64/c64xsid.h\
libsidplay/src/c64/c64cia.h\
libsidplay/src/c64/c64vic.h\
libsidplay/src/nullsid.h\
libsidplay/src/sidtune/PP20_Defs.h\
libsidplay/src/sidtune/SidTuneTools.h\
libsidplay/src/sidtune/SidTuneCfg.h\
libsidplay/src/sidtune/PP20.h\
libsidplay/src/mos6510/cycle_based/sid6510c.h\
libsidplay/src/mos6510/cycle_based/mos6510c.h\
libsidplay/src/mos6510/mos6510.h\
libsidplay/src/mos6510/conf6510.h\
libsidplay/src/mos6510/opcodes.h\
libsidplay/src/player.h\
libsidplay/include/sidplay/SmartPtr.h\
libsidplay/include/sidplay/sidusage.h\
libsidplay/include/sidplay/sidtypes.h\
libsidplay/include/sidplay/sidplay2.h\
libsidplay/include/sidplay/sidconfig.h\
libsidplay/include/sidplay/SidTune.h\
libsidplay/include/sidplay/sid2types.h\
libsidplay/include/sidplay/c64env.h\
libsidplay/include/sidplay/sidbuilder.h\
libsidplay/include/sidplay/sidint.h\
libsidplay/include/sidplay/component.h\
libsidplay/include/sidplay/event.h\
libsidplay/include/sidplay/sidendian.h\
libsidplay/include/sidplay/Buffer.h\
libsidplay/include/sidenv.h\
libsidplay/include/config.h\
builders/resid-builder/win/VC/config.h\
builders/resid-builder/src/resid-emu.h\
builders/resid-builder/include/sidplay/builders/resid.h\
builders/resid-builder/include/config.h\
resid/voice.h\
resid/siddefs.h\
resid/wave.h\
resid/sid.h\
resid/filter.h\
resid/envelope.h\
resid/spline.h\
resid/pot.h\
resid/extfilt.h\
libsidplay/src/mos6510/cycle_based/mos6510c.i\
libsidplay/src/mos6510/cycle_based/sid6510c.i\
libsidplay/src/kernal.bin\
libsidplay/src/psiddrv.bin\
libsidplay/src/poweron.bin
