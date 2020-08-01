/*
    Album Art plugin for DeaDBeeF
    Copyright (C) 2009-2011 Viktor Semykin <thesame.ml@gmail.com>
    Copyright (C) 2009-2013 Alexey Yakovenko <waker@users.sourceforge.net>

    This software is provided 'as-is', without any express or implied
    warranty.  In no event will the authors be held liable for any damages
    arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.

    3. This notice may not be removed or altered from any source distribution.
*/

#ifdef HAVE_CONFIG_H
    #include "../../config.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <libgen.h>
#include <dirent.h>
#include <unistd.h>
#include <fnmatch.h>
#include <pthread.h>
#include <errno.h>
#include <sys/stat.h>
#ifdef __linux__
    #include <sys/prctl.h>
#endif
#if HAVE_SYS_CDEFS_H
    #include <sys/cdefs.h>
#endif
#include <limits.h>
#ifdef USE_METAFLAC
    #include <FLAC/metadata.h>
#endif
#ifdef USE_IMLIB2
    #include <Imlib2.h>
#else
    #include <jpeglib.h>
    #include <png.h>
#endif
#include "../../deadbeef.h"
#include "artwork_internal.h"
#include "lastfm.h"
#include "musicbrainz.h"
#include "albumartorg.h"
#include "wos.h"
#include "cache.h"
#include "artwork.h"
#include "../../shared/mp4tagutil.h"

#define trace(...) { fprintf (stderr, __VA_ARGS__); }
//#define trace(...)

DB_functions_t *deadbeef;
static DB_artwork_plugin_t plugin;

#define NOARTWORK_IMAGE "noartwork.png"
static char *default_cover;

typedef struct cover_callback_s {
    artwork_callback cb;
    void *ud;
    struct cover_callback_s *next;
} cover_callback_t;

typedef struct cover_query_s {
    char *fname;
    char *artist;
    char *album;
    int size;
    cover_callback_t *callback;
    struct cover_query_s *next;
} cover_query_t;

static cover_query_t *queue;
static cover_query_t *queue_tail;
static int terminate;
static intptr_t tid;
static uintptr_t queue_mutex;
static uintptr_t queue_cond;

static int artwork_enable_embedded;
static int artwork_enable_local;
#ifdef USE_VFS_CURL
    static int artwork_enable_lfm;
    static int artwork_enable_mb;
    static int artwork_enable_aao;
    static int artwork_enable_wos;
#endif
static int scale_towards_longer;
static int missing_artwork;
static char *nocover_path;

static time_t cache_reset_time;
static time_t scaled_cache_reset_time;
static time_t default_reset_time;

#define DEFAULT_FILEMASK "*cover*.jpg;*front*.jpg;*folder*.jpg;*cover*.png;*front*.png;*folder*.png"
static char *artwork_filemask;

static float
scale_dimensions (int scaled_size, int width, int height, unsigned int *scaled_width, unsigned int *scaled_height)
{
    /* Calculate the dimensions of the scaled image */
    float scaling_ratio;
    if (scale_towards_longer == width > height) {
        *scaled_height = scaled_size;
        scaling_ratio = (float)height / *scaled_height;
        *scaled_width = width / scaling_ratio + 0.5;
    }
    else {
        *scaled_width = scaled_size;
        scaling_ratio = (float)width / *scaled_width;
        *scaled_height = height / scaling_ratio + 0.5;
    }
    return scaling_ratio;
}

#ifndef USE_IMLIB2
#ifdef USE_BICUBIC
static float cerp (float p0, float p1, float p2, float p3, float d, float d2, float d3)
{
    /* Cubic Hermite spline (a = -0.5, Catmull-Rom) */
    return p1 - (p0*d3 - 3*p1*d3 + 3*p2*d3 - p3*d3 - 2*p0*d2 + 5*p1*d2 - 4*p2*d2 + p3*d2 + p0*d - p2*d)/2;
}

static int_fast16_t bcerp (const png_byte *row0, const png_byte *row1, const png_byte *row2, const png_byte *row3,
                      const uint_fast32_t x0, const uint_fast32_t x1, const uint_fast32_t x2, const uint_fast32_t x3, const uint_fast8_t component,
                      float dx, float dx2, float dx3, float dy, float dy2, float dy3) {
    const uint_fast32_t index0 = x0 + component;
    const uint_fast32_t index1 = x1 + component;
    const uint_fast32_t index2 = x2 + component;
    const uint_fast32_t index3 = x3 + component;
    float p0 = cerp (row0[index0], row1[index0], row2[index0], row3[index0], dy, dy2, dy3);
    float p1 = cerp (row0[index1], row1[index1], row2[index1], row3[index1], dy, dy2, dy3);
    float p2 = cerp (row0[index2], row1[index2], row2[index2], row3[index2], dy, dy2, dy3);
    float p3 = cerp (row0[index3], row1[index3], row2[index3], row3[index3], dy, dy2, dy3);
    return cerp (p0, p1, p2, p3, dx, dx2, dx3) + 0.5;
}
#endif
static uint_fast32_t blerp_pixel (const png_byte *row, const png_byte *next_row, const uint_fast32_t x_index, const uint_fast32_t next_x_index,
                                 const uint_fast32_t weight, const uint_fast32_t weightx, const uint_fast32_t weighty, const uint_fast32_t weightxy)
{
    return row[x_index]*weight + row[next_x_index]*weightx + next_row[x_index]*weighty + next_row[next_x_index]*weightxy;
}

static uint_fast32_t *
calculate_quick_dividers (float scaling_ratio)
{
    /* Replace slow divisions by expected numbers with faster multiply/shift combo */
    const uint16_t max_sample_size = scaling_ratio + 1;
    uint_fast32_t *dividers = malloc ((max_sample_size * max_sample_size + 1) * sizeof (uint_fast32_t));
    if (dividers) {
        for (uint16_t y = max_sample_size; y; y--) {
            for (uint16_t x = y; x; x--) {
                const uint32_t num_pixels = x * y;
                /* Accurate to 1/256 for up to 256 pixels and always for powers of 2 */
                dividers[num_pixels] = num_pixels <= 256 || !(num_pixels & (num_pixels - 1)) ? 256*256/num_pixels : 0;
            }
        }
    }
    return dividers;
}

typedef struct {
    struct jpeg_error_mgr pub;	/* "public" fields */
    jmp_buf setjmp_buffer;	/* for return to caller */
} my_error_mgr_t;

METHODDEF (void)
my_error_exit (j_common_ptr cinfo)
{
  /* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
  my_error_mgr_t *myerr = (my_error_mgr_t *) cinfo->err;

//  (*cinfo->err->output_message)(cinfo);

  /* Return control to the setjmp point */
  longjmp (myerr->setjmp_buffer, 1);
}


