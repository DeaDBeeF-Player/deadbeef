CC=gcc

dumbpath=dumb-kode54

CFLAGS+=-Wall -fPIC -D_GNU_SOURCE -I$(dumbpath)/include -std=c99

LDFLAGS+=-module -shared -lm

SOURCES=\
dumb-kode54/src/it/readam.c\
dumb-kode54/src/it/readstm.c\
dumb-kode54/src/it/loads3m.c\
dumb-kode54/src/it/loadxm.c\
dumb-kode54/src/it/ptmeffect.c\
dumb-kode54/src/it/loadstm.c\
dumb-kode54/src/it/readxm.c\
dumb-kode54/src/it/itorder.c\
dumb-kode54/src/it/loadpsm.c\
dumb-kode54/src/it/itload.c\
dumb-kode54/src/it/loadasy2.c\
dumb-kode54/src/it/readasy.c\
dumb-kode54/src/it/loadasy.c\
dumb-kode54/src/it/read669.c\
dumb-kode54/src/it/readdsmf.c\
dumb-kode54/src/it/reads3m.c\
dumb-kode54/src/it/readriff.c\
dumb-kode54/src/it/load6692.c\
dumb-kode54/src/it/read6692.c\
dumb-kode54/src/it/readmod2.c\
dumb-kode54/src/it/loadriff2.c\
dumb-kode54/src/it/xmeffect.c\
dumb-kode54/src/it/loadmod2.c\
dumb-kode54/src/it/itread.c\
dumb-kode54/src/it/itunload.c\
dumb-kode54/src/it/loadmtm2.c\
dumb-kode54/src/it/loadptm.c\
dumb-kode54/src/it/itload2.c\
dumb-kode54/src/it/loadmtm.c\
dumb-kode54/src/it/loadoldpsm.c\
dumb-kode54/src/it/loadstm2.c\
dumb-kode54/src/it/readstm2.c\
dumb-kode54/src/it/readptm.c\
dumb-kode54/src/it/itrender.c\
dumb-kode54/src/it/readoldpsm.c\
dumb-kode54/src/it/reads3m2.c\
dumb-kode54/src/it/readpsm.c\
dumb-kode54/src/it/itread2.c\
dumb-kode54/src/it/loads3m2.c\
dumb-kode54/src/it/loadpsm2.c\
dumb-kode54/src/it/loadxm2.c\
dumb-kode54/src/it/loadptm2.c\
dumb-kode54/src/it/readxm2.c\
dumb-kode54/src/it/itmisc.c\
dumb-kode54/src/it/loadoldpsm2.c\
dumb-kode54/src/it/readmod.c\
dumb-kode54/src/it/loadmod.c\
dumb-kode54/src/it/loadriff.c\
dumb-kode54/src/it/readmtm.c\
dumb-kode54/src/it/load669.c\
dumb-kode54/src/core/rendduh.c\
dumb-kode54/src/core/readduh.c\
dumb-kode54/src/core/duhlen.c\
dumb-kode54/src/core/loadduh.c\
dumb-kode54/src/core/unload.c\
dumb-kode54/src/core/duhtag.c\
dumb-kode54/src/core/register.c\
dumb-kode54/src/core/rawsig.c\
dumb-kode54/src/core/dumbfile.c\
dumb-kode54/src/core/rendsig.c\
dumb-kode54/src/core/makeduh.c\
dumb-kode54/src/core/atexit.c\
dumb-kode54/src/helpers/clickrem.c\
dumb-kode54/src/helpers/silence.c\
dumb-kode54/src/helpers/stdfile.c\
dumb-kode54/src/helpers/resample.c\
dumb-kode54/src/helpers/riff.c\
dumb-kode54/src/helpers/memfile.c\
dumb-kode54/src/helpers/sampbuf.c\
dumb-kode54/src/helpers/barray.c\
cdumb.c

OBJECTS=$(SOURCES:.c=.o)

OUT=dumb.so

all: $(SOURCES) $(OUT)

$(OUT): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -c -o $@

clean:
	rm $(OBJECTS) $(OUT)

