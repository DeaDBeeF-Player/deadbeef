default: runtests

BUILD=testbuild
.PHONY: runtests

CC=clang
CXX=clang++

ORIGIN=$(shell pwd)
STATIC_DEPS:=$(ORIGIN)/static-deps
STATIC_ROOT:=$(STATIC_DEPS)/lib-x86-64
INCLUDE=-I external/googletest/googletest \
	-I external/googletest/googletest/include \
	-I external/mp4p/include \
	-I plugins/libparser \
	-I shared \
	-I . \
	-I src \
	-I include \
	-I src/ConvertUTF \
	-I src/scriptable \
	-I plugins/coreaudio \
	-I$(STATIC_ROOT)/include

CFLAGS=-fblocks -fcommon -O3 $(INCLUDE) \
	-D_FORTIFY_SOURCE=0 \
	-D_GNU_SOURCE \
	-DHAVE_LOG2=1 \
	-DDOCDIR=\"\" \
	-DPREFIX=\"\" \
	-DLIBDIR=\"\" \
	-DVERSION=\"\" \
	-DUSE_LIBMAD \
	-DUSE_LIBMPG123 \
	-DXCTEST \
	-DGOOGLETEST_STATIC
LIBRARIES=-lmad -lmpg123 -lcurl -ldispatch -lpthread -lBlocksRuntime -lm -ljansson -ldl
LDFLAGS=-L$(STATIC_ROOT)/lib -L$(STATIC_ROOT)/lib/x86_64-linux-gnu


TEST_C_SOURCES=$(wildcard src/*.c \
	shared/*.c \
	src/scriptable/*.c \
	shared/scriptable/*.c \
	plugins/libparser/*.c \
	plugins/nullout/*.c \
	src/ConvertUTF/*.c \
	src/metadata/*.c \
	plugins/m3u/*.c \
	plugins/vfs_curl/*.c \
	plugins/shellexec/*.c \
	external/mp4p/src/*.c \
	external/wcwidth/*.c \
	src/md5/*.c \
	plugins/mp3/*.c \
	Tests/*.c)
TEST_C_SOURCES:=$(filter-out src/main.c,$(TEST_C_SOURCES))
TEST_C_OBJS:=$(addprefix $(BUILD)/,\
	$(notdir $(patsubst %.c,%.o,$(TEST_C_SOURCES))) \
)
ifdef DDB_TEST_SUITES
	TEST_CPP_SOURCES=$(addprefix Tests/,$(addsuffix Tests.cpp,$(DDB_TEST_SUITES)))
else
	TEST_CPP_SOURCES=$(wildcard Tests/*Tests.cpp)
endif
TEST_CPP_SOURCES:=Tests/gtest-runner.cpp $(TEST_CPP_SOURCES)
TEST_CPP_OBJS:=$(addprefix $(BUILD)/,\
	$(notdir $(patsubst %.cpp,%.o,$(TEST_CPP_SOURCES))) \
)

GOOGLE_TEST_SOURCES=external/googletest/googletest/src/gtest-all.cc
GOOGLE_TEST_OBJS:=$(addprefix $(BUILD)/,$(notdir $(patsubst %.cc,%.o,$(GOOGLE_TEST_SOURCES))))

VPATH=src \
	$(addprefix src/,scriptable ConvertUTF md5 metadata) \
	$(addprefix plugins/,libparser m3u mp3 nullout shellexec vfs_curl) \
	$(addprefix external/,mp4p/src wcwidth googletest/googletest/src) \
	shared \
	shared/scriptable \
	Tests

$(BUILD)/%.o: %.c
	$(CC) -std=c99 $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: %.cpp
	$(CXX) -std=c++14 $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: %.cc
	$(CXX) -std=c++14 $(CFLAGS) -c $< -o $@

$(BUILD)/runtests: $(TEST_C_OBJS) $(GOOGLE_TEST_OBJS) $(TEST_CPP_OBJS)
	$(CXX) $(LDFLAGS) $(TEST_C_OBJS) $(GOOGLE_TEST_OBJS) $(TEST_CPP_OBJS) $(LIBRARIES) -o $@

runtests: $(BUILD)/runtests
	./$(BUILD)/runtests

$(BUILD)/Test%: $(TEST_C_OBJS) $(GOOGLE_TEST_OBJS) $(BUILD)/%Tests.o $(BUILD)/gtest-runner.o
	$(CXX) $(LDFLAGS) $^ $(LIBRARIES) -o $@

Test%: $(BUILD)/Test%
	$<

clean:
	rm -f $(BUILD)/*.o
	rm -f $(BUILD)/runtests