static int
jpeg_resize (const char *fname, const char *outname, int scaled_size) {
    trace ("resizing %s into %s\n", fname, outname);
    FILE *fp = NULL, *out = NULL;
    struct jpeg_decompress_struct cinfo;
    struct jpeg_compress_struct cinfo_out;
    void *sample_rows_buffer = NULL;
    uint_fast32_t *quick_dividers = NULL;
    my_error_mgr_t jerr;

    cinfo.mem = cinfo_out.mem = NULL;
    cinfo_out.err = cinfo.err = jpeg_std_error (&jerr.pub);
    jerr.pub.error_exit = my_error_exit;
    if (setjmp (jerr.setjmp_buffer)) {
        trace ("failed to scale %s as jpeg\n", outname);
        jpeg_destroy_decompress (&cinfo);
        jpeg_destroy_compress (&cinfo_out);
        if (sample_rows_buffer) {
            free (sample_rows_buffer);
        }
        if (quick_dividers) {
            free (quick_dividers);
        }
        if (fp) {
            fclose (fp);
        }
        if (out) {
            fclose (out);
        }
        return -1;
    }

    fp = fopen (fname, "rb");
    if (!fp) {
        return -1;
    }
    out = fopen (outname, "w+b");
    if (!out) {
        fclose (fp);
        return -1;
    }

    jpeg_create_decompress (&cinfo);
    jpeg_create_compress (&cinfo_out);

    jpeg_stdio_src (&cinfo, fp);
    jpeg_stdio_dest (&cinfo_out, out);

    jpeg_read_header (&cinfo, TRUE);
    jpeg_start_decompress (&cinfo);

    const unsigned int num_components = cinfo.output_components;
    const unsigned int width = cinfo.image_width;
    const unsigned int height = cinfo.image_height;
    unsigned int scaled_width, scaled_height;
    float scaling_ratio = scale_dimensions (scaled_size, width, height, &scaled_width, &scaled_height);
    if (scaling_ratio >= 65535 || scaled_width < 1 || scaled_width > 32767 || scaled_height < 1 || scaled_width > 32767) {
        trace ("scaling ratio (%g) or scaled image dimensions (%ux%u) are invalid\n", scaling_ratio, scaled_width, scaled_height);
        my_error_exit ((j_common_ptr)&cinfo);
    }

    cinfo_out.image_width      = scaled_width;
    cinfo_out.image_height     = scaled_height;
    cinfo_out.input_components = num_components;
    cinfo_out.in_color_space   = cinfo.out_color_space;
    jpeg_set_defaults (&cinfo_out);
    jpeg_set_quality (&cinfo_out, 95, TRUE);
    jpeg_start_compress (&cinfo_out, TRUE);

    const uint_fast32_t row_components = width * num_components;
    const uint_fast32_t scaled_row_components = scaled_width * num_components;
    JSAMPLE out_line[scaled_row_components];
    JSAMPROW out_row = out_line;

    if (scaling_ratio > 2) {
        /* Simple (unweighted) area sampling for large downscales */
        quick_dividers = calculate_quick_dividers (scaling_ratio);
        const uint16_t max_sample_height = scaling_ratio + 1;
        sample_rows_buffer = malloc (max_sample_height * row_components * sizeof (JSAMPLE));
        if (!sample_rows_buffer || !quick_dividers) {
            my_error_exit ((j_common_ptr)&cinfo);
        }
        JSAMPROW rows[max_sample_height];
        for (uint16_t row_index = 0; row_index < max_sample_height; row_index++) {
            rows[row_index] = sample_rows_buffer + row_index*row_components;
        }

        /* Loop through all (down-)scaled pixels */
        float y_interp = 0.5;
        for (uint_fast16_t scaled_y = 0; scaled_y < scaled_height; scaled_y++) {
            const uint_fast32_t y = y_interp;
            const uint_fast32_t y_limit = y_interp += scaling_ratio;
            const uint_fast16_t num_y_pixels = (y_limit < height ? y_limit : height) - y;

            for (uint_fast16_t row_index = 0; row_index < num_y_pixels; row_index++) {
                jpeg_read_scanlines (&cinfo, &rows[row_index], 1);
            }

            float x_interp = 0.5;
            for (uint_fast32_t scaled_x = 0; scaled_x < scaled_row_components; scaled_x+=num_components) {
                const uint_fast32_t x = x_interp;
                x_interp += scaling_ratio;
                const uint_fast32_t x_limit = x_interp < width ? x_interp : width;
                const uint_fast32_t x_index = x * num_components;
                const uint_fast32_t x_limit_index = x_limit * num_components;

                /* Sum all values where the scaled pixel overlaps at least half a pixel in each direction */
                const uint_fast32_t num_pixels = num_y_pixels * (x_limit - x);
                const uint_fast32_t quick_divider = quick_dividers[num_pixels];
                for (uint_fast8_t component=0; component<num_components; component++) {
                    uint_fast32_t value = 0;
                    for (uint_fast32_t pixel_index = x_index+component; pixel_index < x_limit_index; pixel_index+=num_components) {
                        for (uint_fast16_t row_index = 0; row_index < num_y_pixels; row_index++) {
                            value += rows[row_index][pixel_index];
                        }
                    }
                    out_row[scaled_x+component] = quick_divider ? value * quick_divider >> 16 : value / num_pixels;
                }
            }

            jpeg_write_scanlines (&cinfo_out, &out_row, 1);
        }

        free (sample_rows_buffer);
        free (quick_dividers);
    }
    else {
#ifndef USE_BICUBIC
        /* Bilinear interpolation for upscales and modest downscales */
        JSAMPLE scanline[row_components];
        JSAMPLE next_scanline[row_components];
        JSAMPROW row = scanline;
        JSAMPROW next_row = next_scanline;
        float downscale_offset = scaling_ratio < 1 ? 0 : (scaling_ratio - 1) / 2;
        float y_interp = downscale_offset;
        for (uint_fast16_t scaled_y = 0; scaled_y < scaled_height; scaled_y++, y_interp+=scaling_ratio) {
            const uint_fast32_t y = y_interp;
            const uint_fast16_t y_diff = (uint_fast32_t)(y_interp*256) - (y<<8);
            const uint_fast16_t y_remn = 256 - y_diff;

            if (cinfo.output_scanline < y+2) {
                while (cinfo.output_scanline < y+1) {
                    jpeg_read_scanlines (&cinfo, &next_row, 1);
                }
                memcpy (row, next_row, row_components*sizeof (JSAMPLE));
                if (y+2 < height) {
                    jpeg_read_scanlines (&cinfo, &next_row, 1);
                }
            }

            float x_interp = downscale_offset;
            for (uint_fast32_t scaled_x = 0; scaled_x < scaled_row_components; scaled_x+=num_components, x_interp+=scaling_ratio) {
                const uint_fast32_t x = x_interp;
                const uint_fast32_t x_index = x * num_components;
                const uint_fast32_t next_x_index = x+1 < width ? x_index+num_components : x_index;

                const uint_fast16_t x_diff = (uint_fast32_t)(x_interp*256) - (x<<8);
                const uint_fast16_t x_remn = 256 - x_diff;
                const uint_fast32_t weight = x_remn * y_remn;
                const uint_fast32_t weightx = x_diff * y_remn;
                const uint_fast32_t weighty = x_remn * y_diff;
                const uint_fast32_t weightxy = x_diff * y_diff;

                for (uint_fast8_t component=0; component<num_components; component++) {
                    out_line[scaled_x + component] = blerp_pixel (row, next_row, x_index+component, next_x_index+component, weight, weightx, weighty, weightxy) >> 16;
                }
            }

            jpeg_write_scanlines (&cinfo_out, &out_row, 1);
        }
#else
        /* Bicubic interpolation to improve the scaled image quality */
        JSAMPLE scanline0[row_components];
        JSAMPLE scanline1[row_components];
        JSAMPLE scanline2[row_components];
        JSAMPLE scanline3[row_components];
        JSAMPROW row0 = scanline0;
        JSAMPROW row1 = scanline1;
        JSAMPROW row2 = scanline2;
        JSAMPROW row3 = scanline3;
        jpeg_read_scanlines (&cinfo, &row1, 1);
        memcpy (row2, row1, row_components*sizeof (JSAMPLE));

        float downscale_offset = scaling_ratio < 1 ? 0 : (scaling_ratio - 1) / 2;
        float y_interp = downscale_offset;
        for (uint_fast16_t scaled_y = 0; scaled_y < scaled_height; scaled_y++, y_interp+=scaling_ratio) {
            const uint_fast32_t y = y_interp;
            float dy = y_interp - (int_fast32_t)y_interp;
            float dy2 = dy * dy;
            float dy3 = dy2 * dy;

            if (cinfo.output_scanline < y+3) {
                while (cinfo.output_scanline < y) {
                    jpeg_read_scanlines (&cinfo, &row1, 1);
                }
                memcpy (row0, row1, row_components*sizeof (JSAMPLE));
                if (cinfo.output_scanline < y+1) {
                    jpeg_read_scanlines (&cinfo, &row2, 1);
                }
                memcpy (row1, row2, row_components*sizeof (JSAMPLE));
                if (y+2 < height && cinfo.output_scanline < y+2) {
                    jpeg_read_scanlines (&cinfo, &row3, 1);
                }
                memcpy (row2, row3, row_components*sizeof (JSAMPLE));
                if (y+3 < height) {
                    jpeg_read_scanlines (&cinfo, &row3, 1);
                }
            }

            float x_interp = downscale_offset;
            for (uint_fast32_t scaled_x = 0; scaled_x < scaled_row_components; scaled_x+=num_components, x_interp+=scaling_ratio) {
                const uint_fast32_t x = x_interp;
                const uint_fast32_t x1 = x * num_components;
                const uint_fast32_t x0 = x > 0 ? x1-num_components : x1;
                const uint_fast32_t x2 = x+1 < width ? x1+num_components : x1;
                const uint_fast32_t x3 = x+2 < width ? x2+num_components : x2;

                float dx = x_interp - (int_fast32_t)x_interp;
                float dx2 = dx * dx;
                float dx3 = dx2 * dx;

                for (uint_fast8_t component=0; component<num_components; component++) {
                    const int_fast16_t pixel = bcerp (row0, row1, row2, row3, x0, x1, x2, x3, component, dx, dx2, dx3, dy, dy2, dy3);
                    out_row[scaled_x + component] = pixel < 0 ? 0 : pixel > 255 ? 255 : pixel;
                }
            }

            jpeg_write_scanlines (&cinfo_out, &out_row, 1);
        }
#endif
    }

    jpeg_finish_compress (&cinfo_out);

    jpeg_destroy_compress (&cinfo_out);
    jpeg_destroy_decompress (&cinfo);

    fclose (fp);
    fclose (out);

    return 0;
}

