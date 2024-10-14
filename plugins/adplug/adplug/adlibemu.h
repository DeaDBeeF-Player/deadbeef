/*
 * ADLIBEMU.H
 * Copyright (C) 1998-2001 Ken Silverman
 * Ken Silverman's official web site: "http://www.advsys.net/ken"
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
 */

#ifndef _ADLIBEMU_H
#define _ADLIBEMU_H

#define MAXCELLS 18
#define WAVPREC 2048
#define FIFOSIZ 256

typedef struct
{
    float val, t, tinc, vol, sustain, amp, mfb;
    float a0, a1, a2, a3, decaymul, releasemul;
    short *waveform;
    long wavemask;
    void (*cellfunc)(void *, float);
    unsigned char flags, dum0, dum1, dum2;
} celltype;

typedef struct
{
    long nlvol[9], nrvol[9];
    long nlplc[9], nrplc[9];
    long rend;

    // private
    float AMPSCALE;
    long numspeakers, bytespersample;
    float recipsamp;
    celltype cell[MAXCELLS];
    signed short wavtable[WAVPREC*3];
    float nfrqmul[16];
    unsigned char adlibreg[256], ksl[8][16];
    unsigned char odrumstat;
    float *rptr[9], *nrptr[9];
    float rbuf[9][FIFOSIZ*2];
    float snd[FIFOSIZ*2];
    int initfirstime;
} adlibemu_context;

#ifdef USING_ASM
# error USING_ASM is broken in adplug (-fPIC, context)
#endif

void adlibinit(adlibemu_context *ctx, long dasamplerate,long danumspeakers,long dabytespersample);
void adlib0(adlibemu_context *ctx,long i,long v);
void adlibgetsample(adlibemu_context *ctx,unsigned char *sndptr,long numbytes);
void adlibsetvolume(adlibemu_context *ctx,int i);

#endif // _ADLIBEMU_H
