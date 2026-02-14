/*
 * WMA compatible decoder
 * Copyright (c) 2002 The FFmpeg Project.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/**
 * @file wmadec.c
 * WMA compatible decoder.
 */

#include <libasf/asf.h>
#include "wmadec.h"
#include "wmafixed.h"
#include "wmadata.h"

//#define trace(...) { fprintf (stderr, __VA_ARGS__); }
#define trace(fmt,...)
#define DEBUGF trace

static void wma_lsp_to_curve_init(WMADecodeContext *s, int frame_len);

/*declarations of statically allocated variables used to remove malloc calls*/

# define IBSS_ATTR

/*MDCT reconstruction windows*/
static fixed32 stat0[2048] IBSS_ATTR_WMA_XL_IRAM MEM_ALIGN_ATTR;
static fixed32 stat1[1024] IBSS_ATTR_WMA_XL_IRAM MEM_ALIGN_ATTR;
static fixed32 stat2[ 512] IBSS_ATTR_WMA_XL_IRAM MEM_ALIGN_ATTR;
static fixed32 stat3[ 256] IBSS_ATTR_WMA_XL_IRAM MEM_ALIGN_ATTR;
static fixed32 stat4[ 128] IBSS_ATTR_WMA_XL_IRAM MEM_ALIGN_ATTR;

/*VLC lookup tables*/
static uint16_t *runtabarray[2];
static uint16_t *levtabarray[2];         

static uint16_t runtab_big[1336]   MEM_ALIGN_ATTR;
static uint16_t runtab_small[1072] MEM_ALIGN_ATTR;
static uint16_t levtab_big[1336]   MEM_ALIGN_ATTR;
static uint16_t levtab_small[1072] MEM_ALIGN_ATTR;

#define VLCBUF1SIZE 4598
#define VLCBUF2SIZE 3574
#define VLCBUF3SIZE 360
#define VLCBUF4SIZE 540

/*putting these in IRAM actually makes PP slower*/

static VLC_TYPE vlcbuf1[VLCBUF1SIZE][2] IBSS_ATTR_WMA_XL_IRAM MEM_ALIGN_ATTR;
static VLC_TYPE vlcbuf2[VLCBUF2SIZE][2] MEM_ALIGN_ATTR;
/* This buffer gets reused for lsp tables */
static VLC_TYPE vlcbuf3[VLCBUF3SIZE][2] MEM_ALIGN_ATTR;
static VLC_TYPE vlcbuf4[VLCBUF4SIZE][2] MEM_ALIGN_ATTR;




/**
  * Apply MDCT window and add into output.
  *
  * We ensure that when the windows overlap their squared sum
  * is always 1 (MDCT reconstruction rule).
  *
  * The Vorbis I spec has a great diagram explaining this process.
  * See section 1.3.2.3 of http://xiph.org/vorbis/doc/Vorbis_I_spec.html
  */
 static void wma_window(WMADecodeContext *s, fixed32 *in, fixed32 *out)
 {
     //float *in = s->output;
     int block_len, bsize, n;

     /* left part */
     
     /* previous block was larger, so we'll use the size of the current 
      * block to set the window size*/
     if (s->block_len_bits <= s->prev_block_len_bits) {
         block_len = s->block_len;
         bsize = s->frame_len_bits - s->block_len_bits;

         vector_fmul_add_add(out, in, s->windows[bsize], block_len);

     } else {
         /*previous block was smaller or the same size, so use it's size to set the window length*/
         block_len = 1 << s->prev_block_len_bits;
         /*find the middle of the two overlapped blocks, this will be the first overlapped sample*/
         n = (s->block_len - block_len) / 2;
         bsize = s->frame_len_bits - s->prev_block_len_bits;

         vector_fmul_add_add(out+n, in+n, s->windows[bsize],  block_len);

         memcpy(out+n+block_len, in+n+block_len, n*sizeof(fixed32));
     }
    /* Advance to the end of the current block and prepare to window it for the next block.
     * Since the window function needs to be reversed, we do it backwards starting with the
     * last sample and moving towards the first
     */
     out += s->block_len;
     in += s->block_len;

     /* right part */
     if (s->block_len_bits <= s->next_block_len_bits) {
         block_len = s->block_len;
         bsize = s->frame_len_bits - s->block_len_bits;

         vector_fmul_reverse(out, in, s->windows[bsize], block_len);

     } else {
         block_len = 1 << s->next_block_len_bits;
         n = (s->block_len - block_len) / 2;
         bsize = s->frame_len_bits - s->next_block_len_bits;

         memcpy(out, in, n*sizeof(fixed32));

         vector_fmul_reverse(out+n, in+n, s->windows[bsize], block_len);

         memset(out+n+block_len, 0, n*sizeof(fixed32));
     }
 }




/* XXX: use same run/length optimization as mpeg decoders */
static void init_coef_vlc(VLC *vlc,
                          uint16_t **prun_table, uint16_t **plevel_table,
                          const CoefVLCTable *vlc_table, int tab)
{
    int n = vlc_table->n;
    const uint8_t *table_bits = vlc_table->huffbits;
    const uint32_t *table_codes = vlc_table->huffcodes;
    const uint16_t *levels_table = vlc_table->levels;
    uint16_t *run_table, *level_table;
    const uint16_t *p;
    int i, l, j, level;


    init_vlc(vlc, VLCBITS, n, table_bits, 1, 1, table_codes, 4, 4, INIT_VLC_USE_NEW_STATIC);

    run_table = runtabarray[tab];
    level_table= levtabarray[tab];

    p = levels_table;
    i = 2;
    level = 1;
    while (i < n)
    {
        l = *p++;
        for(j=0;j<l;++j)
        {
            run_table[i] = j;
            level_table[i] = level;
            ++i;
        }
        ++level;
    }
    *prun_table = run_table;
    *plevel_table = level_table;
}

