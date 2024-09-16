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
 * sixdepack.cpp - Based on sixpack.c by Philip G. Gage, April 1991
 *
 */

#include "sixdepack.h"

unsigned short Sixdepak::bitvalue(unsigned short bit)
{
    assert(bit < copybits(COPYRANGES - 1));
    return 1 << bit;
}

unsigned short Sixdepak::copybits(unsigned short range)
{
    assert(range < COPYRANGES);
    return 2 * range + 4; // 4, 6, 8, 10, 12, 14
}

unsigned short Sixdepak::copymin(unsigned short range)
{
    assert(range < COPYRANGES);
    /*
    if (range > 0 )
        return bitvalue(copybits(range - 1)) + copymin(range - 1);
    else
        return 0;
    */
    static const unsigned short table[COPYRANGES] = {
        0, 16, 80, 336, 1360, 5456};
    return table[range];
}

void Sixdepak::inittree()
{
    unsigned short i;

    for (i = 2; i <= TWICEMAX; i++) {
        dad[i] = i / 2;
        freq[i] = 1;
    }

    for (i = 1; i <= MAXCHAR; i++) {
        leftc[i] = 2 * i;
        rghtc[i] = 2 * i + 1;
    }
}

void Sixdepak::updatefreq(unsigned short a, unsigned short b)
{
    for (;;) {
        freq[dad[a]] = freq[a] + freq[b];
        a = dad[a];

        if (a == ROOT)
            break;

        if (leftc[dad[a]] == a)
            b = rghtc[dad[a]];
        else
            b = leftc[dad[a]];
    }

    if (freq[ROOT] == MAXFREQ)
        for (a = 1; a <= TWICEMAX; a++)
            freq[a] >>= 1;
}

void Sixdepak::updatemodel(unsigned short code)
{
    unsigned short a = code + SUCCMAX, b, c, code1, code2;

    freq[a]++;
    if (dad[a] != ROOT) {
        code1 = dad[a];
        if (leftc[code1] == a)
            updatefreq(a, rghtc[code1]);
        else
            updatefreq(a, leftc[code1]);

        do {
            code2 = dad[code1];
            if (leftc[code2] == code1)
                b = rghtc[code2];
            else
                b = leftc[code2];

            if (freq[a] > freq[b]) {
                if (leftc[code2] == code1)
                    rghtc[code2] = a;
                else
                    leftc[code2] = a;

                if (leftc[code1] == a) {
                    leftc[code1] = b;
                    c = rghtc[code1];
                } else {
                    rghtc[code1] = b;
                    c = leftc[code1];
                }

                dad[b] = code1;
                dad[a] = code2;
                updatefreq(b, c);
                a = b;
            }

            a = dad[a];
            code1 = dad[a];
        } while (code1 != ROOT);
    }
}

unsigned short Sixdepak::inputcode(unsigned short bits)
{
    unsigned short i, code = 0;

    for (i = 1; i <= bits; i++) {
        if (!ibitcount) {
            if (ibufcount == input_size)
                return 0;
            ibitbuffer = wdbuf[ibufcount];
            ibufcount++;
            ibitcount = 15;
        } else
            ibitcount--;

        if (ibitbuffer & 0x8000)
            code |= bitvalue(i - 1);
        ibitbuffer <<= 1;
    }

    return code;
}

unsigned short Sixdepak::uncompress()
{
    unsigned short a = 1;

    do {
        if (!ibitcount) {
            if (ibufcount == input_size)
                return TERMINATE;
            ibitbuffer = wdbuf[ibufcount];
            ibufcount++;
            ibitcount = 15;
        } else
            ibitcount--;

        if (ibitbuffer & 0x8000)
            a = rghtc[a];
        else
            a = leftc[a];
        ibitbuffer <<= 1;
    } while (a <= MAXCHAR);

    a -= SUCCMAX;
    updatemodel(a);
    return a;
}

size_t Sixdepak::do_decode()
{
    size_t obufcount = ibufcount = 0;
    ibitcount = 0;
    ibitbuffer = 0;

    inittree();

    for (;;) {
        unsigned short c = uncompress();

        if (c == TERMINATE) {
            return obufcount;
        } else if (c < 256) {
            if (obufcount == output_size)
                return output_size;

            obuf[obufcount++] = (unsigned char)c;
        } else {
            unsigned short t = c - FIRSTCODE,
                           index = t / CODESPERRANGE,
                           len = t + MINCOPY - index * CODESPERRANGE,
                           dist = inputcode(copybits(index)) + copymin(index) + len;

            for (int i = 0; i < len; i++) {
                if (obufcount == output_size)
                    return output_size;

                obuf[obufcount] = dist > obufcount ? 0 : obuf[obufcount - dist];
                obufcount++;
            }
        }
    }
}

Sixdepak::Sixdepak(
    unsigned short *in, size_t isize, unsigned char *out, size_t osize) : input_size(isize), output_size(osize), wdbuf(in), obuf(out)
{
}

size_t Sixdepak::decode(
    unsigned short *source, size_t srcbytes,
    unsigned char *dest, size_t dstbytes)
{
    if (srcbytes < 2 || srcbytes > MAXBUF - 4096 /*why?*/ || dstbytes < 1)
        return 0;
    // There is no real reason to enforce upper bounds, but removing
    // the checks changes behaviour for non-compliant inputs.
    if (dstbytes > MAXBUF)
        dstbytes = MAXBUF;

    // The constructor wants input size in words, not bytes.
    Sixdepak *decoder = new Sixdepak(source, srcbytes / 2, dest, dstbytes);

    size_t out_size = decoder->do_decode();

    delete decoder;
    return out_size;
}
