# libalac make

CFLAGS = -g -O3 -c
LFLAGS = -Wall
CC = g++

SRCDIR = .
OBJDIR = ./obj
INCLUDES = .

HEADERS = \
$(SRCDIR)/EndianPortable.h \
$(SRCDIR)/aglib.h \
$(SRCDIR)/ALACAudioTypes.h \
$(SRCDIR)/ALACBitUtilities.h\
$(SRCDIR)/ALACDecoder.h \
$(SRCDIR)/ALACEncoder.h \
$(SRCDIR)/dplib.h \
$(SRCDIR)/matrixlib.h

SOURCES = \
$(SRCDIR)/EndianPortable.c \
$(SRCDIR)/ALACBitUtilities.c \
$(SRCDIR)/ALACDecoder.cpp \
$(SRCDIR)/ALACEncoder.cpp \
$(SRCDIR)/ag_dec.c \
$(SRCDIR)/ag_enc.c \
$(SRCDIR)/dp_dec.c \
$(SRCDIR)/dp_enc.c \
$(SRCDIR)/matrix_dec.c \
$(SRCDIR)/matrix_enc.c

OBJS = \
EndianPortable.o \
ALACBitUtilities.o \
ALACDecoder.o \
ALACEncoder.o \
ag_dec.o \
ag_enc.o \
dp_dec.o \
dp_enc.o \
matrix_dec.o \
matrix_enc.o

libalac.a:	$(OBJS)
	ar rcs libalac.a $(OBJS)

EndianPortable.o : EndianPortable.c
	$(CC) -I $(INCLUDES) $(CFLAGS) EndianPortable.c

ALACBitUtilities.o : ALACBitUtilities.c
	$(CC) -I $(INCLUDES) $(CFLAGS) ALACBitUtilities.c

ALACDecoder.o : ALACDecoder.cpp
	$(CC) -I $(INCLUDES) $(CFLAGS) ALACDecoder.cpp

ALACEncoder.o : ALACEncoder.cpp
	$(CC) -I $(INCLUDES) $(CFLAGS) ALACEncoder.cpp

ag_dec.o : ag_dec.c
	$(CC) -I $(INCLUDES) $(CFLAGS) ag_dec.c

ag_enc.o : ag_enc.c
	$(CC) -I $(INCLUDES) $(CFLAGS) ag_enc.c

dp_dec.o : dp_dec.c
	$(CC) -I $(INCLUDES) $(CFLAGS) dp_dec.c

dp_enc.o : dp_enc.c
	$(CC) -I $(INCLUDES) $(CFLAGS) dp_enc.c

matrix_dec.o : matrix_dec.c
	$(CC) -I $(INCLUDES) $(CFLAGS) matrix_dec.c

matrix_enc.o : matrix_enc.c
	$(CC) -I $(INCLUDES) $(CFLAGS) matrix_enc.c
		
clean:
	-rm $(OBJS) libalac.a

