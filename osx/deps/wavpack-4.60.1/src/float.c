////////////////////////////////////////////////////////////////////////////
//                           **** WAVPACK ****                            //
//                  Hybrid Lossless Wavefile Compressor                   //
//              Copyright (c) 1998 - 2006 Conifer Software.               //
//                          All Rights Reserved.                          //
//      Distributed under the BSD Software License (see license.txt)      //
////////////////////////////////////////////////////////////////////////////

// float.c

#include "wavpack_local.h"

#include <stdlib.h>

#ifdef DEBUG_ALLOC
#define malloc malloc_db
#define realloc realloc_db
#define free free_db
void *malloc_db (uint32_t size);
void *realloc_db (void *ptr, uint32_t size);
void free_db (void *ptr);
int32_t dump_alloc (void);
#endif

#ifndef NO_PACK

void write_float_info (WavpackStream *wps, WavpackMetadata *wpmd)
{
    char *byteptr;

    byteptr = wpmd->data = malloc (4);
    wpmd->id = ID_FLOAT_INFO;
    *byteptr++ = wps->float_flags;
    *byteptr++ = wps->float_shift;
    *byteptr++ = wps->float_max_exp;
    *byteptr++ = wps->float_norm_exp;
    wpmd->byte_length = (int32_t)(byteptr - (char *) wpmd->data);
}

int scan_float_data (WavpackStream *wps, f32 *values, int32_t num_values)
{
    int32_t shifted_ones = 0, shifted_zeros = 0, shifted_both = 0;
    int32_t false_zeros = 0, neg_zeros = 0;
    uint32_t ordata = 0, crc = 0xffffffff;
    int32_t count, value, shift_count;
    int max_exp = 0;
    f32 *dp;

    wps->float_shift = wps->float_flags = 0;

    for (dp = values, count = num_values; count--; dp++) {
        crc = crc * 27 + get_mantissa (*dp) * 9 + get_exponent (*dp) * 3 + get_sign (*dp);

        if (get_exponent (*dp) > max_exp && get_exponent (*dp) < 255)
            max_exp = get_exponent (*dp);
    }

    wps->crc_x = crc;

    for (dp = values, count = num_values; count--; dp++) {
        if (get_exponent (*dp) == 255) {
            wps->float_flags |= FLOAT_EXCEPTIONS;
            value = 0x1000000;
            shift_count = 0;
        }
        else if (get_exponent (*dp)) {
            shift_count = max_exp - get_exponent (*dp);
            value = 0x800000 + get_mantissa (*dp);
        }
        else {
            shift_count = max_exp ? max_exp - 1 : 0;
            value = get_mantissa (*dp);

//          if (get_mantissa (*dp))
//              denormals++;
        }

        if (shift_count < 25)
            value >>= shift_count;
        else
            value = 0;

        if (!value) {
            if (get_exponent (*dp) || get_mantissa (*dp))
                ++false_zeros;
            else if (get_sign (*dp))
                ++neg_zeros;
        }
        else if (shift_count) {
            int32_t mask = (1 << shift_count) - 1;

            if (!(get_mantissa (*dp) & mask))
                shifted_zeros++;
            else if ((get_mantissa (*dp) & mask) == mask)
                shifted_ones++;
            else
                shifted_both++;
        }

        ordata |= value;
        * (int32_t *) dp = (get_sign (*dp)) ? -value : value;
    }

    wps->float_max_exp = max_exp;

    if (shifted_both)
        wps->float_flags |= FLOAT_SHIFT_SENT;
    else if (shifted_ones && !shifted_zeros)
        wps->float_flags |= FLOAT_SHIFT_ONES;
    else if (shifted_ones && shifted_zeros)
        wps->float_flags |= FLOAT_SHIFT_SAME;
    else if (ordata && !(ordata & 1)) {
        while (!(ordata & 1)) {
            wps->float_shift++;
            ordata >>= 1;
        }

        for (dp = values, count = num_values; count--; dp++)
            * (int32_t *) dp >>= wps->float_shift;
    }

    wps->wphdr.flags &= ~MAG_MASK;

    while (ordata) {
        wps->wphdr.flags += 1 << MAG_LSB;
        ordata >>= 1;
    }

    if (false_zeros || neg_zeros)
        wps->float_flags |= FLOAT_ZEROS_SENT;

    if (neg_zeros)
        wps->float_flags |= FLOAT_NEG_ZEROS;

//  error_line ("samples = %d, max exp = %d, pre-shift = %d, denormals = %d",
//      num_values, max_exp, wps->float_shift, denormals);
//  if (wps->float_flags & FLOAT_EXCEPTIONS)
//      error_line ("exceptions!");
//  error_line ("shifted ones/zeros/both = %d/%d/%d, true/neg/false zeros = %d/%d/%d",
//      shifted_ones, shifted_zeros, shifted_both, true_zeros, neg_zeros, false_zeros);

    return wps->float_flags & (FLOAT_EXCEPTIONS | FLOAT_ZEROS_SENT | FLOAT_SHIFT_SENT | FLOAT_SHIFT_SAME);
}