const uint8_t ff_log2_tab[256]={
        0,0,1,1,2,2,2,2,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
        5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
        6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
        6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
        7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
        7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
        7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
        7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7
};


#define av_log2       av_log2_c
static inline av_const int av_log2_c(unsigned int v)
{
    int n = 0;
    if (v & 0xffff0000) {
        v >>= 16;
        n += 16;
    }
    if (v & 0xff00) {
        v >>= 8;
        n += 8;
    }
    n += ff_log2_tab[v];

    return n;
}


int wma_decode_init(WMADecodeContext* s, asf_waveformatex_t *wfx)
{
    
    int i, flags2;
    fixed32 *window;
    uint8_t *extradata;
    fixed64 bps1;
    fixed32 high_freq;
    fixed64 bps;
    int sample_rate1;
    int coef_vlc_table;
    //    int filehandle;
    #ifdef CPU_COLDFIRE
    coldfire_set_macsr(EMAC_FRACTIONAL | EMAC_SATURATE);
    #endif

    /*clear stereo setting to avoid glitches when switching stereo->mono*/
    s->channel_coded[0]=0;
    s->channel_coded[1]=0;
    s->ms_stereo=0;

    s->sample_rate = wfx->rate;
    s->nb_channels = wfx->channels;
    s->bit_rate = wfx->bitrate;
    s->block_align = wfx->blockalign;
    trace ("wma samplerate: %d\n", wfx->rate);

    if (wfx->codec_id == ASF_CODEC_ID_WMAV1) {
        s->version = 1;
    } else if (wfx->codec_id == ASF_CODEC_ID_WMAV2 ) {
        s->version = 2;
    } else {
        /*one of those other wma flavors that don't have GPLed decoders */
        trace ("invalid wma codec id: %d\n", wfx->codec_id);
        return -1;
    }

    trace ("wma version: %d\n", s->version);

    /* extract flag infos */
    flags2 = 0;
    extradata = wfx->data;
    if (s->version == 1 && wfx->datalen >= 4) {
        flags2 = extradata[2] | (extradata[3] << 8);
    }else if (s->version == 2 && wfx->datalen >= 6){
        flags2 = extradata[4] | (extradata[5] << 8);
    }
    s->use_exp_vlc = flags2 & 0x0001;
    s->use_bit_reservoir = flags2 & 0x0002;
    s->use_variable_block_len = flags2 & 0x0004;

    /* compute MDCT block size */
    if (s->sample_rate <= 16000){
        s->frame_len_bits = 9;
    }else if (s->sample_rate <= 22050 ||
             (s->sample_rate <= 32000 && s->version == 1)){
        s->frame_len_bits = 10;
    }else{
        s->frame_len_bits = 11;
    }
    s->frame_len = 1 << s->frame_len_bits;
    if (s-> use_variable_block_len)
    {
        int nb_max, nb;
        nb = ((flags2 >> 3) & 3) + 1;
        if ((s->bit_rate / s->nb_channels) >= 32000)
        {
            nb += 2;
        }
        nb_max = s->frame_len_bits - BLOCK_MIN_BITS;        //max is 11-7
        if (nb > nb_max)
            nb = nb_max;
        s->nb_block_sizes = nb + 1;
    }
    else
    {
        s->nb_block_sizes = 1;
    }

    /* init rate dependant parameters */
    s->use_noise_coding = 1;
    high_freq = itofix64(s->sample_rate) >> 1;


    /* if version 2, then the rates are normalized */
    sample_rate1 = s->sample_rate;
    if (s->version == 2)
    {
        if (sample_rate1 >= 44100)
            sample_rate1 = 44100;
        else if (sample_rate1 >= 22050)
            sample_rate1 = 22050;
        else if (sample_rate1 >= 16000)
            sample_rate1 = 16000;
        else if (sample_rate1 >= 11025)
            sample_rate1 = 11025;
        else if (sample_rate1 >= 8000)
            sample_rate1 = 8000;
    }

    fixed64 tmp = itofix64(s->bit_rate);
    fixed64 tmp2 = itofix64(s->nb_channels * s->sample_rate);
    bps = fixdiv64(tmp, tmp2);
    fixed64 tim = bps * s->frame_len;
    fixed64 tmpi = fixdiv64(tim,itofix64(8));
    s->byte_offset_bits = av_log2(fixtoi64(tmpi+0x8000)) + 2;

    /* compute high frequency value and choose if noise coding should
       be activated */
    bps1 = bps;
    if (s->nb_channels == 2)
        bps1 = fixmul32(bps,0x1999a);
    if (sample_rate1 == 44100)
    {
        if (bps1 >= 0x9c29)
            s->use_noise_coding = 0;
        else
            high_freq = fixmul32(high_freq,0x6666);
    }
    else if (sample_rate1 == 22050)
    {
        if (bps1 >= 0x128f6)
            s->use_noise_coding = 0;
        else if (bps1 >= 0xb852)
            high_freq = fixmul32(high_freq,0xb333);
        else
            high_freq = fixmul32(high_freq,0x999a);
    }
    else if (sample_rate1 == 16000)
    {
        if (bps > 0x8000)
            high_freq = fixmul32(high_freq,0x8000);
        else
            high_freq = fixmul32(high_freq,0x4ccd);
    }
    else if (sample_rate1 == 11025)
    {
        high_freq = fixmul32(high_freq,0xb333);
    }
    else if (sample_rate1 == 8000)
    {
        if (bps <= 0xa000)
        {
           high_freq = fixmul32(high_freq,0x8000);
        }
        else if (bps > 0xc000)
        {
            s->use_noise_coding = 0;
        }
        else
        {
            high_freq = fixmul32(high_freq,0xa666);
        }
    }
    else
    {
        if (bps >= 0xcccd)
        {
            high_freq = fixmul32(high_freq,0xc000);
        }
        else if (bps >= 0x999a)
        {
            high_freq = fixmul32(high_freq,0x999a);
        }
        else
        {
            high_freq = fixmul32(high_freq,0x8000);
        }
    }

    /* compute the scale factor band sizes for each MDCT block size */
    {
        int a, b, pos, lpos, k, block_len, i, j, n;
        const uint8_t *table;

        if (s->version == 1)
        {
            s->coefs_start = 3;
        }
        else
        {
            s->coefs_start = 0;
        }
        for(k = 0; k < s->nb_block_sizes; ++k)
        {
            block_len = s->frame_len >> k;

            if (s->version == 1)
            {
                lpos = 0;
                for(i=0;i<25;++i)
                {
                    a = wma_critical_freqs[i];
                    b = s->sample_rate;
                    pos = ((block_len * 2 * a)  + (b >> 1)) / b;
                    if (pos > block_len)
                        pos = block_len;
                    s->exponent_bands[0][i] = pos - lpos;
                    if (pos >= block_len)
                    {
                        ++i;
                        break;
                    }
                    lpos = pos;
                }
                s->exponent_sizes[0] = i;
            }
            else
            {
                /* hardcoded tables */
                table = NULL;
                a = s->frame_len_bits - BLOCK_MIN_BITS - k;
                if (a < 3)
                {
                    if (s->sample_rate >= 44100)
                        table = exponent_band_44100[a];
                    else if (s->sample_rate >= 32000)
                        table = exponent_band_32000[a];
                    else if (s->sample_rate >= 22050)
                        table = exponent_band_22050[a];
                }
                if (table)
                {
                    n = *table++;
                    for(i=0;i<n;++i)
                        s->exponent_bands[k][i] = table[i];
                    s->exponent_sizes[k] = n;
                }
                else
                {
                    j = 0;
                    lpos = 0;
                    for(i=0;i<25;++i)
                    {
                        a = wma_critical_freqs[i];
                        b = s->sample_rate;
                        pos = ((block_len * 2 * a)  + (b << 1)) / (4 * b);
                        pos <<= 2;
                        if (pos > block_len)
                            pos = block_len;
                        if (pos > lpos)
                            s->exponent_bands[k][j++] = pos - lpos;
                        if (pos >= block_len)
                            break;
                        lpos = pos;
                    }
                    s->exponent_sizes[k] = j;
                }
            }

            /* max number of coefs */
            s->coefs_end[k] = (s->frame_len - ((s->frame_len * 9) / 100)) >> k;
            /* high freq computation */

            fixed32 tmp1 = high_freq*2;            /* high_freq is a fixed32!*/
            fixed32 tmp2=itofix32(s->sample_rate>>1);
            s->high_band_start[k] = fixtoi32( fixdiv32(tmp1, tmp2) * (block_len>>1) +0x8000);

            /*
            s->high_band_start[k] = (int)((block_len * 2 * high_freq) /
                                          s->sample_rate + 0.5);*/

            n = s->exponent_sizes[k];
            j = 0;
            pos = 0;
            for(i=0;i<n;++i)
            {
                int start, end;
                start = pos;
                pos += s->exponent_bands[k][i];
                end = pos;
                if (start < s->high_band_start[k])
                    start = s->high_band_start[k];
                if (end > s->coefs_end[k])
                    end = s->coefs_end[k];
                if (end > start)
                    s->exponent_high_bands[k][j++] = end - start;
            }
            s->exponent_high_sizes[k] = j;
        }
    }

    /* ffmpeg uses malloc to only allocate as many window sizes as needed.  
    *  However, we're really only interested in the worst case memory usage.
    *  In the worst case you can have 5 window sizes, 128 doubling up 2048
    *  Smaller windows are handled differently.
    *  Since we don't have malloc, just statically allocate this
    */
    fixed32 *temp[5];
    temp[0] = stat0;
    temp[1] = stat1;
    temp[2] = stat2;
    temp[3] = stat3;
    temp[4] = stat4;

    /* init MDCT windows : simple sinus window */
    for(i = 0; i < s->nb_block_sizes; i++)
    {
        int n, j;
        fixed32 alpha;
        n = 1 << (s->frame_len_bits - i);
        window = temp[i];
         
         /* this calculates 0.5/(2*n) */
        alpha = (1<<15)>>(s->frame_len_bits - i+1);  
        for(j=0;j<n;++j)
        {
            fixed32 j2 = itofix32(j) + 0x8000;
            /*alpha between 0 and pi/2*/
            window[j] = fsincos(fixmul32(j2,alpha)<<16, 0); 
        }
        s->windows[i] = window;

    }

    s->reset_block_lengths = 1;

    if (s->use_noise_coding) /* init the noise generator */
    {
        /* LSP values are simply 2x the EXP values */
        if (s->use_exp_vlc)
        {
            s->noise_mult = 0x51f;
            /*unlikely, but we may have previoiusly used this table for LSP,
            so halve the values if needed*/
            if(noisetable_exp[0] == 0x0a) {
                for (i=0;i<NOISE_TAB_SIZE;++i)
                    noisetable_exp[i] >>= 1;  
            }
            s->noise_table = noisetable_exp;
        }
        else
        {
            s->noise_mult = 0xa3d;
            /*check that we haven't already doubled this table*/
            if(noisetable_exp[0] == 0x5) { 
                for (i=0;i<NOISE_TAB_SIZE;++i)
                    noisetable_exp[i] <<= 1;
            }
            s->noise_table = noisetable_exp;
        }
#if 0
/*TODO:  Rockbox has a dither function.  Consider using it for noise coding*/

/* We use a lookup table computered in advance, so no need to do this*/
        {
            unsigned int seed;
            fixed32 norm;
            seed = 1;
            norm = 0;   // PJJ: near as makes any diff to 0!
            for (i=0;i<NOISE_TAB_SIZE;++i)
            {
                seed = seed * 314159 + 1;
                s->noise_table[i] = itofix32((int)seed) * norm;
            }
        }
#endif

         s->hgain_vlc.table = vlcbuf4;
         s->hgain_vlc.table_allocated = VLCBUF4SIZE;
         init_vlc(&s->hgain_vlc, HGAINVLCBITS, sizeof(hgain_huffbits),
                  hgain_huffbits, 1, 1,
                  hgain_huffcodes, 2, 2, INIT_VLC_USE_NEW_STATIC);
    }

    if (s->use_exp_vlc)
    {

        s->exp_vlc.table = vlcbuf3;
        s->exp_vlc.table_allocated = VLCBUF3SIZE;

         init_vlc(&s->exp_vlc, EXPVLCBITS, sizeof(scale_huffbits),
                  scale_huffbits, 1, 1,
                  scale_huffcodes, 4, 4, INIT_VLC_USE_NEW_STATIC);
    }
    else
    {
        wma_lsp_to_curve_init(s, s->frame_len);
    }

    /* choose the VLC tables for the coefficients */
    coef_vlc_table = 2;
    if (s->sample_rate >= 32000)
    {
        if (bps1 < 0xb852)
            coef_vlc_table = 0;
        else if (bps1 < 0x128f6)
            coef_vlc_table = 1;
    }

    /* since the coef2 table is the biggest and that has index 2 in coef_vlcs
       it's safe to always assign like this */
    runtabarray[0] = runtab_big; runtabarray[1] = runtab_small;
    levtabarray[0] = levtab_big; levtabarray[1] = levtab_small;

    s->coef_vlc[0].table = vlcbuf1;
    s->coef_vlc[0].table_allocated = VLCBUF1SIZE;
    s->coef_vlc[1].table = vlcbuf2;
    s->coef_vlc[1].table_allocated = VLCBUF2SIZE;


    init_coef_vlc(&s->coef_vlc[0], &s->run_table[0], &s->level_table[0],
                  &coef_vlcs[coef_vlc_table * 2], 0);
    init_coef_vlc(&s->coef_vlc[1], &s->run_table[1], &s->level_table[1],
                  &coef_vlcs[coef_vlc_table * 2 + 1], 1);

    s->last_superframe_len = 0;
    s->last_bitoffset = 0;

    return 0;
}


