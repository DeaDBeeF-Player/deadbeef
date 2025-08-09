noinst_LTLIBRARIES = libscriptable.la

libscriptable_la_SOURCES = \
	scriptable.c scriptable.h\
   	scriptable_model.c scriptable_model.h\
	scriptable_dsp.c scriptable_dsp.h\
	scriptable_encoder.c scriptable_encoder.h

libscriptable_la_CFLAGS = -fPIC -std=c99 -I@top_srcdir@/include -I@top_srcdir@/shared $(DISPATCH_CFLAGS)