static int
png_resize (const char *fname, const char *outname, int scaled_size) {
    png_structp png_ptr = NULL, new_png_ptr = NULL;
    png_infop info_ptr = NULL, new_info_ptr = NULL;
    png_uint_32 height, width;
    int bit_depth, color_type;
    int err = -1;
    FILE *fp = NULL;
    FILE *out = NULL;
    png_byte *out_row = NULL;
    uint_fast32_t *quick_dividers = NULL;

    fp = fopen (fname, "rb");
    if (!fp) {
        trace ("failed to open %s for reading\n", fname);
        goto error;
    }

    png_ptr = png_create_read_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) {
        trace ("failed to create PNG read struct\n");
        goto error;
    }

    if (setjmp (png_jmpbuf ((png_ptr))))
    {
        trace ("failed to read %s as png\n", fname);
        goto error;
    }

    png_init_io (png_ptr, fp);

    info_ptr = png_create_info_struct (png_ptr);
    if (!info_ptr) {
        trace ("failed to create PNG info struct\n");
        goto error;
    }

    png_read_png (png_ptr, info_ptr, PNG_TRANSFORM_STRIP_16|PNG_TRANSFORM_PACKING|PNG_TRANSFORM_EXPAND, NULL);
    png_bytep *row_pointers = png_get_rows (png_ptr, info_ptr);
    png_get_IHDR (png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, NULL, NULL, NULL);

    unsigned int scaled_width, scaled_height;
    float scaling_ratio = scale_dimensions (scaled_size, width, height, &scaled_width, &scaled_height);
    if (scaling_ratio >= 65535 || scaled_width < 1 || scaled_width > 32767 || scaled_height < 1 || scaled_width > 32767) {
        trace ("scaling ratio (%g) or scaled image dimensions (%ux%u) are invalid\n", scaling_ratio, scaled_width, scaled_height);
        goto error;
    }

    out = fopen (outname, "w+b");
    if (!out) {
        trace ("failed to open %s for writing\n", outname);
        goto error;
    }

    new_png_ptr = png_create_write_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!new_png_ptr) {
        trace ("failed to create png write struct\n");
        goto error;
    }

    if (setjmp (png_jmpbuf ((new_png_ptr))))
    {
        trace ("failed to write %s as png\n", outname);
        goto error;
    }

    png_init_io (new_png_ptr, out);

    new_info_ptr = png_create_info_struct (new_png_ptr);
    if (!new_info_ptr) {
        trace ("failed to create png info struct for writing\n");
        goto error;
    }

    png_set_IHDR (new_png_ptr, new_info_ptr, scaled_width, scaled_height, bit_depth, color_type, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_write_info (new_png_ptr, new_info_ptr);
    png_set_packing (new_png_ptr);

    const uint8_t has_alpha = color_type & PNG_COLOR_MASK_ALPHA;
    const uint8_t num_values = color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA ? 1 : 3;
    const uint8_t num_components = num_values + (has_alpha ? 1 : 0);
    const uint_fast32_t scaled_row_components = scaled_width * num_components;
    out_row = malloc (scaled_row_components * sizeof (png_byte));
    if (!out_row) {
        goto error;
    }

    if (scaling_ratio > 2) {
        /* Simple (unweighted) area sampling for large downscales */
        quick_dividers = calculate_quick_dividers (scaling_ratio);
        if (!quick_dividers) {
            goto error;
        }

        /* Loop through all (down-)scaled pixels */
        float y_interp = 0.5;
        for (uint_fast16_t scaled_y = 0; scaled_y < scaled_height; scaled_y++) {
            const uint_fast32_t y = y_interp;
            const uint_fast32_t y_limit = y_interp += scaling_ratio;
            const uint_fast16_t num_y_pixels = (y_limit < height ? y_limit : height) - y;
            png_byte *rows[num_y_pixels];
            for (uint_fast16_t row_index = 0; row_index < num_y_pixels; row_index++) {
                rows[row_index] = row_pointers[y+row_index];
            }

            float x_interp = 0.5;
            for (uint_fast32_t scaled_x = 0; scaled_x < scaled_row_components; scaled_x+=num_components) {
                const uint_fast32_t x = x_interp;
                x_interp += scaling_ratio;
                const uint_fast32_t x_limit = x_interp < width ? x_interp : width;
                const uint_fast32_t x_index = x * num_components;
                const uint_fast32_t x_limit_index = x_limit * num_components;
                const uint_fast32_t num_pixels = num_y_pixels * (x_limit - x);

                /* Sum all values where the scaled pixel overlaps at least half an original pixel in each direction */
                const uint_fast32_t quick_divider = quick_dividers[num_pixels];
                if (has_alpha) {
                    /* Alpha weight possible transparent pixels */
                    uint_fast32_t greyred_value = 0;
                    uint_fast32_t green_value = 0;
                    uint_fast32_t blue_value = 0;
                    uint_fast32_t alpha_value = 0;
                    for (uint_fast16_t row_index = 0; row_index < num_y_pixels; row_index++) {
                        const png_byte *start = rows[row_index] + x_index;
                        png_byte *ptr = rows[row_index] + x_limit_index;
                        do {
                            const png_byte alpha = *--ptr;
                            alpha_value += alpha;
                            if (num_values == 3) {
                                blue_value += *--ptr * alpha;
                                green_value += *--ptr * alpha;
                            }
                            greyred_value += *--ptr * alpha;
                        } while (ptr > start);
                    }
                    if (alpha_value < num_pixels) {
                        for (uint_fast8_t component = 0; component < num_components; component++) {
                            out_row[scaled_x+component] = 0;
                        }
                    }
                    else {
                        out_row[scaled_x+num_values] = alpha_value > 254*num_pixels ? 255 : quick_divider ? alpha_value*quick_divider>>16 : alpha_value/num_pixels;
                        if (quick_divider && out_row[scaled_x+num_values] == 255) {
                            out_row[scaled_x] = greyred_value * quick_divider >> 24;
                            if (num_values == 3) {
                                out_row[scaled_x+1] = green_value * quick_divider >> 24;
                                out_row[scaled_x+2] = blue_value * quick_divider >> 24;
                            }
                        }
                        else {
                            out_row[scaled_x] = greyred_value / alpha_value;
                            if (num_values == 3) {
                                out_row[scaled_x+1] = green_value / alpha_value;
                                out_row[scaled_x+2] = blue_value / alpha_value;
                            }
                        }
                    }
                }
                else if (num_values == 3) {
                    /* For opaque RGB pixels, use a simple average on each colour component */
                    uint_fast32_t red_value = 0;
                    uint_fast32_t green_value = 0;
                    uint_fast32_t blue_value = 0;
                    for (uint_fast16_t row_index = 0; row_index < num_y_pixels; row_index++) {
                        const png_byte *start = rows[row_index] + x_index;
                        png_byte *ptr = rows[row_index] + x_limit_index;
                        do {
                            blue_value += *--ptr;
                            green_value += *--ptr;
                            red_value += *--ptr;
                        } while (ptr > start);

                    }
                    out_row[scaled_x] = quick_divider ? red_value * quick_divider >> 16 : red_value / num_pixels;
                    out_row[scaled_x+1] = quick_divider ? green_value * quick_divider >> 16 : green_value / num_pixels;
                    out_row[scaled_x+2] = quick_divider ? blue_value * quick_divider >> 16 : blue_value / num_pixels;
                }
                else {
                    /* For opaque grey pixels, use a simple average of the values */
                    uint_fast32_t grey_value = 0;
                    for (uint_fast16_t row_index = 0; row_index < num_y_pixels; row_index++) {
                        for (uint_fast32_t pixel_index = x_index; pixel_index < x_limit_index; pixel_index++) {
                            grey_value += rows[row_index][pixel_index];
                        }
                    }
                    out_row[scaled_x] = quick_divider ? grey_value * quick_divider >> 16 : grey_value / num_pixels;
                }
            }

            png_write_row (new_png_ptr, out_row);
        }
    }
    else {
#ifndef USE_BICUBIC
        /* Bilinear interpolation for upscales and modest downscales */
        float downscale_offset = scaling_ratio < 1 ? 0 : (scaling_ratio - 1) / 2;
        uint_fast32_t scaled_alpha = 255 << 16;
        float y_interp = downscale_offset;
        for (uint_fast16_t scaled_y = 0; scaled_y < scaled_height; scaled_y++, y_interp+=scaling_ratio) {
            const uint_fast32_t y = y_interp;
            const png_byte *row = row_pointers[y];
            const png_byte *next_row = y+1 < height ? row_pointers[y+1] : row;

            const uint_fast16_t y_diff = (uint_fast32_t)(y_interp*256) - (y<<8);
            const uint_fast16_t y_remn = 256 - y_diff;

            float x_interp = downscale_offset;
            for (uint_fast32_t scaled_x = 0; scaled_x < scaled_row_components; scaled_x+=num_components, x_interp+=scaling_ratio) {
                const uint_fast32_t x = x_interp;
                const uint_fast32_t x_index = x * num_components;
                const uint_fast32_t next_x_index = x < width ? x_index+num_components : x_index;

                const uint_fast16_t x_diff = (uint_fast32_t)(x_interp*256) - (x<<8);
                const uint_fast16_t x_remn = 256 - x_diff;
                const uint_fast32_t weight = x_remn * y_remn;
                const uint_fast32_t weightx = x_diff * y_remn;
                const uint_fast32_t weighty = x_remn * y_diff;
                const uint_fast32_t weightxy = x_diff * y_diff;

                uint_fast32_t alpha, alphax, alphay, alphaxy;
                if (has_alpha) {
                    /* Interpolate alpha channel and weight pixels by their alpha */
                    alpha = weight * row[x_index + num_values];
                    alphax = weightx * row[next_x_index + num_values];
                    alphay = weighty * next_row[x_index + num_values];
                    alphaxy = weightxy * next_row[next_x_index + num_values];
                    scaled_alpha = alpha + alphax + alphay + alphaxy;
                    out_row[scaled_x + num_values] = scaled_alpha >> 16;
                }

                if (scaled_alpha == 255 << 16) {
                    /* Simplified calculation for fully opaque pixels */
                    for (uint_fast8_t component=0; component<num_values; component++) {
                        out_row[scaled_x + component] = blerp_pixel (row, next_row, x_index+component, next_x_index+component, weight, weightx, weighty, weightxy) >> 16;
                    }
                }
                else if (scaled_alpha == 0) {
                    /* For speed, don't preserve the values of fully transparent pixels */
                    for (uint_fast8_t component=0; component<num_values; component++) {
                        out_row[scaled_x + component] = 0;
                    }
                }
                else {
                    /* Alpha-weight partially transparent pixels to avoid background colour bleeding */
                    for (uint_fast8_t component=0; component<num_values; component++) {
                        out_row[scaled_x + component] = blerp_pixel (row, next_row, x_index+component, next_x_index+component, alpha, alphax, alphay, alphaxy) / scaled_alpha;
                    }
                }
            }

            png_write_row (new_png_ptr, out_row);
        }
#else
        /* Bicubic interpolation to improve the scaled image quality */
        if (has_alpha) {
            for (uint_fast16_t y = 0; y < height; y++) {
                png_byte *row = row_pointers[y];
                for (uint_fast16_t x_index = 0; x_index < width*4; x_index+=4) {
                    const png_byte alpha = row[x_index + 3];
                    if (alpha < 255) {
                        for (uint_fast8_t component=0; component<3; component++) {
                            row[x_index + component] = row[x_index + component]*alpha >> 8;
                        }
                    }
                }
            }
        }

        float downscale_offset = scaling_ratio < 1 ? 0 : (scaling_ratio - 1) / 2;
        int_fast16_t scaled_alpha = 255;
        float y_interp = downscale_offset;
        for (uint_fast16_t scaled_y = 0; scaled_y < scaled_height; scaled_y++, y_interp+=scaling_ratio) {
            const uint_fast32_t y = y_interp;
            const png_byte *row1 = row_pointers[y];
            const png_byte *row0 = y > 0 ? row_pointers[y-1] : row1;
            const png_byte *row2 = y+1 < height ? row_pointers[y+1] : row1;
            const png_byte *row3 = y+2 < height ? row_pointers[y+2] : row2;

            float dy = y_interp - (int_fast32_t)y_interp;
            float dy2 = dy * dy;
            float dy3 = dy2 * dy;

            float x_interp = downscale_offset;
            for (uint_fast32_t scaled_x = 0; scaled_x < scaled_row_components; scaled_x+=num_components, x_interp+=scaling_ratio) {
                const uint_fast32_t x = x_interp;
                const uint_fast32_t x1 = x * num_components;
                const uint_fast32_t x0 = x > 0 ? x1-num_components : x1;
                const uint_fast32_t x2 = x+1 < width ? x1+num_components : x1;
                const uint_fast32_t x3 = x+2 < width ? x2+num_components : x2;

                float dx = x_interp - (int_fast32_t)x_interp;
                float dx2 = dx * dx;
                float dx3 = dx2 * dx;

                if (has_alpha) {
                    scaled_alpha = bcerp (row0, row1, row2, row3, x0, x1, x2, x3, 3, dx, dx2, dx3, dy, dy2, dy3);
                    out_row[scaled_x + 3] = scaled_alpha < 0 ? 0 : scaled_alpha > 255 ? 255 : scaled_alpha;
                }

                if (scaled_alpha == 255) {
                    for (uint_fast8_t component=0; component<3; component++) {
                        const int_fast16_t pixel = bcerp (row0, row1, row2, row3, x0, x1, x2, x3, component, dx, dx2, dx3, dy, dy2, dy3);
                        out_row[scaled_x + component] = pixel < 0 ? 0 : pixel > 255 ? 255 : pixel;
                    }
                }
                else if (scaled_alpha == 0) {
                    for (uint_fast8_t component=0; component<3; component++) {
                        out_row[scaled_x + component] = 0;
                    }
                }
                else {
                    for (uint_fast8_t component=0; component<3; component++) {
                        const int_fast16_t pixel = (bcerp (row0, row1, row2, row3, x0, x1, x2, x3, component, dx, dx2, dx3, dy, dy2, dy3)<<8) / scaled_alpha;
                        out_row[scaled_x + component] = pixel < 0 ? 0 : pixel > 255 ? 255 : pixel;
                    }
                }
            }

            png_write_row (new_png_ptr, out_row);
        }
#endif
    }

    png_write_end (new_png_ptr, new_info_ptr);

    err = 0;
error:
    if (out) {
        fclose (out);
    }
    if (fp) {
        fclose (fp);
    }
    if (png_ptr) {
        png_destroy_read_struct (&png_ptr, &info_ptr, NULL);
    }
    if (new_png_ptr) {
        png_destroy_write_struct (&new_png_ptr, &new_info_ptr);
    }
    if (out_row) {
        free (out_row);
    }
    if (quick_dividers) {
        free (quick_dividers);
    }

    return err;
}