void send_float_data (WavpackStream *wps, f32 *values, int32_t num_values)
{
    int max_exp = wps->float_max_exp;
    int32_t count, value, shift_count;
    f32 *dp;

    for (dp = values, count = num_values; count--; dp++) {
        if (get_exponent (*dp) == 255) {
            if (get_mantissa (*dp)) {
                putbit_1 (&wps->wvxbits);
                putbits (get_mantissa (*dp), 23, &wps->wvxbits);
            }
            else {
                putbit_0 (&wps->wvxbits);
            }

            value = 0x1000000;
            shift_count = 0;
        }
        else if (get_exponent (*dp)) {
            shift_count = max_exp - get_exponent (*dp);
            value = 0x800000 + get_mantissa (*dp);
        }
        else {
            shift_count = max_exp ? max_exp - 1 : 0;
            value = get_mantissa (*dp);
        }

        if (shift_count < 25)
            value >>= shift_count;
        else
            value = 0;

        if (!value) {
            if (wps->float_flags & FLOAT_ZEROS_SENT) {
                if (get_exponent (*dp) || get_mantissa (*dp)) {
                    putbit_1 (&wps->wvxbits);
                    putbits (get_mantissa (*dp), 23, &wps->wvxbits);

                    if (max_exp >= 25) {
                        putbits (get_exponent (*dp), 8, &wps->wvxbits);
                    }

                    putbit (get_sign (*dp), &wps->wvxbits);
                }
                else {
                    putbit_0 (&wps->wvxbits);

                    if (wps->float_flags & FLOAT_NEG_ZEROS)
                        putbit (get_sign (*dp), &wps->wvxbits);
                }
            }
        }
        else if (shift_count) {
            if (wps->float_flags & FLOAT_SHIFT_SENT) {
                int32_t data = get_mantissa (*dp) & ((1 << shift_count) - 1);
                putbits (data, shift_count, &wps->wvxbits);
            }
            else if (wps->float_flags & FLOAT_SHIFT_SAME) {
                putbit (get_mantissa (*dp) & 1, &wps->wvxbits);
            }
        }
    }
}

#endif

#if !defined(NO_UNPACK) || defined(INFO_ONLY)

int read_float_info (WavpackStream *wps, WavpackMetadata *wpmd)
{
    int bytecnt = wpmd->byte_length;
    char *byteptr = wpmd->data;

    if (bytecnt != 4)
        return FALSE;

    wps->float_flags = *byteptr++;
    wps->float_shift = *byteptr++;
    wps->float_max_exp = *byteptr++;
    wps->float_norm_exp = *byteptr;
    return TRUE;
}

#endif

#ifndef NO_UNPACK

