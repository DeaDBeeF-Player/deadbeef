if HAVE_SOUNDTOUCH
soundtouchpath=@top_srcdir@/plugins/soundtouch/soundtouch

pkglib_LTLIBRARIES = ddb_soundtouch.la

ddb_soundtouch_la_CFLAGS = $(CFLAGS) -std=c99 -I$(soundtouchpath)/include -I$(soundtouchpath)/source/soundtouch -I@top_srcdir@/include -fPIC
ddb_soundtouch_la_LDFLAGS = -module -avoid-version $(NOCPPLIB) -lm

ddb_soundtouch_la_CXXFLAGS = $(CXXFLAGS) -I$(soundtouchpath)/include -I$(soundtouchpath)/source/soundtouch -I@top_srcdir@/include
if HAVE_SSE2
ddb_soundtouch_la_CXXFLAGS += -msse2
endif

ddb_soundtouch_la_SOURCES = \
	plugin.c\
	st.cpp\
	st.h\
    soundtouch/include/BPMDetect.h\
    soundtouch/include/FIFOSampleBuffer.h\
    soundtouch/include/FIFOSamplePipe.h\
    soundtouch/include/STTypes.h\
    soundtouch/include/SoundTouch.h\
    soundtouch/include/soundtouch_config.h\
    soundtouch/source/SoundTouch/AAFilter.cpp\
    soundtouch/source/SoundTouch/AAFilter.h\
    soundtouch/source/SoundTouch/BPMDetect.cpp\
    soundtouch/source/SoundTouch/FIFOSampleBuffer.cpp\
    soundtouch/source/SoundTouch/FIRFilter.cpp\
    soundtouch/source/SoundTouch/FIRFilter.h\
    soundtouch/source/SoundTouch/InterpolateCubic.cpp\
    soundtouch/source/SoundTouch/InterpolateCubic.h\
    soundtouch/source/SoundTouch/InterpolateLinear.cpp\
    soundtouch/source/SoundTouch/InterpolateLinear.h\
    soundtouch/source/SoundTouch/InterpolateShannon.cpp\
    soundtouch/source/SoundTouch/InterpolateShannon.h\
    soundtouch/source/SoundTouch/PeakFinder.cpp\
    soundtouch/source/SoundTouch/PeakFinder.h\
    soundtouch/source/SoundTouch/RateTransposer.cpp\
    soundtouch/source/SoundTouch/RateTransposer.h\
    soundtouch/source/SoundTouch/SoundTouch.cpp\
    soundtouch/source/SoundTouch/SoundTouch.sln\
    soundtouch/source/SoundTouch/SoundTouch.vcxproj\
    soundtouch/source/SoundTouch/TDStretch.cpp\
    soundtouch/source/SoundTouch/TDStretch.h\
    soundtouch/source/SoundTouch/cpu_detect.h\
    soundtouch/source/SoundTouch/cpu_detect_x86.cpp\
    soundtouch/source/SoundTouch/mmx_optimized.cpp\
    soundtouch/source/SoundTouch/sse_optimized.cpp

endif