#endif

#ifdef USE_IMLIB2
static int
imlib_resize (const char *in, const char *out, int img_size)
{
    Imlib_Image img = imlib_load_image_immediately (in);
    if (!img) {
        trace ("file %s not found, or imlib2 can't load it\n", in);
        return -1;
    }
    imlib_context_set_image (img);

    int w = imlib_image_get_width ();
    int h = imlib_image_get_height ();
    int sw, sh;
    scale_dimensions (img_size, w, h, &sw, &sh);
    if (sw < 1 || sw > 32767 || sh < 1 || sh > 32767) {
        trace ("%d/%d scaled image is too large\n", sw, sh);
        imlib_free_image ();
        return -1;
    }

    int is_jpeg = imlib_image_format () && imlib_image_format ()[0] == 'j';
    Imlib_Image scaled = imlib_create_cropped_scaled_image (0, 0, w, h, sw, sh);
    if (!scaled) {
        trace ("imlib2 can't create scaled image from %s\n", in);
        imlib_free_image ();
        return -1;
    }
    imlib_context_set_image (scaled);

    imlib_image_set_format (is_jpeg ? "jpg" : "png");
    if (is_jpeg)
        imlib_image_attach_data_value ("quality", NULL, 95, NULL);
    Imlib_Load_Error err = 0;
    imlib_save_image_with_error_return (out, &err);
    if (err != 0) {
        trace ("imlib save %s returned %d\n", out, err);
        imlib_free_image ();
        imlib_context_set_image (img);
        imlib_free_image ();
        return -1;
    }

    imlib_free_image ();
    imlib_context_set_image (img);
    imlib_free_image ();
    return 0;
}
#endif

static int
scale_file (const char *in, const char *out, int img_size)
{
    trace ("artwork: scaling %s to %s\n", in, out);

    if (img_size < 1 || img_size > 32767) {
        trace ("%d is not a valid scaled image size\n", img_size);
        return -1;
    }

    if (!ensure_dir (out)) {
        return -1;
    }

    cache_lock ();
#ifdef USE_IMLIB2
    int imlib_err = imlib_resize (in, out, img_size);
    cache_unlock ();
    return imlib_err;
#else
    int err = jpeg_resize (in, out, img_size);
    if (err != 0) {
        unlink (out);
        err = png_resize (in, out, img_size);
        if (err != 0) {
            unlink (out);
        }
    }
    cache_unlock ();
    return err;
#endif
}

// esc_char is needed to prevent using file path separators,
// e.g. to avoid writing arbitrary files using "../../../filename"
static char
esc_char (char c) {
#ifndef WIN32
    if (c == '/') {
        return '\\';
    }
#else
    if (c == '/' || c == ':' || c == '|' || c == '*' || c == '\\' || c == '\"' || c == '<' || c == '>' || c == '?') {
        return '_';
    }
#endif
    return c;
}

static int
make_cache_dir_path (char *path, int size, const char *artist, int img_size) {
    char esc_artist[NAME_MAX+1];
    if (artist) {
        size_t i = 0;
        while (artist[i] && i < NAME_MAX) {
            esc_artist[i] = esc_char (artist[i]);
            i++;
        }
        esc_artist[i] = '\0';
    }
    else {
        strcpy (esc_artist, "Unknown artist");
    }

    if (make_cache_root_path (path, size) < 0) {
        return -1;
    }

    const size_t size_left = size - strlen (path);
    int path_length;
    if (img_size == -1) {
        path_length = snprintf (path+strlen (path), size_left, "covers/%s/", esc_artist);
    }
    else {
        path_length = snprintf (path+strlen (path), size_left, "covers-%d/%s/", img_size, esc_artist);
    }
    if (path_length >= size_left) {
        trace ("Cache path truncated at %d bytes\n", size);
        return -1;
    }

    return 0;
}

static int
make_cache_path2 (char *path, int size, const char *fname, const char *album, const char *artist, int img_size) {
    path[0] = '\0';

    if (!album || !*album) {
        if (fname) {
            album = fname;
        }
        else if (artist && *artist) {
            album = artist;
        }
        else {
            trace ("not possible to get any unique album name\n");
            return -1;
        }
    }
    if (!artist || !*artist) {
        artist = "Unknown artist";
    }

    #ifdef __MINGW32__
    if (make_cache_dir_path (path, size, artist, img_size)) {
        return -1;
    }
    #else
    if (make_cache_dir_path (path, size-NAME_MAX, artist, img_size)) {
        return -1;
    }
    #endif

    int max_album_chars = min (NAME_MAX, size - strlen (path)) - sizeof ("1.jpg.part");
    if (max_album_chars <= 0) {
        trace ("Path buffer not long enough for %s and filename\n", path);
        return -1;
    }

    char esc_album[max_album_chars+1];
    const char *palbum = strlen (album) > max_album_chars ? album+strlen (album)-max_album_chars : album;
    size_t i = 0;
    do {
        esc_album[i] = esc_char (palbum[i]);
    } while (palbum[i++]);

    sprintf (path+strlen (path), "%s%s", esc_album, ".jpg");
    return 0;
}

static void
make_cache_path (char *path, int size, const char *album, const char *artist, int img_size) {
    make_cache_path2 (path, size, NULL, album, artist, img_size);
}

static const char *
get_default_cover (void)
{
    if (default_cover) {
        /* Just give back the current path */
        return default_cover;
    }

    if (missing_artwork == 1) {
        /* 1 is the Deadbeef default image */
        const char *pixmap_dir = deadbeef->get_system_dir (DDB_SYS_DIR_PIXMAP);
        default_cover = malloc (strlen (pixmap_dir) + 1 + sizeof (NOARTWORK_IMAGE));
        if (default_cover) {
            sprintf (default_cover, "%s/%s", pixmap_dir, NOARTWORK_IMAGE);
        }
    }
    else if (missing_artwork == 2 && nocover_path && nocover_path[0]) {
        /* 2 is a custom image in nocover_path */
        default_cover = strdup (nocover_path);
    }
    if (!default_cover) {
        /* Everything else, including errors, is an empty image */
        default_cover = "";
    }

    /* We have a new path, tell the caller */
    return NULL;
}

static void
clear_default_cover (void)
{
    if (default_cover && default_cover[0]) {
        free (default_cover);
    }
    default_cover = NULL;
}

static void
clear_query (cover_query_t *query)
{
    if (query->fname) {
        free (query->fname);
    }
    if (query->artist) {
        free (query->artist);
    }
    if (query->album) {
        free (query->album);
    }
    free (query);
}

static void
cache_reset_callback (const char *fname, const char *artist, const char *album, void *user_data)
{
    /* All scaled artwork is now (including this second) obsolete */
    deadbeef->mutex_lock (queue_mutex);
    scaled_cache_reset_time = time (NULL);
    deadbeef->conf_set_int64 ("artwork.scaled.cache_reset_time", scaled_cache_reset_time);

    if (user_data == &cache_reset_time) {
        /* All artwork is now (including this second) obsolete */
        cache_reset_time = scaled_cache_reset_time;
        deadbeef->conf_set_int64 ("artwork.cache_reset_time", cache_reset_time);
        deadbeef->sendmessage (DB_EV_PLAYLIST_REFRESH, 0, 0, 0);
    }
    deadbeef->mutex_unlock (queue_mutex);

    /* Wait for a new second to start before proceeding */
    while (time (NULL) == scaled_cache_reset_time) {
        usleep (100000);
    }
}

static cover_callback_t *
new_query_callback (artwork_callback cb, void *ud)
{
    if (!cb) {
        return NULL;
    }

    cover_callback_t *callback = malloc (sizeof (cover_callback_t));
    if (!callback) {
        trace ("artwork callback alloc failed\n");
        cb (NULL, NULL, NULL, ud);
        return NULL;
    }

    callback->cb = cb;
    callback->ud = ud;
    callback->next = NULL;
    return callback;
}

static int
strings_match (const char *s1, const char *s2)
{
    return s1 == s2 || s1 && s2 && !strcasecmp (s1, s2);
}

static void
enqueue_query (const char *fname, const char *artist, const char *album, int img_size, const artwork_callback cb, void *ud)
{
    for (cover_query_t *q = queue; q; q = q->next) {
        if (strings_match (artist, q->artist) && strings_match (album, q->album) && q->size == img_size) {
            trace ("artwork queue: %s %s %s %d already in queue - add to callbacks\n", fname, artist, album, img_size);
            cover_callback_t **last_callback = &q->callback;
            while (*last_callback && (*last_callback)->cb != cache_reset_callback) {
                last_callback = & (*last_callback)->next;
            }
            if (!*last_callback) {
                *last_callback = new_query_callback (cb, ud);
                return;
            }
        }
    }

    trace ("artwork queue: enqueue_query %s %s %s %d\n", fname, artist, album, img_size);
    cover_query_t *q = malloc (sizeof (cover_query_t));
    if (q) {
        q->fname = fname && *fname ? strdup (fname) : NULL;
        q->artist = artist ? strdup (artist) : NULL;
        q->album = album ? strdup (album) : NULL;
        q->size = img_size;
        q->next = NULL;
        q->callback = new_query_callback (cb, ud);

        if (fname && !q->fname || artist && !q->artist || album && !q->album) {
            clear_query (q);
            q = NULL;
        }
    }

    if (!q) {
        if (cb) {
            cb (NULL, NULL, NULL, ud);
        }
        return;
    }

    if (queue_tail) {
        queue_tail->next = q;
    }
    else {
        queue = q;
    }
    queue_tail = q;
    deadbeef->cond_signal (queue_cond);
}