/* compute x^-0.25 with an exponent and mantissa table. We use linear
   interpolation to reduce the mantissa table size at a small speed
   expense (linear interpolation approximately doubles the number of
   bits of precision). */
static inline fixed32 pow_m1_4(WMADecodeContext *s, fixed32 x)
{
    union {
        float f;
        unsigned int v;
    } u, t;
    unsigned int e, m;
    fixed32 a, b;

    u.f = fixtof64(x);
    e = u.v >> 23;
    m = (u.v >> (23 - LSP_POW_BITS)) & ((1 << LSP_POW_BITS) - 1);
    /* build interpolation scale: 1 <= t < 2. */
    t.v = ((u.v << LSP_POW_BITS) & ((1 << 23) - 1)) | (127 << 23);
    a = ((fixed32*)s->lsp_pow_m_table1)[m];
    b = ((fixed32*)s->lsp_pow_m_table2)[m];

    /* lsp_pow_e_table contains 32.32 format */
    /* TODO:  Since we're unlikely have value that cover the whole
     * IEEE754 range, we probably don't need to have all possible exponents */

    return (lsp_pow_e_table[e] * (a + fixmul32(b, ftofix32(t.f))) >>32);
}

static void wma_lsp_to_curve_init(WMADecodeContext *s, int frame_len)
{
    fixed32 wdel, a, b, temp2;
    int i;

    wdel = fixdiv32(itofix32(1),     itofix32(frame_len));
    for (i=0; i<frame_len; ++i)
    {
        /* TODO: can probably reuse the trig_init values here */
        fsincos((wdel*i)<<15, &temp2);
        /* get 3 bits headroom + 1 bit from not doubleing the values */
        s->lsp_cos_table[i] = temp2>>3;

    }
    /* NOTE: these two tables are needed to avoid two operations in
       pow_m1_4 */
    b = itofix32(1);
    int ix = 0;

    s->lsp_pow_m_table1 = &vlcbuf3[0];
    s->lsp_pow_m_table2 = &vlcbuf3[1<<LSP_POW_BITS];

    /*double check this later*/
    for(i=(1 << LSP_POW_BITS) - 1;i>=0;i--)
    {
        a = pow_a_table[ix++]<<4;
        ((fixed32*)s->lsp_pow_m_table1)[i] = 2 * a - b;
        ((fixed32*)s->lsp_pow_m_table2)[i] = b - a;
        b = a;
    }

}

