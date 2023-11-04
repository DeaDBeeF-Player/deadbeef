# ,----------------------------------------------------------------------.
# |                                                                      |
# |  emu68 -- Makefile.am -- Use with automake to produce configure.in   |
# |                                                                      |
# |               Copyright (C) 1998-2015 Benjamin Gerard                |
# |                                                                      |
# |               <http://sourceforge.net/users/benjihan>                |
# |                                                                      |
# `----------------------------------------------------------------------'
# @configure_input@

ACLOCAL_AMFLAGS    = -I ../m4 --install
AM_CFLAGS          = $(ALL_CFLAGS) $(LIB_CFLAGS)
AM_CPPFLAGS        = -I$(top_srcdir) $(file68_inc) -DEMU68_EXPORT $(MONO)

if emu68_monolitic
MONO               = -DEMU68_MONOLITIC
linesources        = $(monoliticsources)
alternatesources   = $(splitedsources)
else
MONO               =
linesources        = $(splitedsources)
alternatesources   = $(monoliticsources)
endif

commonsources=\
 emu68.c error68.c getea68.c inst68.c ioplug68.c mem68.c 

monoliticsources=\
 lines68.c

splitedsources=\
 line0_68.c line1_68.c line2_68.c line3_68.c line4_68.c line5_68.c      \
 line6_68.c line7_68.c line8_68.c line9_68.c lineA_68.c lineB_68.c      \
 lineC_68.c lineD_68.c lineE_68.c lineF_68.c table68.c

mysources=\
 $(commonsources) $(linesources)

myheaders=\
 assert68.h cc68.h emu68.h emu68_api.h error68.h excep68.h inst68.h     \
 ioplug68.h macro68.h mem68.h srdef68.h struct68.h type68.h lines68.h

myinlines=\
inl68_arithmetic.h inl68_bcd.h inl68_bitmanip.h inl68_datamove.h        \
inl68_exception.h inl68_logic.h inl68_progctrl.h inl68_shifting.h       \
inl68_systctrl.h

extrasources=\
 lines/line0.c lines/line1.c lines/line2.c lines/line3.c lines/line4.c  \
 lines/line5.c lines/line6.c lines/line7.c lines/line8.c lines/line9.c  \
 lines/lineA.c lines/lineB.c lines/lineC.c lines/lineD.c lines/lineE.c  \
 lines/lineF.c lines/table.c

# ,----------------------------------------------------------------------.
# | Libtool library                                                      |
# `----------------------------------------------------------------------'

noinst_LTLIBRARIES  = libemu68.la
libemu68_la_SOURCES = $(mysources) $(myheaders)
libemu68_la_LDFLAGS = -avoid-version
noinst_HEADERS      = $(extrasources) $(alternatesources) $(myinlines)