static char *filter_custom_mask = NULL;

static int
filter_custom (const struct dirent *f)
{
// FNM_CASEFOLD is not defined on solaris. On other platforms it is.
// It should be safe to define it as FNM_INGORECASE if it isn't defined.
#ifndef FNM_CASEFOLD
#define FNM_CASEFOLD FNM_IGNORECASE
#endif
    return !fnmatch (filter_custom_mask, f->d_name, FNM_CASEFOLD);
}

static char *
vfs_scan_results (struct dirent *entry, const char *container_uri)
{
    /* VFS container, double check the match in case scandir didn't implement filtering */
    if (filter_custom (entry)) {
        trace ("found cover %s in %s\n", entry->d_name, container_uri);
        char *artwork_path = malloc (strlen (container_uri) + 1 + strlen (entry->d_name) + 1);
        if (artwork_path) {
            sprintf (artwork_path, "%s:%s", container_uri, entry->d_name);
            return artwork_path;
        }
    }

    return NULL;
}

static char *
dir_scan_results (struct dirent **files, int files_count, const char *container)
{
    /* Local file in directory */
    for (size_t i = 0; i < files_count; i++) {
        trace ("found cover %s in local folder\n", files[0]->d_name);
        char *artwork_path = malloc (strlen (container) + 1 + strlen (files[i]->d_name) + 1);
        if (artwork_path) {
            sprintf (artwork_path, "%s/%s", container, files[i]->d_name);
            struct stat stat_struct;
            if (!stat (artwork_path, &stat_struct) && S_ISREG (stat_struct.st_mode) && stat_struct.st_size > 0) {
                return artwork_path;
            }
            free (artwork_path);
        }
    }

    return NULL;
}

static int
scan_local_path (char *mask, const char *cache_path, const char *local_path, const char *uri, DB_vfs_t *vfsplug)
{
    filter_custom_mask = mask;
    struct dirent **files;
    int (* custom_scandir)(const char *, struct dirent ***, int (*)(const struct dirent *), int (*)(const struct dirent **, const struct dirent **));
    custom_scandir = vfsplug ? vfsplug->scandir : scandir;
    int files_count = custom_scandir (local_path, &files, filter_custom, NULL);
    if (files_count > 0) {
        char *artwork_path = uri ? vfs_scan_results (files[0], uri) : dir_scan_results (files, files_count, local_path);

        for (size_t i = 0; i < files_count; i++) {
            free (files[i]);
        }
        free (files);

        if (artwork_path) {
            int res = copy_file (artwork_path, cache_path);
            free (artwork_path);
            return res;
        }
    }

    return -1;
}

static void
extract_relative_path_from_mask (const char *mask, const char *local_path, char *trimmed_mask, char *full_local_path)
{
    strcpy (trimmed_mask, mask);
    strcpy (full_local_path, local_path);

    if (!mask[0] || mask[0] == '/' || mask[strlen (mask)-1] == '/')
        return;

    const char *end_of_path_in_mask = strrchr (mask, '/');
    if (end_of_path_in_mask == NULL)
        return;

    if (full_local_path[strlen (full_local_path)-1] != '/')
        strcat (full_local_path, "/");
    strncat (full_local_path, mask, (size_t)(end_of_path_in_mask - mask));
    strcpy (trimmed_mask, end_of_path_in_mask+1);
}

static int
local_image_file (const char *cache_path, const char *local_path, const char *uri, DB_vfs_t *vfsplug)
{
    if (!artwork_filemask) {
        return -1;
    }
    trace ("scanning %s for artwork\n", local_path);
    char filemask[strlen (artwork_filemask)+1];
    strcpy (filemask, artwork_filemask);
    const char *filemask_end = filemask + strlen (filemask);
    char *p;
    while (p = strrchr (filemask, ';')) {
        *p = '\0';
    }

    for (char *mask = filemask; mask < filemask_end; mask += strlen (mask)+1) {
        if (!mask[0])
            continue;

        int is_found = 0;
        if (strrchr (mask, '/')) {
            char *trimmed_mask = (char *)malloc (strlen (mask) + 1);
            char *full_local_path = (char *)malloc (strlen (local_path) + strlen (mask) + 1);

            extract_relative_path_from_mask (mask, local_path, trimmed_mask, full_local_path);
            is_found = !scan_local_path (trimmed_mask, cache_path, full_local_path, uri, vfsplug);

            free (trimmed_mask);
            free (full_local_path);
        }
        else {
            is_found = !scan_local_path (mask, cache_path, local_path, uri, vfsplug);
        }

        if (is_found) {
            return 0;
        }
    }
    if (!scan_local_path ("*.jpg", cache_path, local_path, uri, vfsplug) ||
        !scan_local_path ("*.jpeg", cache_path, local_path, uri, vfsplug) ||
        !scan_local_path ("*.png", cache_path, local_path, uri, vfsplug)) {
        return 0;
    }

    trace ("No cover art files in local folder\n");
    return -1;
}

static const uint8_t *
id3v2_skip_str (int enc, const uint8_t *ptr, const uint8_t *end) {
    if (enc == 0 || enc == 3) {
        while (ptr < end && *ptr) {
            ptr++;
        }
        ptr++;
        return ptr < end ? ptr : NULL;
    }
    else {
        while (ptr < end-1 && (ptr[0] || ptr[1])) {
            ptr += 2;
        }
        ptr += 2;
        return ptr < end ? ptr : NULL;
    }
}

static const uint8_t *
id3v2_artwork (const DB_id3v2_frame_t *f, int minor_version)
{
    if (strcmp (f->id, "APIC")) {
        return NULL;
    }

    if (f->size < 20) {
        trace ("artwork: id3v2 APIC frame is too small\n");
        return NULL;
    }

    const uint8_t *data = f->data;

    if (minor_version == 4 && (f->flags[1] & 1)) {
        data += 4;
    }
#if 0
    printf ("version: %d, flags: %d %d\n", minor_version, (int)f->flags[0], (int)f->flags[1]);
    for (size_t i = 0; i < 20; i++) {
        printf ("%c", data[i] < 0x20 ? '?' : data[i]);
    }
    printf ("\n");
    for (size_t i = 0; i < 20; i++) {
        printf ("%02x ", data[i]);
    }
    printf ("\n");
#endif
    const uint8_t *end = f->data + f->size;
    int enc = *data;
    data++; // enc
    // mime-type must always be ASCII - hence enc is 0 here
    const uint8_t *mime_end = id3v2_skip_str (enc, data, end);
    if (!mime_end) {
        trace ("artwork: corrupted id3v2 APIC frame\n");
        return NULL;
    }
//    if (strcasecmp (data, "image/jpeg") &&
#ifdef USE_IMLIB2
//        strcasecmp (data, "image/gif") &&
//        strcasecmp (data, "image/tiff") &&
#endif
//        strcasecmp (data, "image/png")) {
//        trace ("artwork: unsupported mime type: %s\n", data);
//        return NULL;
//    }
    if (*mime_end != 3 && *mime_end != 0) {
        trace ("artwork: picture type=%d\n", *mime_end);
        return NULL;
    }
    trace ("artwork: mime-type=%s, picture type: %d\n", data, *mime_end);
    data = mime_end;
    data++; // picture type
    data = id3v2_skip_str (enc, data, end); // description
    if (!data) {
        trace ("artwork: corrupted id3v2 APIC frame\n");
        return NULL;
    }

    return data;
}

static const uint8_t *
apev2_artwork (const DB_apev2_frame_t *f)
{
    if (strcasecmp (f->key, "cover art (front)")) {
        return NULL;
    }

    const uint8_t *data = f->data;
    const uint8_t *end = f->data + f->size;
    while (data < end && *data)
        data++;
    if (data == end) {
        trace ("artwork: apev2 cover art frame has no name\n");
        return NULL;
    }

    int sz = end - ++data;
    if (sz < 20) {
        trace ("artwork: apev2 cover art frame is too small\n");
        return NULL;
    }

//    uint8_t *ext = strrchr (f->data, '.');
//    if (!ext || !*++ext) {
//        trace ("artwork: apev2 cover art name has no extension\n");
//        return NULL;
//    }

//    if (strcasecmp (ext, "jpeg") &&
//        strcasecmp (ext, "jpg") &&
#ifdef USE_IMLIB2
//        strcasecmp (ext, "gif") &&
//        strcasecmp (ext, "tif") &&
//        strcasecmp (ext, "tiff") &&
#endif
//        strcasecmp (ext, "png")) {
//        trace ("artwork: unsupported file type: %s\n", ext);
//        return NULL;
//    }

    return data;
}

#ifdef USE_METAFLAC
static size_t
flac_io_read (void *ptr, size_t size, size_t nmemb, FLAC__IOHandle handle) {
    return deadbeef->fread (ptr, size, nmemb, (DB_FILE *)handle);
}

static int
flac_io_seek (FLAC__IOHandle handle, FLAC__int64 offset, int whence) {
    return deadbeef->fseek ((DB_FILE *)handle, offset, whence);
}

static FLAC__int64
flac_io_tell (FLAC__IOHandle handle) {
    return deadbeef->ftell ((DB_FILE *)handle);
}

static int
flac_io_eof (FLAC__IOHandle handle) {
    int64_t pos = deadbeef->ftell ((DB_FILE *)handle);
    return pos == deadbeef->fgetlength ((DB_FILE *)handle);
}

static int
flac_io_close (FLAC__IOHandle handle) {
    deadbeef->fclose ((DB_FILE *)handle);
    return 0;
}

static FLAC__IOCallbacks flac_iocb = {
    .read = flac_io_read,
    .write = NULL,
    .seek = flac_io_seek,
    .tell = flac_io_tell,
    .eof = NULL,
    .close = NULL
};