/* NOTE: We use the same code as Vorbis here */
/* XXX: optimize it further with SSE/3Dnow */
static void wma_lsp_to_curve(WMADecodeContext *s,
                             fixed32 *out,
                             fixed32 *val_max_ptr,
                             int n,
                             fixed32 *lsp)
{
    int i, j;
    fixed32 p, q, w, v, val_max, temp2;

    val_max = 0;
    for(i=0;i<n;++i)
    {
        /* shift by 2 now to reduce rounding error,
         * we can renormalize right before pow_m1_4
         */

        p = 0x8000<<5;
        q = 0x8000<<5;
        w = s->lsp_cos_table[i];

        for (j=1;j<NB_LSP_COEFS;j+=2)
        {
            /* w is 5.27 format, lsp is in 16.16, temp2 becomes 5.27 format */
            temp2 = ((w - (lsp[j - 1]<<11)));

            /* q is 16.16 format, temp2 is 5.27, q becomes 16.16 */
            q = fixmul32b(q, temp2 )<<4;
            p = fixmul32b(p, (w - (lsp[j]<<11)))<<4;
        }

        /* 2 in 5.27 format is 0x10000000 */
        p = fixmul32(p, fixmul32b(p, (0x10000000 - w)))<<3;
        q = fixmul32(q, fixmul32b(q, (0x10000000 + w)))<<3;

        v = (p + q) >>9;  /* p/q end up as 16.16 */
        v = pow_m1_4(s, v);
        if (v > val_max)
            val_max = v;
        out[i] = v;
    }

    *val_max_ptr = val_max;
}

