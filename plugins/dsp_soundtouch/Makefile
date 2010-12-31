CC=gcc
CXX=g++

OUT=ddb_dsp_soundtouch.so

soundtouch_path=soundtouch

CFLAGS+=-Wall -g -D_GNU_SOURCE -I$(soundtouch_path)/include -I../../include -std=c99 -fPIC -msse2
CXXFLAGS+=-Wall -g -D_GNU_SOURCE -I$(soundtouch_path)/include -I../../include -fPIC -msse2

LDFLAGS+=-shared -lm

SOURCES=plugin.c

CXX_SOURCES=st.cpp\
$(soundtouch_path)/source/SoundTouch/AAFilter.cpp\
$(soundtouch_path)/source/SoundTouch/BPMDetect.cpp\
$(soundtouch_path)/source/SoundTouch/cpu_detect_x86_gcc.cpp\
$(soundtouch_path)/source/SoundTouch/FIFOSampleBuffer.cpp\
$(soundtouch_path)/source/SoundTouch/FIRFilter.cpp\
$(soundtouch_path)/source/SoundTouch/mmx_optimized.cpp\
$(soundtouch_path)/source/SoundTouch/PeakFinder.cpp\
$(soundtouch_path)/source/SoundTouch/RateTransposer.cpp\
$(soundtouch_path)/source/SoundTouch/SoundTouch.cpp\
$(soundtouch_path)/source/SoundTouch/sse_optimized.cpp\
$(soundtouch_path)/source/SoundTouch/TDStretch.cpp

HEADERS=st.h\
$(soundtouch_path)/include/BPMDetect.h\
$(soundtouch_path)/include/FIFOSampleBuffer.h\
$(soundtouch_path)/include/FIFOSamplePipe.h\
$(soundtouch_path)/include/soundtouch_config.h\
$(soundtouch_path)/include/SoundTouch.h\
$(soundtouch_path)/include/STTypes.h

CXX_OBJECTS=$(CXX_SOURCES:.cpp=.o)
OBJECTS=$(SOURCES:.c=.o)

all: $(SOURCES) $(OUT)

$(OUT): $(OBJECTS) $(CXX_OBJECTS)
	$(CXX) $(LDFLAGS) $(OBJECTS) $(CXX_OBJECTS) -o $@

.c.o: $(HEADERS)
	$(CC) $(CFLAGS) $< -c -o $@

.cpp.o: $(HEADERS)
	$(CXX) $(CXXFLAGS) $< -c -o $@

clean:
	rm $(OBJECTS) $(CXX_OBJECTS) $(OUT)