static int
flac_extract_art (const char *filename, const char *outname) {
    if (!strcasestr (filename, ".flac") && !strcasestr (filename, ".oga")) {
        return -1;
    }
    int err = -1;
    DB_FILE *file = NULL;
    FLAC__Metadata_Iterator *iterator = NULL;

    FLAC__Metadata_Chain *chain = FLAC__metadata_chain_new ();
    if (!chain) {
        return -1;
    }

    file = deadbeef->fopen (filename);
    if (!file) {
        trace ("artwork: failed to open %s\n", filename);
        goto error;
    }

    int res = FLAC__metadata_chain_read_with_callbacks (chain, (FLAC__IOHandle)file, flac_iocb);
#if USE_OGG
    if (!res && FLAC__metadata_chain_status (chain) == FLAC__METADATA_SIMPLE_ITERATOR_STATUS_NOT_A_FLAC_FILE) {
        res = FLAC__metadata_chain_read_ogg_with_callbacks (chain, (FLAC__IOHandle)file, flac_iocb);
    }
#endif
    deadbeef->fclose (file);
    if (!res) {
        trace ("artwork: failed to read metadata from flac: %s\n", filename);
        goto error;
    }

    FLAC__StreamMetadata *picture = 0;
    iterator = FLAC__metadata_iterator_new ();
    if (!iterator) {
        goto error;
    }
    FLAC__metadata_iterator_init (iterator, chain);
    do {
        FLAC__StreamMetadata *block = FLAC__metadata_iterator_get_block (iterator);
        if (block->type == FLAC__METADATA_TYPE_PICTURE) {
            picture = block;
        }
    } while (FLAC__metadata_iterator_next (iterator) && 0 == picture);

    if (!picture) {
        trace ("%s doesn't have an embedded cover\n", filename);
        goto error;
    }

    FLAC__StreamMetadata_Picture *pic = &picture->data.picture;
    if (pic && pic->data_length > 0) {
        trace ("found flac cover art of %d bytes (%s)\n", pic->data_length, pic->description);
        trace ("will write flac cover art into %s\n", outname);
        if (!write_file (outname, pic->data, pic->data_length)) {
            err = 0;
        }
    }
error:
    if (chain) {
        FLAC__metadata_chain_delete (chain);
    }
    if (iterator) {
        FLAC__metadata_iterator_delete (iterator);
    }
    return err;
}
#endif

static int
id3_extract_art (const char *fname, const char *outname) {
    int err = -1;

    DB_id3v2_tag_t id3v2_tag;
    memset (&id3v2_tag, 0, sizeof (id3v2_tag));
    DB_FILE *id3v2_fp = deadbeef->fopen (fname);
    if (id3v2_fp && !deadbeef->junk_id3v2_read_full (NULL, &id3v2_tag, id3v2_fp)) {
        int minor_version = id3v2_tag.version[0];
        for (DB_id3v2_frame_t *f = id3v2_tag.frames; f; f = f->next) {
            const uint8_t *image_data = id3v2_artwork (f, minor_version);
            if (image_data) {
                const size_t sz = f->size - (image_data - f->data);
                trace ("will write id3v2 APIC (%d bytes) into %s\n", sz, outname);
                if (sz > 0 && !write_file (outname, image_data, sz)) {
                    err = 0;
                }
            }
        }
    }
    deadbeef->junk_id3v2_free (&id3v2_tag);
    if (id3v2_fp) {
        deadbeef->fclose (id3v2_fp);
    }
    return err;
}

static int
apev2_extract_art (const char *fname, const char *outname) {
    int err = -1;
    DB_apev2_tag_t apev2_tag;
    memset (&apev2_tag, 0, sizeof (apev2_tag));
    DB_FILE *apev2_fp = deadbeef->fopen (fname);
    if (apev2_fp && !deadbeef->junk_apev2_read_full (NULL, &apev2_tag, apev2_fp)) {
        for (DB_apev2_frame_t *f = apev2_tag.frames; f; f = f->next) {
            const uint8_t *image_data = apev2_artwork (f);
            if (image_data) {
                const size_t sz = f->size - (image_data - f->data);
                trace ("will write apev2 cover art (%d bytes) into %s\n", sz, outname);
                if (sz > 0 && !write_file (outname, image_data, sz)) {
                    err = 0;
                }
                break;
            }
        }

    }
    deadbeef->junk_apev2_free (&apev2_tag);
    if (apev2_fp) {
        deadbeef->fclose (apev2_fp);
    }
    return err;
}

static int
mp4_extract_art (const char *fname, const char *outname) {
    int ret = -1;
    mp4p_atom_t *mp4file = NULL;
    DB_FILE* fp = NULL;
    uint8_t* image_blob = NULL;

    if (!strcasestr (fname, ".mp4") && !strcasestr (fname, ".m4a") && !strcasestr (fname, ".m4b")) {
        return -1;
    }

    fp = deadbeef->fopen (fname);
    if (!fp) {
        goto error;
    }

    mp4p_file_callbacks_t callbacks = {0};
    callbacks.ptrhandle = fp;
    mp4_init_ddb_file_callbacks (&callbacks);
    mp4file = mp4p_open(&callbacks);
    if (!mp4file) {
        goto error;
    }

    mp4p_atom_t *covr = mp4_get_cover_atom(mp4file);
    if (!covr) {
        goto error;
    }

    mp4p_ilst_meta_t *data = covr->data;

    uint32_t sz = data->data_size;
    image_blob = malloc (sz);
    if (data->blob) {
        image_blob = memcpy (image_blob, data->blob, sz);
    }
    else if (data->values) {
        uint16_t *v = data->values;
        for (size_t i = 0; i < sz/2; i++, v++) {
            image_blob[i*2+0] = (*v) >> 8;
            image_blob[i*2+1] = (*v) & 0xff;
        }
    }
    else {
        goto error;
    }

    trace ("will write mp4 cover art (%u bytes) into %s\n", sz, outname);
    if (!write_file (outname, (char *)image_blob, sz)) {
        ret = 0;
    }
    free (image_blob);
    image_blob = NULL;

error:
    if (image_blob) {
        free(image_blob);
        image_blob = NULL;
    }
    if (mp4file) {
        mp4p_atom_free_list (mp4file);
        mp4file = NULL;
    }
    if (fp) {
        deadbeef->fclose (fp);
        fp = NULL;
    }
    return ret;
}


static int
web_lookups (const char *artist, const char *album, const char *cache_path)
{
#if USE_VFS_CURL
    if (artwork_enable_lfm) {
        if (!fetch_from_lastfm (artist, album, cache_path)) {
            return 1;
        }
        if (errno == ECONNABORTED) {
            return 0;
        }
    }

    if (artwork_enable_mb) {
        if (!fetch_from_musicbrainz (artist, album, cache_path)) {
            return 1;
        }
        if (errno == ECONNABORTED) {
            return 0;
        }
    }

    if (artwork_enable_aao) {
        if (!fetch_from_albumart_org (artist, album, cache_path)) {
            return 1;
        }
        if (errno == ECONNABORTED) {
            return 0;
        }
    }
#endif

    return -1;
}

static int
process_scaled_query (const cover_query_t *query)
{
    char unscaled_path[PATH_MAX];
    make_cache_path2 (unscaled_path, sizeof (unscaled_path), query->fname, query->album, query->artist, -1);

    struct stat stat_buf;
    if (!stat (unscaled_path, &stat_buf) && S_ISREG (stat_buf.st_mode) && stat_buf.st_size > 0) {
        char scaled_path[PATH_MAX];
        make_cache_path2 (scaled_path, sizeof (scaled_path), query->fname, query->album, query->artist, query->size);
        trace ("artwork: scaling %s into %s (%d pixels)\n", unscaled_path, scaled_path, query->size);
        if (*scaled_path && !scale_file (unscaled_path, scaled_path, query->size)) {
            return 1;
        }
    }

    return 0;
}

static char *
vfs_path (const char *fname)
{
    if (fname[0] == '/' || strstr (fname, "file://") == fname) {
        return NULL;
    }

    char *p = strstr (fname, "://");
    if (p) {
        p += 3;
        char *q = strrchr (p, ':');
        if (q) {
            *q = '\0';
        }
    }
    return p;
}

static DB_vfs_t *
scandir_plug (const char *vfs_fname)
{
    DB_vfs_t **vfsplugs = deadbeef->plug_get_vfs_list ();
    for (size_t i = 0; vfsplugs[i]; i++) {
        if (vfsplugs[i]->is_container && vfsplugs[i]->is_container (vfs_fname) && vfsplugs[i]->scandir) {
            return vfsplugs[i];
        }
    }

    return NULL;
}

static int
path_more_recent (const char *fname, const time_t placeholder_mtime)
{
    struct stat stat_buf;
    return !stat (fname, &stat_buf) && stat_buf.st_mtime > placeholder_mtime;
}

static int
recheck_missing_artwork (const char *input_fname, const time_t placeholder_mtime)
{
    int res = 0;
    char *fname = strdup (input_fname);
    /* Check if local files could have new associated artwork */
    if (deadbeef->is_local_file (fname)) {
        char *vfs_fname = vfs_path (fname);
        const char *real_fname = vfs_fname ? vfs_fname : fname;

        /* Recheck artwork if file (track or VFS container) was modified since the last check */
        if (path_more_recent (real_fname, placeholder_mtime)) {
            return 1;
        }

        /* Recheck local artwork if the directory contents have changed */
        char *dname = strdup (dirname (fname));
        res = artwork_enable_local && path_more_recent (dname, placeholder_mtime);
        free (dname);
    }

    free (fname);
    return res;
}

