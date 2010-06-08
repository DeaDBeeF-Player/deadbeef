if HAVE_EMIDI
emidipath=@top_srcdir@/plugins/emidi

EXTRA_DIST = $(emidipath)/readme.txt

pkglib_LTLIBRARIES = emidi.la

emidi_la_SOURCES = plugin.c emidi.cpp emidi.h\
libem/CEnvelope.cpp\
libem/CMIDIMessage.cpp\
libem/CMIDIModule.cpp\
libem/COpllDevice.cpp\
libem/CPSGDrum.cpp\
libem/CSccDevice.cpp\
libem/CSMF.cpp\
libem/CSMFPlay.cpp\
libem/SccInst.h\
libem/SccWave.h\
libem/CEnvelope.hpp\
libem/CMIDIMessage.hpp\
libem/CMIDIModule.hpp\
libem/COpllDevice.hpp\
libem/CPSGDrum.hpp\
libem/CSccDevice.hpp\
libem/CSMF.hpp\
libem/CSMFPlay.hpp\
libem/DsaCommon.hpp\
libem/ISoundDevice.hpp\
libem/device/2413tone.h\
libem/device/2413tone.ill\
libem/device/281btone.h\
libem/device/dbgout.h\
libem/device/emu2149.c\
libem/device/emu2149.h\
libem/device/emu2212.c\
libem/device/emu2212.h\
libem/device/emu2413.c\
libem/device/emu2413.h\
libem/device/emu76489.c\
libem/device/emu76489.h\
libem/device/emu8950.c\
libem/device/emu8950.h\
libem/device/emuadpcm.c\
libem/device/emuadpcm.h\
libem/device/emutypes.h\
libem/device/LIBEMU.vcproj\
libem/device/sample2413.c\
libem/device/vrc7tone.h

emidi_la_LIBADD = -lstdc++
emidi_la_LDFLAGS = -module

AM_CFLAGS = $(CFLAGS) -std=c99 -I$(emidipath)/libem -fPIC
AM_CPPFLAGS = $(CXXFLAGS) -I$(emidipath)/libem

endif
