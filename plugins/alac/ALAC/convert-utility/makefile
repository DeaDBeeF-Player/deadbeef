# alacconvert make

CFLAGS = -g -O3 -c
LFLAGS = -Wall -L$(CODECDIR) -lalac
CC = g++

SRCDIR = .
CODECDIR = ../codec
OBJDIR = ./obj
INCLUDES = ../codec

HEADERS = \
$(SRCDIR)/CAFFileALAC.h \
$(CODECDIR)/EndianPortable.h \
$(CODECDIR)/aglib.h \
$(CODECDIR)/ALACAudioTypes.h \
$(CODECDIR)/ALACBitUtilities.h\
$(CODECDIR)/ALACDecoder.h \
$(CODECDIR)/ALACEncoder.h \
$(CODECDIR)/dplib.h \
$(CODECDIR)/matrixlib.h

SOURCES = \
$(SRCDIR)/main.cpp \
$(SRCDIR)/CAFFileALAC.cpp \

OBJS = \
main.o \
CAFFileALAC.o 

alacconvert:	$(OBJS)
	(cd $(CODECDIR); $(MAKE))
	$(CC) $(LFLAGS) $(OBJS) -o alacconvert

main.o : main.cpp
	$(CC) -I $(INCLUDES) $(CFLAGS) main.cpp

CAFFileALAC.o : CAFFileALAC.cpp
	$(CC) -I $(INCLUDES) $(CFLAGS) CAFFileALAC.cpp

clean:
	-rm $(OBJS) alacconvert
	