static int
process_query (const cover_query_t *query)
{
    if (!query->fname) {
        return 0;
    }

    char cache_path[PATH_MAX];
    make_cache_path2 (cache_path, sizeof (cache_path), query->fname, query->album, query->artist, -1);
    trace ("artwork: query cover for %s %s to %s\n", query->album, query->artist, cache_path);

    /* Flood control, don't retry missing artwork for an hour unless something changes */
    struct stat placeholder_stat;
    if (!stat (cache_path, &placeholder_stat) && placeholder_stat.st_mtime + 60*60 > time (NULL)) {
        int recheck = recheck_missing_artwork (query->fname, placeholder_stat.st_mtime);
        if (!recheck) {
            return 0;
        }
    }

    if (artwork_enable_local && deadbeef->is_local_file (query->fname)) {
        char *fname_copy = strdup (query->fname);
        if (fname_copy) {
            char *vfs_fname = vfs_path (fname_copy);
            if (vfs_fname) {
                /* Search inside scannable VFS containers */
                DB_vfs_t *plugin = scandir_plug (vfs_fname);
                if (plugin && !local_image_file (cache_path, vfs_fname, fname_copy, plugin)) {
                    free (fname_copy);
                    return 1;
                }
            }

            /* Search in file directory */
            if (!local_image_file (cache_path, dirname (vfs_fname ? vfs_fname : fname_copy), NULL, NULL)) {
                free (fname_copy);
                return 1;
            }

            free (fname_copy);
        }
    }

    if (artwork_enable_embedded && deadbeef->is_local_file (query->fname)) {
#ifdef USE_METAFLAC
        // try to load embedded from flac metadata
        trace ("trying to load artwork from Flac tag for %s\n", query->fname);
        if (!flac_extract_art (query->fname, cache_path)) {
            return 1;
        }
#endif

        // try to load embedded from id3v2
        trace ("trying to load artwork from id3v2 tag for %s\n", query->fname);
        if (!id3_extract_art (query->fname, cache_path)) {
            return 1;
        }

        // try to load embedded from apev2
        trace ("trying to load artwork from apev2 tag for %s\n", query->fname);
        if (!apev2_extract_art (query->fname, cache_path)) {
            return 1;
        }

        // try to load embedded from mp4
        trace ("trying to load artwork from mp4 tag for %s\n", query->fname);
        if (!mp4_extract_art (query->fname, cache_path)) {
            return 1;
        }
    }

#ifdef USE_VFS_CURL
    /* Web lookups */
    if (artwork_enable_wos && strlen (query->fname) > 3 && !strcasecmp (query->fname+strlen (query->fname)-3, ".ay")) {
        if (!fetch_from_wos (query->album, cache_path)) {
            return 1;
        }
        if (errno == ECONNABORTED) {
            return 0;
        }
    }

    int res = web_lookups (query->artist, query->album, cache_path);
    if (res >= 0) {
        return res;
    }

    if (query->album) {
        /* Try stripping parenthesised text off the end of the album name */
        char *p = strpbrk (query->album, "([");
        if (p) {
            char parenthesis = *p;
            *p = '\0';
            int res = web_lookups (query->artist, query->album, cache_path);
            *p = '(';
            if (res >= 0) {
                return res;
            }
        }
    }
#endif

    /* Touch placeholder */
    write_file (cache_path, NULL, 0);

    return 0;
}

static void
send_query_callbacks (cover_callback_t *callback, const char *fname, const char *artist, const char *album)
{
    if (callback) {
        trace ("Make callback with data %s %s %s to %p (next=%p)\n", fname, artist, album, callback->ud, callback->next);
        callback->cb (fname, artist, album, callback->ud);
        send_query_callbacks (callback->next, fname, artist, album);
        free (callback);
    }
}

static cover_query_t *
dequeue_query (void)
{
    cover_query_t *query = queue;
    queue = queue->next;
    if (!queue) {
        queue_tail = NULL;
    }
    return query;
}

static void
queue_clear (void)
{
    /* Remove everything except the first query */
    if (queue) {
        while (queue->next) {
            cover_query_t *query = queue->next;
            queue->next = query->next;
            send_query_callbacks (query->callback, NULL, NULL, NULL);
            clear_query (query);
        }
        queue_tail = queue;
    }
}

static void
fetcher_thread (void *none)
{
#ifdef __linux__
    prctl (PR_SET_NAME, "deadbeef-artwork", 0, 0, 0, 0);
#endif

    /* Loop until external terminate command */
    deadbeef->mutex_lock (queue_mutex);
    while (!terminate) {
        trace ("artwork fetcher: waiting for signal ...\n");
        pthread_cond_wait ((pthread_cond_t *)queue_cond, (pthread_mutex_t *)queue_mutex);
        trace ("artwork fetcher: cond signalled, process queue\n");

        /* Loop until queue is empty */
        while (queue) {
            deadbeef->mutex_unlock (queue_mutex);

            /* Process this query, hopefully writing a file into cache */
            int cached_art = queue->size == -1 ? process_query (queue) : process_scaled_query (queue);
            deadbeef->mutex_lock (queue_mutex);
            cover_query_t *query = dequeue_query ();
            deadbeef->mutex_unlock (queue_mutex);

            /* Make all the callbacks (and free the chain), with data if a file was written */
            if (cached_art) {
                trace ("artwork fetcher: cover art file cached\n");
                send_query_callbacks (query->callback, query->fname, query->artist, query->album);
            }
            else {
                trace ("artwork fetcher: no cover art found\n");
                send_query_callbacks (query->callback, NULL, NULL, NULL);
            }
            clear_query (query);

            /* Look for what to do next */
            deadbeef->mutex_lock (queue_mutex);
        }
    }
    deadbeef->mutex_unlock (queue_mutex);
    trace ("artwork fetcher: terminate thread\n");
}

static int
check_file_age (const char *path, const time_t mtime, const time_t reset_time)
{
    if (mtime <= reset_time) {
        trace ("artwork: deleting cached file %s after reset\n", path);
        unlink (path);
        return 0;
    }

    return 1;
}

static const char *
find_image (const char *path, const time_t reset_time)
{
    struct stat stat_buf;
    if (stat (path, &stat_buf) || !S_ISREG (stat_buf.st_mode)) {
        trace ("artwork: %s not found or not regular file\n", path);
        return NULL;
    }

    if (stat_buf.st_size == 0 && !check_file_age (path, stat_buf.st_mtime, default_reset_time)) {
        trace ("artwork: %s invalidated after default artwork reset\n", path);
        return NULL;
    }

    if (!check_file_age (path, stat_buf.st_mtime, reset_time) || stat_buf.st_size == 0) {
        trace ("artwork: %s is a placeholder or was invalidated after cache reset\n", path);
        return NULL;
    }

    return path;
}

static char *
get_album_art (const char *fname, const char *artist, const char *album, int size, artwork_callback callback, void *user_data)
{
    /* Check if the image is already cached */
    char cache_path[PATH_MAX];
    make_cache_path2 (cache_path, sizeof (cache_path), fname, album, artist, size);
    deadbeef->mutex_lock (queue_mutex);
    const time_t reset_time = size == -1 ? cache_reset_time : scaled_cache_reset_time;
    deadbeef->mutex_unlock (queue_mutex);
    const char *p = find_image (cache_path, reset_time);
    if (p) {
        trace ("Found cached image %s\n", cache_path);
        return strdup (p);
    }

    /* See if we need to make an unscaled image before we make a scaled one */
    deadbeef->mutex_lock (queue_mutex);
    if (size != -1) {
        char unscaled_path[PATH_MAX];
        make_cache_path2 (unscaled_path, sizeof (unscaled_path), fname, album, artist, -1);
        if (!find_image (unscaled_path, cache_reset_time)) {
            enqueue_query (fname, artist, album, -1, NULL, NULL);
        }
    }

    /* Request to fetch the image */
    enqueue_query (fname, artist, album, size, callback, user_data);
    deadbeef->mutex_unlock (queue_mutex);
    return NULL;
}

static void
artwork_reset (int fast) {
    trace ("artwork:%s reset queue\n", fast ? " fast" : "");
    deadbeef->mutex_lock (queue_mutex);
    queue_clear ();
    if (!fast && queue && queue->callback) {
        cover_callback_t *callback_chain = queue->callback;
        queue->callback = NULL;
        send_query_callbacks (callback_chain, NULL, NULL, NULL);
    }
    deadbeef->mutex_unlock (queue_mutex);
}

static void
get_fetcher_preferences (void)
{
    artwork_enable_embedded = deadbeef->conf_get_int ("artwork.enable_embedded", 1);
    artwork_enable_local = deadbeef->conf_get_int ("artwork.enable_localfolder", 1);
    if (artwork_enable_local) {
        deadbeef->conf_lock ();
        const char *new_artwork_filemask = deadbeef->conf_get_str_fast ("artwork.filemask", NULL);
        if (!new_artwork_filemask || !new_artwork_filemask[0]) {
            new_artwork_filemask = DEFAULT_FILEMASK;
            deadbeef->conf_set_str ("artwork.filemask", new_artwork_filemask);
        }
        if (!strings_match (artwork_filemask, new_artwork_filemask)) {
            char *old_artwork_filemask = artwork_filemask;
            artwork_filemask = strdup (new_artwork_filemask);
            if (old_artwork_filemask) {
                free (old_artwork_filemask);
            }
        }
        deadbeef->conf_unlock ();
    }
#ifdef USE_VFS_CURL
    artwork_enable_lfm = deadbeef->conf_get_int ("artwork.enable_lastfm", 0);
    artwork_enable_mb = deadbeef->conf_get_int ("artwork.enable_musicbrainz", 0);
    artwork_enable_aao = deadbeef->conf_get_int ("artwork.enable_albumartorg", 0);
    artwork_enable_wos = deadbeef->conf_get_int ("artwork.enable_wos", 0);
#endif
    scale_towards_longer = deadbeef->conf_get_int ("artwork.scale_towards_longer", 1);
    missing_artwork = deadbeef->conf_get_int ("artwork.missing_artwork", 1);
    if (missing_artwork == 2) {
        deadbeef->conf_lock ();
        const char *new_nocover_path = deadbeef->conf_get_str_fast ("artwork.nocover_path", NULL);
        if (!strings_match (new_nocover_path, nocover_path)) {
            char *old_nocover_path = nocover_path;
            nocover_path = new_nocover_path ? strdup (new_nocover_path) : NULL;
            if (old_nocover_path) {
                free (old_nocover_path);
            }
        }
        deadbeef->conf_unlock ();
    }
}

static void
insert_cache_reset (void *user_data)
{
    /* No point jumping through hoops if the reset time is already now */
    if (scaled_cache_reset_time == time (NULL)) {
        return;
    }

    /* Submit a dummy query to set the cache reset time in a callback */
    if (!queue) {
        enqueue_query (NULL, NULL, NULL, -1, cache_reset_callback, user_data);
        return;
    }

    cover_callback_t **last_callback = &queue->callback;
    while (*last_callback) {
        if ((*last_callback)->cb == cache_reset_callback) {
            /* Set an existing callback to do the right resets */
            if ((*last_callback)->ud == &scaled_cache_reset_time && user_data == &cache_reset_time) {
                (*last_callback)->ud = user_data;
            }
            return;
        }
        last_callback = & (*last_callback)->next;
    }

    /* Add a new callback at the end of the chain */
    *last_callback = new_query_callback (cache_reset_callback, user_data);
}