static void float_values_nowvx (WavpackStream *wps, int32_t *values, int32_t num_values);

void float_values (WavpackStream *wps, int32_t *values, int32_t num_values)
{
    uint32_t crc = wps->crc_x;

    if (!bs_is_open (&wps->wvxbits)) {
        float_values_nowvx (wps, values, num_values);
        return;
    }

    while (num_values--) {
        int shift_count = 0, exp = wps->float_max_exp;
        f32 outval = 0;
        uint32_t temp;

        if (*values == 0) {
            if (wps->float_flags & FLOAT_ZEROS_SENT) {
                if (getbit (&wps->wvxbits)) {
                    getbits (&temp, 23, &wps->wvxbits);
                    set_mantissa (outval, temp);

                    if (exp >= 25) {
                        getbits (&temp, 8, &wps->wvxbits);
                        set_exponent (outval, temp);
                    }

                    set_sign (outval, getbit (&wps->wvxbits));
                }
                else if (wps->float_flags & FLOAT_NEG_ZEROS)
                    set_sign (outval, getbit (&wps->wvxbits));
            }
        }
        else {
            *values <<= wps->float_shift;

            if (*values < 0) {
                *values = -*values;
                set_sign (outval, 1);
            }

            if (*values == 0x1000000) {
                if (getbit (&wps->wvxbits)) {
                    getbits (&temp, 23, &wps->wvxbits);
                    set_mantissa (outval, temp);
                }

                set_exponent (outval, 255);
            }
            else {
                if (exp)
                    while (!(*values & 0x800000) && --exp) {
                        shift_count++;
                        *values <<= 1;
                    }

                if (shift_count) {
                    if ((wps->float_flags & FLOAT_SHIFT_ONES) ||
                        ((wps->float_flags & FLOAT_SHIFT_SAME) && getbit (&wps->wvxbits)))
                            *values |= ((1 << shift_count) - 1);
                    else if (wps->float_flags & FLOAT_SHIFT_SENT) {
                        getbits (&temp, shift_count, &wps->wvxbits);
                        *values |= temp & ((1 << shift_count) - 1);
                    }
                }

                set_mantissa (outval, *values);
                set_exponent (outval, exp);
            }
        }

        crc = crc * 27 + get_mantissa (outval) * 9 + get_exponent (outval) * 3 + get_sign (outval);
        * (f32 *) values++ = outval;
    }

    wps->crc_x = crc;
}

static void float_values_nowvx (WavpackStream *wps, int32_t *values, int32_t num_values)
{
    while (num_values--) {
        int shift_count = 0, exp = wps->float_max_exp;
        f32 outval = 0;

        if (*values) {
            *values <<= wps->float_shift;

            if (*values < 0) {
                *values = -*values;
                set_sign (outval, 1);
            }

            if (*values >= 0x1000000) {
                while (*values & 0xf000000) {
                    *values >>= 1;
                    ++exp;
                }
            }
            else if (exp) {
                while (!(*values & 0x800000) && --exp) {
                    shift_count++;
                    *values <<= 1;
                }

                if (shift_count && (wps->float_flags & FLOAT_SHIFT_ONES))
                    *values |= ((1 << shift_count) - 1);
            }

            set_mantissa (outval, *values);
            set_exponent (outval, exp);
        }

        * (f32 *) values++ = outval;
    }
}

#endif

void WavpackFloatNormalize (int32_t *values, int32_t num_values, int delta_exp)
{
    f32 *fvalues = (f32 *) values;
    int exp;

    if (!delta_exp)
        return;

    while (num_values--) {
        if ((exp = get_exponent (*fvalues)) == 0 || exp + delta_exp <= 0)
            *fvalues = 0;
        else if (exp == 255 || (exp += delta_exp) >= 255) {
            set_exponent (*fvalues, 255);
            set_mantissa (*fvalues, 0);
        }
        else
            set_exponent (*fvalues, exp);

        fvalues++;
    }
}
