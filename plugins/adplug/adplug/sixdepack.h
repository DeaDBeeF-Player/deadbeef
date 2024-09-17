/*
 * Adplug - Replayer for many OPL2/OPL3 audio file formats.
 * Copyright (C) 1999 - 2007 Simon Peter, <dn.tlp@gmx.net>, et al.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * sixdepack.h - Based on sixpack.c by Philip G. Gage, April 1991
 *
 */

#ifndef SIXDEPACK_H
#define SIXDEPACK_H

#include <string>
#include <assert.h>

class Sixdepak {
public:
    enum {
        COPYRANGES = 6,
        MINCOPY = 3,
        MAXCOPY = 255,
        CODESPERRANGE = MAXCOPY - MINCOPY + 1,
        ROOT = 1,
        TERMINATE = 256,
        FIRSTCODE = 257,
        MAXCHAR = FIRSTCODE + COPYRANGES * CODESPERRANGE - 1,
        SUCCMAX = MAXCHAR + 1,
        TWICEMAX = 2 * MAXCHAR + 1,
        MAXFREQ = 2000,
        MAXDISTANCE = 21839, // (1 << copybits(COPYRANGES-1)) - 1 + copymin(COPYRANGES-1)
        MAXSIZE = MAXDISTANCE + MAXCOPY,
        MAXBUF = 42 * 1024,
    };

    static size_t decode(unsigned short *source, size_t srcbytes, unsigned char *dest, size_t dstbytes);

private:
    static unsigned short bitvalue(unsigned short bit);
    static unsigned short copybits(unsigned short range);
    static unsigned short copymin(unsigned short range);

    void inittree();
    void updatefreq(unsigned short a, unsigned short b);
    void updatemodel(unsigned short code);
    unsigned short inputcode(unsigned short bits);
    unsigned short uncompress();
    size_t do_decode();
    Sixdepak(unsigned short *in, size_t isize, unsigned char *out, size_t osize);

    unsigned short ibitcount, ibitbuffer;
    unsigned short leftc[MAXCHAR + 1], rghtc[MAXCHAR + 1];
    unsigned short dad[TWICEMAX + 1], freq[TWICEMAX + 1];
    size_t ibufcount, input_size, output_size;
    unsigned short *wdbuf;
    unsigned char *obuf;
};

#endif