static void
artwork_configchanged (void)
{
    cache_configchanged ();

    int old_artwork_enable_embedded = artwork_enable_embedded;
    int old_artwork_enable_local = artwork_enable_local;
    const char *old_artwork_filemask = artwork_filemask;
#ifdef USE_VFS_CURL
    int old_artwork_enable_lfm = artwork_enable_lfm;
    int old_artwork_enable_mb = artwork_enable_mb;
    int old_artwork_enable_aao = artwork_enable_aao;
    int old_artwork_enable_wos = artwork_enable_wos;
#endif
    int old_missing_artwork = missing_artwork;
    const char *old_nocover_path = nocover_path;
    int old_scale_towards_longer = scale_towards_longer;

    get_fetcher_preferences ();

    if (old_missing_artwork != missing_artwork || old_nocover_path != nocover_path) {
        trace ("artwork config changed, invalidating default artwork...\n");
        clear_default_cover ();
        default_reset_time = time (NULL);
        deadbeef->sendmessage (DB_EV_PLAYLIST_REFRESH, 0, 0, 0);
    }

    if (old_artwork_enable_embedded != artwork_enable_embedded ||
        old_artwork_enable_local != artwork_enable_local ||
#ifdef USE_VFS_CURL
        old_artwork_enable_lfm != artwork_enable_lfm ||
        old_artwork_enable_mb != artwork_enable_mb ||
        old_artwork_enable_aao != artwork_enable_aao ||
        old_artwork_enable_wos != artwork_enable_wos ||
#endif
        old_artwork_filemask != artwork_filemask) {
        trace ("artwork config changed, invalidating cache...\n");
        deadbeef->mutex_lock (queue_mutex);
        insert_cache_reset (&cache_reset_time);
        artwork_abort_http_request ();
        deadbeef->mutex_unlock (queue_mutex);
    }
    else if (old_scale_towards_longer != scale_towards_longer) {
        trace ("artwork config changed, invalidating scaled cache...\n");
        deadbeef->mutex_lock (queue_mutex);
        insert_cache_reset (&scaled_cache_reset_time);
        deadbeef->mutex_unlock (queue_mutex);
    }
}

static int
artwork_message (uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2) {
    switch (id) {
    case DB_EV_CONFIGCHANGED:
        artwork_configchanged ();
        break;
    }
    return 0;
}

static int
invalidate_playitem_cache (DB_plugin_action_t *action, ddb_action_context_t ctx)
{
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    if (!plt)
        return -1;

    DB_playItem_t *it = deadbeef->plt_get_first (plt, PL_MAIN);
    while (it) {
        if (deadbeef->pl_is_selected (it)) {
            deadbeef->pl_lock ();
            const char *url = deadbeef->pl_find_meta (it, ":URI");
            const char *artist = deadbeef->pl_find_meta (it, "artist");
            const char *album = deadbeef->pl_find_meta (it, "album");
            const char *title = album ? album : deadbeef->pl_find_meta (it, "title");
            char cache_path[PATH_MAX];
            if (!make_cache_path2 (cache_path, PATH_MAX, url, title, artist, -1)) {
                char subdir_path[PATH_MAX];
                make_cache_dir_path (subdir_path, PATH_MAX, artist, -1);
                const char *subdir_name = basename (subdir_path);
                const char *entry_name = basename (cache_path);
                trace ("Expire %s from cache\n", cache_path);
                remove_cache_item (cache_path, subdir_path, subdir_name, entry_name);
            }
            deadbeef->pl_unlock ();
        }
        deadbeef->pl_item_unref (it);
        it = deadbeef->pl_get_next (it, PL_MAIN);
    }
    deadbeef->plt_unref (plt);

    clear_default_cover ();

    deadbeef->sendmessage (DB_EV_PLAYLIST_REFRESH, 0, 0, 0);

    return 0;
}

static DB_plugin_action_t *
artwork_get_actions (DB_playItem_t *it)
{
    if (!it) // Only currently show for the playitem context menu
        return NULL;

    static DB_plugin_action_t context_action = {
        .title = "Refresh cover art",
        .name = "invalidate_playitem_cache",
        .callback2 = invalidate_playitem_cache,
        .flags = DB_ACTION_ADD_MENU | DB_ACTION_SINGLE_TRACK | DB_ACTION_MULTIPLE_TRACKS,
        .next = NULL
    };

    return &context_action;
}

static int
artwork_plugin_stop (void)
{
    if (tid) {
        trace ("Stopping fetcher thread ... \n");
        deadbeef->mutex_lock (queue_mutex);
        queue_clear ();
        terminate = 1;
        deadbeef->cond_signal (queue_cond);
        while (queue) {
            artwork_abort_http_request ();
            deadbeef->mutex_unlock (queue_mutex);
            usleep (10000);
            deadbeef->mutex_lock (queue_mutex);
        }
        deadbeef->mutex_unlock (queue_mutex);
        deadbeef->thread_join (tid);
        tid = 0;
        trace ("Fetcher thread stopped\n");
    }
    if (queue_mutex) {
        deadbeef->mutex_free (queue_mutex);
        queue_mutex = 0;
    }
    if (queue_cond) {
        deadbeef->cond_free (queue_cond);
        queue_cond = 0;
    }

    if (artwork_filemask) {
        free (artwork_filemask);
    }
    clear_default_cover ();
    if (nocover_path) {
        free (nocover_path);
    }

    stop_cache_cleaner ();

    return 0;
}

static int
artwork_plugin_start (void)
{
    get_fetcher_preferences ();
    cache_reset_time = deadbeef->conf_get_int64 ("artwork.cache_reset_time", 0);
    scaled_cache_reset_time = deadbeef->conf_get_int64 ("artwork.scaled.cache_reset_time", 0);

#ifdef USE_IMLIB2
    imlib_set_cache_size (0);
#endif

    terminate = 0;
    queue_mutex = deadbeef->mutex_create_nonrecursive ();
    queue_cond = deadbeef->cond_create ();
    if (queue_mutex && queue_cond) {
        tid = deadbeef->thread_start_low_priority (fetcher_thread, NULL);
    }
    if (!tid) {
        artwork_plugin_stop ();
        return -1;
    }

    start_cache_cleaner ();

    return 0;
}

static const char settings_dlg[] =
    "property box hbox[1] border=8 height=-1;\n"
    "property \"Cache update period (in hours, 0=never)\" entry artwork.cache.period 48;\n"
    "property \"Fetch from embedded tags\" checkbox artwork.enable_embedded 1;\n"
    "property box hbox[2] spacing=0 height=-1;\n"
    "property \"Fetch from local folder\" checkbox artwork.enable_localfolder 1;\n"
    "property box vbox[1] expand fill height=-1;\n"
    "property \" -\" entry artwork.filemask \"" DEFAULT_FILEMASK "\";\n"
#ifdef USE_VFS_CURL
    "property box hbox[3] spacing=16 height=-1;\n"
    "property \"Fetch from Last.fm\" checkbox artwork.enable_lastfm 0;\n"
    "property \"Fetch from MusicBrainz\" checkbox artwork.enable_musicbrainz 0;\n"
    "property \"Fetch from Albumart.org\" checkbox artwork.enable_albumartorg 0;\n"
    "property \"Fetch from World of Spectrum (AY files only)\" checkbox artwork.enable_wos 0;\n"
#endif
    "property box vbox[2] spacing=4 border=8;\n"
    "property box hbox[1] height=-1;"
    "property \"When no artwork is found\" select[3] artwork.missing_artwork 1 \"leave blank\" \"use DeaDBeeF default cover\" \"display custom image\";"
    "property \"Custom image path\" file artwork.nocover_path \"\";\n"
    "property \"Scale artwork towards longer side\" checkbox artwork.scale_towards_longer 1;\n"
;

// define plugin interface
static DB_artwork_plugin_t plugin = {
    .plugin.plugin.api_vmajor = DB_API_VERSION_MAJOR,
    .plugin.plugin.api_vminor = DB_API_VERSION_MINOR,
    .plugin.plugin.version_major = 1,
    .plugin.plugin.version_minor = DDB_ARTWORK_VERSION,
    .plugin.plugin.type = DB_PLUGIN_MISC,
    .plugin.plugin.id = "artwork",
    .plugin.plugin.name = "Album Artwork",
    .plugin.plugin.descr = "Loads album artwork from embedded tags, local directories, or internet services",
    .plugin.plugin.copyright =
        "Album Art plugin for DeaDBeeF\n"
        "Copyright (C) 2009-2011 Viktor Semykin <thesame.ml@gmail.com>\n"
        "Copyright (C) 2009-2013 Alexey Yakovenko <waker@users.sourceforge.net>\n"
        "\n"
        "This software is provided 'as-is', without any express or implied\n"
        "warranty.  In no event will the authors be held liable for any damages\n"
        "arising from the use of this software.\n"
        "\n"
        "Permission is granted to anyone to use this software for any purpose,\n"
        "including commercial applications, and to alter it and redistribute it\n"
        "freely, subject to the following restrictions:\n"
        "\n"
        "1. The origin of this software must not be misrepresented; you must not\n"
        " claim that you wrote the original software. If you use this software\n"
        " in a product, an acknowledgment in the product documentation would be\n"
        " appreciated but is not required.\n"
        "\n"
        "2. Altered source versions must be plainly marked as such, and must not be\n"
        " misrepresented as being the original software.\n"
        "\n"
        "3. This notice may not be removed or altered from any source distribution.\n"
    ,
    .plugin.plugin.website = "http://deadbeef.sf.net",
    .plugin.plugin.start = artwork_plugin_start,
    .plugin.plugin.stop = artwork_plugin_stop,
    .plugin.plugin.configdialog = settings_dlg,
    .plugin.plugin.message = artwork_message,
    .plugin.plugin.get_actions = artwork_get_actions,
    .get_album_art = get_album_art,
    .reset = artwork_reset,
    .get_default_cover = get_default_cover,
    .get_album_art_sync = NULL,
    .make_cache_path = make_cache_path,
    .make_cache_path2 = make_cache_path2,
};

DB_plugin_t *
artwork_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}