/* decode exponents coded with LSP coefficients (same idea as Vorbis)
 * only used for low bitrate (< 16kbps) files
 */
static void decode_exp_lsp(WMADecodeContext *s, int ch)
{
    fixed32 lsp_coefs[NB_LSP_COEFS];
    int val, i;

    for (i = 0; i < NB_LSP_COEFS; ++i)
    {
        if (i == 0 || i >= 8)
            val = get_bits(&s->gb, 3);
        else
            val = get_bits(&s->gb, 4);
        lsp_coefs[i] = lsp_codebook[i][val];
    }

    wma_lsp_to_curve(s,
                     s->exponents[ch],
                     &s->max_exponent[ch],
                     s->block_len,
                     lsp_coefs);
}

/* decode exponents coded with VLC codes - used for bitrate >= 32kbps*/
static int decode_exp_vlc(WMADecodeContext *s, int ch)
{
    int last_exp, n, code;
    const uint16_t *ptr, *band_ptr;
    fixed32 v, max_scale;
    fixed32 *q,*q_end;

    /*accommodate the 60 negative indices */
    const fixed32 *pow_10_to_yover16_ptr = &pow_10_to_yover16[61];

    band_ptr = s->exponent_bands[s->frame_len_bits - s->block_len_bits];
    ptr = band_ptr;
    q = s->exponents[ch];
    q_end = q + s->block_len;
    max_scale = 0;


    if (s->version == 1)        //wmav1 only
    {
        last_exp = get_bits(&s->gb, 5) + 10;

        v = pow_10_to_yover16_ptr[last_exp];
        max_scale = v;
        n = *ptr++;
        switch (n & 3) do {
            case 0: *q++ = v;
            case 3: *q++ = v;
            case 2: *q++ = v;
            case 1: *q++ = v;
        } while ((n -= 4) > 0);
    } else {
       last_exp = 36;
    }

    while (q < q_end)
    {
        code = get_vlc2(&s->gb, s->exp_vlc.table, EXPVLCBITS, EXPMAX);
        if (code < 0)
        {
            return -1;
        }
        /* NOTE: this offset is the same as MPEG4 AAC ! */
        last_exp += code - 60;

        if (last_exp > 68) {
            return -1;
        }

        v = pow_10_to_yover16_ptr[last_exp];
        if (v > max_scale)
        {
            max_scale = v;
        }
        n = *ptr++;
        switch (n & 3) do {
            case 0: *q++ = v;
            case 3: *q++ = v;
            case 2: *q++ = v;
            case 1: *q++ = v;
        } while ((n -= 4) > 0);
    }

    s->max_exponent[ch] = max_scale;
    return 0;
}

/* return 0 if OK. return 1 if last block of frame. return -1 if
   unrecorrable error. */
