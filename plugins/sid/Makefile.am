sidpath=@top_srcdir@/plugins/sid/sidplay-libs
EXTRA_DIST = \
	$(sidpath)/libsidutils/README $(sidpath)/libsidplay/README $(sidpath)/resid/README\
	$(sidpath)/libsidutils/COPYING $(sidpath)/libsidplay/COPYING $(sidpath)/resid/COPYING\
	$(sidpath)/libsidutils/AUTHORS $(sidpath)/libsidplay/AUTHORS $(sidpath)/resid/AUTHORS

if HAVE_SID
pkglib_LTLIBRARIES = sid.la
sid_la_SOURCES = plugin.c csid.cpp csid.h\
	sidplay-libs/libsidplay/src/mixer.cpp\
	sidplay-libs/libsidplay/src/player.cpp\
	sidplay-libs/builders/resid-builder/include/config.h\
	sidplay-libs/builders/resid-builder/include/sidplay/builders/resid.h\
	sidplay-libs/builders/resid-builder/src/resid-builder.cpp\
	sidplay-libs/builders/resid-builder/src/resid-emu.h\
	sidplay-libs/builders/resid-builder/src/resid.cpp\
	sidplay-libs/builders/resid-builder/win/VC/config.h\
	sidplay-libs/libsidplay/include/config.h\
	sidplay-libs/libsidplay/include/sidenv.h\
	sidplay-libs/libsidplay/include/sidplay/Buffer.h\
	sidplay-libs/libsidplay/include/sidplay/SidTune.h\
	sidplay-libs/libsidplay/include/sidplay/SmartPtr.h\
	sidplay-libs/libsidplay/include/sidplay/c64env.h\
	sidplay-libs/libsidplay/include/sidplay/component.h\
	sidplay-libs/libsidplay/include/sidplay/event.h\
	sidplay-libs/libsidplay/include/sidplay/sid2types.h\
	sidplay-libs/libsidplay/include/sidplay/sidbuilder.h\
	sidplay-libs/libsidplay/include/sidplay/sidconfig.h\
	sidplay-libs/libsidplay/include/sidplay/sidendian.h\
	sidplay-libs/libsidplay/include/sidplay/sidint.h\
	sidplay-libs/libsidplay/include/sidplay/sidplay2.h\
	sidplay-libs/libsidplay/include/sidplay/sidtypes.h\
	sidplay-libs/libsidplay/include/sidplay/sidusage.h\
	sidplay-libs/libsidplay/src/c64/c64cia.h\
	sidplay-libs/libsidplay/src/c64/c64vic.h\
	sidplay-libs/libsidplay/src/c64/c64xsid.h\
	sidplay-libs/libsidplay/src/config.cpp\
	sidplay-libs/libsidplay/src/event.cpp\
	sidplay-libs/libsidplay/src/kernal.bin\
	sidplay-libs/libsidplay/src/mos6510/conf6510.h\
	sidplay-libs/libsidplay/src/mos6510/cycle_based/mos6510c.h\
	sidplay-libs/libsidplay/src/mos6510/cycle_based/mos6510c.i\
	sidplay-libs/libsidplay/src/mos6510/cycle_based/sid6510c.h\
	sidplay-libs/libsidplay/src/mos6510/cycle_based/sid6510c.i\
	sidplay-libs/libsidplay/src/mos6510/mos6510.cpp\
	sidplay-libs/libsidplay/src/mos6510/mos6510.h\
	sidplay-libs/libsidplay/src/mos6510/opcodes.h\
	sidplay-libs/libsidplay/src/mos6526/mos6526.cpp\
	sidplay-libs/libsidplay/src/mos6526/mos6526.h\
	sidplay-libs/libsidplay/src/mos656x/mos656x.cpp\
	sidplay-libs/libsidplay/src/mos656x/mos656x.h\
	sidplay-libs/libsidplay/src/nullsid.h\
	sidplay-libs/libsidplay/src/player.h\
	sidplay-libs/libsidplay/src/poweron.bin\
	sidplay-libs/libsidplay/src/psiddrv.bin\
	sidplay-libs/libsidplay/src/psiddrv.cpp\
	sidplay-libs/libsidplay/src/reloc65.c\
	sidplay-libs/libsidplay/src/sid6526/sid6526.cpp\
	sidplay-libs/libsidplay/src/sid6526/sid6526.h\
	sidplay-libs/libsidplay/src/sidplay2.cpp\
	sidplay-libs/libsidplay/src/sidtune/IconInfo.cpp\
	sidplay-libs/libsidplay/src/sidtune/InfoFile.cpp\
	sidplay-libs/libsidplay/src/sidtune/MUS.cpp\
	sidplay-libs/libsidplay/src/sidtune/PP20.cpp\
	sidplay-libs/libsidplay/src/sidtune/PP20.h\
	sidplay-libs/libsidplay/src/sidtune/PP20_Defs.h\
	sidplay-libs/libsidplay/src/sidtune/PSID.cpp\
	sidplay-libs/libsidplay/src/sidtune/SidTune.cpp\
	sidplay-libs/libsidplay/src/sidtune/SidTuneCfg.h\
	sidplay-libs/libsidplay/src/sidtune/SidTuneTools.cpp\
	sidplay-libs/libsidplay/src/sidtune/SidTuneTools.h\
	sidplay-libs/libsidplay/src/xsid/xsid.cpp\
	sidplay-libs/libsidplay/src/xsid/xsid.h\
	sidplay-libs/libsidplay/unix/sidconfig.h\
	sidplay-libs/libsidplay/unix/sidint.h\
	sidplay-libs/libsidplay/win/VC/config.h\
	sidplay-libs/libsidplay/win/VC/sidconfig.h\
	sidplay-libs/libsidplay/win/VC/sidint.h\
	sidplay-libs/resid/envelope.cpp\
	sidplay-libs/resid/envelope.h\
	sidplay-libs/resid/extfilt.cpp\
	sidplay-libs/resid/extfilt.h\
	sidplay-libs/resid/filter.cpp\
	sidplay-libs/resid/filter.h\
	sidplay-libs/resid/pot.cpp\
	sidplay-libs/resid/pot.h\
	sidplay-libs/resid/sid.cpp\
	sidplay-libs/resid/sid.h\
	sidplay-libs/resid/siddefs.h\
	sidplay-libs/resid/spline.h\
	sidplay-libs/resid/version.cpp\
	sidplay-libs/resid/voice.cpp\
	sidplay-libs/resid/voice.h\
	sidplay-libs/resid/wave.cpp\
	sidplay-libs/resid/wave.h\
	sidplay-libs/resid/wave6581_PST.cpp\
	sidplay-libs/resid/wave6581_PS_.cpp\
	sidplay-libs/resid/wave6581_P_T.cpp\
	sidplay-libs/resid/wave6581__ST.cpp\
	sidplay-libs/resid/wave8580_PST.cpp\
	sidplay-libs/resid/wave8580_PS_.cpp\
	sidplay-libs/resid/wave8580_P_T.cpp\
	sidplay-libs/resid/wave8580__ST.cpp\
	sidplay-libs/unix/config.h
	
sid_la_LDFLAGS = -module -avoid-version $(NOCPPLIB)

sid_la_CFLAGS = $(CFLAGS) -std=c99 -I$(sidpath)/libsidplay/include -I$(sidpath)/builders/resid-builder/include -fPIC -I@top_srcdir@/include
sid_la_CXXFLAGS = $(CXXFLAGS) -DHAVE_UNIX -I$(sidpath) -I$(sidpath)/unix -I$(sidpath)/libsidplay -I$(sidpath)/libsidplay/include -I$(sidpath)/libsidplay/include/sidplay -I$(sidpath)/libsidutils/include/sidplay/utils -I$(sidpath)/builders/resid-builder/include/sidplay/builders -I$(sidpath)/builders/resid-builder/include -I@top_srcdir@/include -DHAVE_STRCASECMP -DHAVE_STRNCASECMP -fno-exceptions -fno-rtti -fno-unwind-tables

endif