static int wma_decode_block(WMADecodeContext *s)
{
    int n, v, a, ch, code, bsize;
    int coef_nb_bits, total_gain;
    int nb_coefs[MAX_CHANNELS];

    memset (nb_coefs, 0, sizeof (nb_coefs));

    fixed32 mdct_norm;

    /*DEBUGF("***decode_block: %d  (%d samples of %d in frame)\n",  s->block_num, s->block_len, s->frame_len);*/

   /* compute current block length */
    if (s->use_variable_block_len)
    {
        n = av_log2(s->nb_block_sizes - 1) + 1;

        if (s->reset_block_lengths)
        {
            s->reset_block_lengths = 0;
            v = get_bits(&s->gb, n);
            if (v >= s->nb_block_sizes)
            {
                return -2;
            }
            s->prev_block_len_bits = s->frame_len_bits - v;
            v = get_bits(&s->gb, n);
            if (v >= s->nb_block_sizes)
            {
                return -3;
            }
            s->block_len_bits = s->frame_len_bits - v;
        }
        else
        {
            /* update block lengths */
            s->prev_block_len_bits = s->block_len_bits;
            s->block_len_bits = s->next_block_len_bits;
        }
        v = get_bits(&s->gb, n);

        if (v >= s->nb_block_sizes)
        {
         // rb->splash(HZ*4, "v was %d", v);        //5, 7
            return -4;        //this is it
        }
        else{
              //rb->splash(HZ, "passed v block (%d)!", v);
      }
        s->next_block_len_bits = s->frame_len_bits - v;
    }
    else
    {
        /* fixed block len */
        s->next_block_len_bits = s->frame_len_bits;
        s->prev_block_len_bits = s->frame_len_bits;
        s->block_len_bits = s->frame_len_bits;
    }
    /* now check if the block length is coherent with the frame length */
    s->block_len = 1 << s->block_len_bits;

    if ((s->block_pos + s->block_len) > s->frame_len)
    {
        return -5;  //oddly 32k sample from tracker fails here
    }

    if (s->nb_channels == 2)
    {
        s->ms_stereo = get_bits1(&s->gb);
    }
    v = 0;
    for (ch = 0; ch < s->nb_channels; ++ch)
    {
        a = get_bits1(&s->gb);
        s->channel_coded[ch] = a;
        v |= a;
    }
    /* if no channel coded, no need to go further */
    /* XXX: fix potential framing problems */
    if (!v)
    {
        goto next;
    }

    bsize = s->frame_len_bits - s->block_len_bits;

    /* read total gain and extract corresponding number of bits for
       coef escape coding */
    total_gain = 1;
    for(;;)
    {
        a = get_bits(&s->gb, 7);
        total_gain += a;
        if (a != 127)
        {
            break;
        }
    }

    if (total_gain < 15)
        coef_nb_bits = 13;
    else if (total_gain < 32)
        coef_nb_bits = 12;
    else if (total_gain < 40)
        coef_nb_bits = 11;
    else if (total_gain < 45)
        coef_nb_bits = 10;
    else
        coef_nb_bits = 9;

    /* compute number of coefficients */
    n = s->coefs_end[bsize] - s->coefs_start;

    for(ch = 0; ch < s->nb_channels; ++ch)
    {
        nb_coefs[ch] = n;
    }
    /* complex coding */
    if (s->use_noise_coding)
    {

        for(ch = 0; ch < s->nb_channels; ++ch)
        {
            if (s->channel_coded[ch])
            {
                int i, n, a;
                n = s->exponent_high_sizes[bsize];
                for(i=0;i<n;++i)
                {
                    a = get_bits1(&s->gb);
                    s->high_band_coded[ch][i] = a;
                    /* if noise coding, the coefficients are not transmitted */
                    if (a)
                        nb_coefs[ch] -= s->exponent_high_bands[bsize][i];
                }
            }
        }
        for(ch = 0; ch < s->nb_channels; ++ch)
        {
            if (s->channel_coded[ch])
            {
                int i, n, val, code;

                n = s->exponent_high_sizes[bsize];
                val = (int)0x80000000;
                for(i=0;i<n;++i)
                {
                    if (s->high_band_coded[ch][i])
                    {
                        if (val == (int)0x80000000)
                        {
                            val = get_bits(&s->gb, 7) - 19;
                        }
                        else
                        {
                            //code = get_vlc(&s->gb, &s->hgain_vlc);
                            code = get_vlc2(&s->gb, s->hgain_vlc.table, HGAINVLCBITS, HGAINMAX);
                            if (code < 0)
                            {
                                return -6;
                            }
                            val += code - 18;
                        }
                        s->high_band_values[ch][i] = val;
                    }
                }
            }
        }
    }

    /* exponents can be reused in short blocks. */
    if ((s->block_len_bits == s->frame_len_bits) || get_bits1(&s->gb))
    {
        for(ch = 0; ch < s->nb_channels; ++ch)
        {
            if (s->channel_coded[ch])
            {
                if (s->use_exp_vlc)
                {
                    if (decode_exp_vlc(s, ch) < 0)
                    {
                        return -7;
                    }
                }
                else
                {
                    decode_exp_lsp(s, ch);
                }
                s->exponents_bsize[ch] = bsize;
            }
        }
    }

    /* parse spectral coefficients : just RLE encoding */
    for(ch = 0; ch < s->nb_channels; ++ch)
    {
        if (s->channel_coded[ch])
        {
            VLC *coef_vlc;
            int level, run, sign, tindex;
            int16_t *ptr, *eptr;
            const int16_t *level_table, *run_table;

            /* special VLC tables are used for ms stereo because
               there is potentially less energy there */
            tindex = (ch == 1 && s->ms_stereo);
            coef_vlc = &s->coef_vlc[tindex];
            run_table = s->run_table[tindex];
            level_table = s->level_table[tindex];
            /* XXX: optimize */
            ptr = &s->coefs1[ch][0];
            eptr = ptr + nb_coefs[ch];
            memset(ptr, 0, s->block_len * sizeof(int16_t));

            for(;;)
            {
                code = get_vlc2(&s->gb, coef_vlc->table, VLCBITS, VLCMAX);

                if (code < 0)
                {
                    return -8;
                }
                if (code == 1)
                {
                    /* EOB */
                    break;
                }
                else if (code == 0)
                {
                    /* escape */
                    level = get_bits(&s->gb, coef_nb_bits);
                    /* NOTE: this is rather suboptimal. reading
                       block_len_bits would be better */
                    run = get_bits(&s->gb, s->frame_len_bits);
                }
                else
                {
                    /* normal code */
                    run = run_table[code];
                    level = level_table[code];
                }
                sign = get_bits1(&s->gb);
                if (!sign)
                    level = -level;
                ptr += run;
                if (ptr >= eptr)
                {
                    break;
                }
                *ptr++ = level;


                /* NOTE: EOB can be omitted */
                if (ptr >= eptr)
                    break;
            }
        }
        if (s->version == 1 && s->nb_channels >= 2)
        {
            align_get_bits(&s->gb);
        }
    }

    {
        int n4 = s->block_len >> 1;


        mdct_norm = 0x10000>>(s->block_len_bits-1);

        if (s->version == 1)
        {
             mdct_norm *= fixtoi32(fixsqrt32(itofix32(n4)));
        }
    }


   /* finally compute the MDCT coefficients */
    for(ch = 0; ch < s->nb_channels; ++ch)
    {
        if (s->channel_coded[ch])
        {
            int16_t *coefs1;
            fixed32 *exponents;
            fixed32 *coefs, atemp;
            fixed64 mult;
            fixed64 mult1;
            fixed32 noise, temp1, temp2, mult2;
            int i, j, n, n1, last_high_band, esize;
            fixed32 exp_power[HIGH_BAND_MAX_SIZE];

            //total_gain, coefs1, mdctnorm are lossless

            coefs1 = s->coefs1[ch];
            exponents = s->exponents[ch];
            esize = s->exponents_bsize[ch];
            coefs = s->coefs[ch];
            n=0;

          /*
          *  The calculation of coefs has a shift right by 2 built in.  This
          *  prepares samples for the Tremor IMDCT which uses a slightly
          *  different fixed format then the ffmpeg one. If the old ffmpeg
          *  imdct is used, each shift storing into coefs should be reduced
          *  by 1.
          *  See SVN logs for details.
          */


            if (s->use_noise_coding)
            {   
                /*This case is only used for low bitrates (typically less then 32kbps)*/
                
                /*TODO:  mult should be converted to 32 bit to speed up noise coding*/

                mult = fixdiv64(pow_table[total_gain+20],Fixed32To64(s->max_exponent[ch]));
                mult = mult* mdct_norm;
                mult1 = mult;

                /* very low freqs : noise */
                for(i = 0;i < s->coefs_start; ++i)
                {
                    *coefs++ = fixmul32( (fixmul32(s->noise_table[s->noise_index],
                            exponents[i<<bsize>>esize])>>4),Fixed32From64(mult1)) >>2;
                    s->noise_index = (s->noise_index + 1) & (NOISE_TAB_SIZE - 1);
                }

                n1 = s->exponent_high_sizes[bsize];

                /* compute power of high bands */
                exponents = s->exponents[ch] +(s->high_band_start[bsize]<<bsize);
                last_high_band = 0; /* avoid warning */
                for (j=0;j<n1;++j)
                {
                    n = s->exponent_high_bands[s->frame_len_bits -
                                               s->block_len_bits][j];
                    if (s->high_band_coded[ch][j])
                    {
                        fixed32 e2, v;
                        e2 = 0;
                        for(i = 0;i < n; ++i)
                        {
                            /*v is normalized later on so its fixed format is irrelevant*/
                            v = exponents[i<<bsize>>esize]>>4;
                            e2 += fixmul32(v, v)>>3;
                        }
                         exp_power[j] = e2/n; /*n is an int...*/
                        last_high_band = j;
                    }
                    exponents += n<<bsize;
                }

                /* main freqs and high freqs */
                exponents = s->exponents[ch] + (s->coefs_start<<bsize);
                for(j=-1;j<n1;++j)
                {
                    if (j < 0)
                    {
                        n = s->high_band_start[bsize] -
                            s->coefs_start;
                    }
                    else
                    {
                        n = s->exponent_high_bands[s->frame_len_bits -
                                                   s->block_len_bits][j];
                    }
                    if (j >= 0 && s->high_band_coded[ch][j])
                    {
                        /* use noise with specified power */
                        fixed32 tmp = fixdiv32(exp_power[j],exp_power[last_high_band]);

                        /*mult1 is 48.16, pow_table is 48.16*/
                        mult1 = fixmul32(fixsqrt32(tmp),
                                pow_table[s->high_band_values[ch][j]+20]) >> 16;

                        /*this step has a fairly high degree of error for some reason*/
                        mult1 = fixdiv64(mult1,fixmul32(s->max_exponent[ch],s->noise_mult));
                        mult1 = mult1*mdct_norm>>PRECISION;
                        for(i = 0;i < n; ++i)
                        {
                            noise = s->noise_table[s->noise_index];
                            s->noise_index = (s->noise_index + 1) & (NOISE_TAB_SIZE - 1);
                            *coefs++ = fixmul32((fixmul32(exponents[i<<bsize>>esize],noise)>>4),
                                    Fixed32From64(mult1)) >>2;

                        }
                        exponents += n<<bsize;
                    }
                    else
                    {
                        /* coded values + small noise */
                        for(i = 0;i < n; ++i)
                        {
                            noise = s->noise_table[s->noise_index];
                            s->noise_index = (s->noise_index + 1) & (NOISE_TAB_SIZE - 1);

                           /*don't forget to renormalize the noise*/
                           temp1 = (((int32_t)*coefs1++)<<16) + (noise>>4);
                           temp2 = fixmul32(exponents[i<<bsize>>esize], mult>>18);
                           *coefs++ = fixmul32(temp1, temp2);
                        }
                        exponents += n<<bsize;
                    }
                }

                /* very high freqs : noise */
                n = s->block_len - s->coefs_end[bsize];
                mult2 = fixmul32(mult>>16,exponents[((-1<<bsize))>>esize]) ;
                for (i = 0; i < n; ++i)
                {
                    /*renormalize the noise product and then reduce to 14.18 precison*/
                    *coefs++ = fixmul32(s->noise_table[s->noise_index],mult2) >>6;

                    s->noise_index = (s->noise_index + 1) & (NOISE_TAB_SIZE - 1);
                }
            }
            else
            {
                /*Noise coding not used, simply convert from exp to fixed representation*/

                fixed32 mult3 = (fixed32)(fixdiv64(pow_table[total_gain+20],
                        Fixed32To64(s->max_exponent[ch])));
                mult3 = fixmul32(mult3, mdct_norm);

                /*zero the first 3 coefficients for WMA V1, does nothing otherwise*/
                for(i=0; i<s->coefs_start; i++)
                    *coefs++=0;

                n = nb_coefs[ch];

                /* XXX: optimize more, unrolling this loop in asm
                                might be a good idea */

                for(i = 0;i < n; ++i)
                {
                    /*ffmpeg imdct needs 15.17, while tremor 14.18*/
                    atemp = (coefs1[i] * mult3)>>2;
                    *coefs++=fixmul32(atemp,exponents[i<<bsize>>esize]);
                }
                n = s->block_len - s->coefs_end[bsize];
                memset(coefs, 0, n*sizeof(fixed32));
            }
        }
    }



    if (s->ms_stereo && s->channel_coded[1])
    {
        fixed32 a, b;
        int i;
        /* nominal case for ms stereo: we do it before mdct */
        /* no need to optimize this case because it should almost
           never happen */
        if (!s->channel_coded[0])
        {
            memset(s->coefs[0], 0, sizeof(fixed32) * s->block_len);
            s->channel_coded[0] = 1;
        }

        for(i = 0; i < s->block_len; ++i)
        {
            a = s->coefs[0][i];
            b = s->coefs[1][i];
            s->coefs[0][i] = a + b;
            s->coefs[1][i] = a - b;
        }
    }

    for(ch = 0; ch < s->nb_channels; ++ch)
    { 
        /* BLOCK_MAX_SIZE is 2048 (samples) and MAX_CHANNELS is 2. */
        static uint32_t scratch_buf[BLOCK_MAX_SIZE * MAX_CHANNELS] IBSS_ATTR MEM_ALIGN_ATTR;
        if (s->channel_coded[ch])
        {
            int n4, index;

            n4 = s->block_len >>1;

            ff_imdct_calc((s->frame_len_bits - bsize + 1),
                          scratch_buf,
                          s->coefs[ch]);

            /* add in the frame */
            index = (s->frame_len / 2) + s->block_pos - n4;
            wma_window(s, scratch_buf, &(s->frame_out[ch][index]));



            /* specific fast case for ms-stereo : add to second
               channel if it is not coded */
            if (s->ms_stereo && !s->channel_coded[1])
            {
                wma_window(s, scratch_buf, &(s->frame_out[1][index]));
            }
        }
    }
next:
    /* update block number */
    ++s->block_num;
    s->block_pos += s->block_len;
    if (s->block_pos >= s->frame_len)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

/* decode a frame of frame_len samples */
static int wma_decode_frame(WMADecodeContext *s)
{
    int ret;

    /* read each block */
    s->block_num = 0;
    s->block_pos = 0;


    for(;;)
    {
        ret = wma_decode_block(s);
        if (ret < 0)
        {

            DEBUGF("wma_decode_block failed with code %d\n", ret);
            return -1;
        }
        if (ret)
        {
            break;
        }
    }
    
    return 0;
}

/* Initialise the superframe decoding */

int wma_decode_superframe_init(WMADecodeContext* s,
                                 const uint8_t *buf,  /*input*/
                                 int buf_size)
{
    if (buf_size==0)
    {
        s->last_superframe_len = 0;
        return 0;
    }

    s->current_frame = 0;

    init_get_bits(&s->gb, buf, buf_size*8);

    if (s->use_bit_reservoir)
    {
        /* read super frame header */
        skip_bits(&s->gb, 4); /* super frame index */
        s->nb_frames = get_bits(&s->gb, 4);

        if (s->last_superframe_len == 0)
            s->nb_frames --;
        else if (s->nb_frames == 0)
            s->nb_frames++;

        s->bit_offset = get_bits(&s->gb, s->byte_offset_bits + 3);
    } else {
        s->nb_frames = 1;
    }

    return 1;
}


/* Decode a single frame in the current superframe - return -1 if
   there was a decoding error, or the number of samples decoded.
*/

int wma_decode_superframe_frame(WMADecodeContext* s,
                                const uint8_t *buf,  /*input*/
                                int buf_size)
{
    int pos, len, ch;
    uint8_t *q;
    int done = 0;
    
    for(ch = 0; ch < s->nb_channels; ch++)
        memmove(&(s->frame_out[ch][0]), 
                &(s->frame_out[ch][s->frame_len]),
                s->frame_len * sizeof(fixed32));
    
    if ((s->use_bit_reservoir) && (s->current_frame == 0))
    {
        if (s->last_superframe_len > 0)
        {
            /* add s->bit_offset bits to last frame */
            if ((s->last_superframe_len + ((s->bit_offset + 7) >> 3)) >
                    MAX_CODED_SUPERFRAME_SIZE)
            {
                DEBUGF("superframe size too large error\n");
                goto fail;
            }
            q = s->last_superframe + s->last_superframe_len;
            len = s->bit_offset;
            while (len > 7)
            {
                *q++ = (get_bits)(&s->gb, 8);
                len -= 8;
            }
            if (len > 0)
            {
                *q++ = (get_bits)(&s->gb, len) << (8 - len);
            }

            /* XXX: s->bit_offset bits into last frame */
            init_get_bits(&s->gb, s->last_superframe, MAX_CODED_SUPERFRAME_SIZE*8);
            /* skip unused bits */
            if (s->last_bitoffset > 0)
                skip_bits(&s->gb, s->last_bitoffset);

            /* this frame is stored in the last superframe and in the
               current one */
            if (wma_decode_frame(s) < 0)
            {
                goto fail;
            }
            done = 1;
        }

        /* read each frame starting from s->bit_offset */
        pos = s->bit_offset + 4 + 4 + s->byte_offset_bits + 3;
        init_get_bits(&s->gb, buf + (pos >> 3), (MAX_CODED_SUPERFRAME_SIZE - (pos >> 3))*8);
        len = pos & 7;
        if (len > 0)
            skip_bits(&s->gb, len);

        s->reset_block_lengths = 1;
    }

    /* If we haven't decoded a frame yet, do it now */
    if (!done)
        {
            if (wma_decode_frame(s) < 0)
            {
                goto fail;
            }
        }

    s->current_frame++;

    if ((s->use_bit_reservoir) && (s->current_frame == s->nb_frames))
    {
        /* we copy the end of the frame in the last frame buffer */
        pos = get_bits_count(&s->gb) + ((s->bit_offset + 4 + 4 + s->byte_offset_bits + 3) & ~7);
        s->last_bitoffset = pos & 7;
        pos >>= 3;
        len = buf_size - pos;
        if (len > MAX_CODED_SUPERFRAME_SIZE || len < 0)
        {
            DEBUGF("superframe size too large error after decoding\n");
            goto fail;
        }
        s->last_superframe_len = len;
        memcpy(s->last_superframe, buf + pos, len);
    }

    return s->frame_len;

fail:
    /* when error, we reset the bit reservoir */

    s->last_superframe_len = 0;
    return -1;
}

