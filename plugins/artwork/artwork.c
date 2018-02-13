/*
    Album Art plugin for DeaDBeeF
    Copyright (C) 2009-2017 Alexey Yakovenko <waker@users.sourceforge.net>
    Copyright (C) 2009-2011 Viktor Semykin <thesame.ml@gmail.com>
    Copyright (C) 2014-2016 Ian Nartowicz <deadbeef@nartowicz.co.uk>

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
#ifdef USE_LIBJPEG
    #include <jpeglib.h>
#endif
#ifdef USE_LIBPNG
    #include <png.h>
#endif
#endif
#include "../../deadbeef.h"
#include "artwork.h"
#include "artwork_internal.h"
#include "lastfm.h"
#include "musicbrainz.h"
#include "albumartorg.h"
#include "wos.h"
#include "cache.h"
#ifdef USE_MP4FF
#include "mp4ff.h"
#endif
#include "../../strdupa.h"

#define trace(...) { deadbeef->log_detailed (&plugin.plugin.plugin, 0, __VA_ARGS__); }

DB_functions_t *deadbeef;
static ddb_artwork_plugin_t plugin;

// list of callbacks + queries for the same cover
typedef struct cover_callback_s {
    ddb_cover_callback_t cb;
    ddb_cover_query_t *info;
    struct cover_callback_s *next;
} cover_callback_t;

// list of unique queries
typedef struct cover_query_s {
    cover_callback_t *callbacks;
    struct cover_query_s *next;
} cover_query_t;

static cover_query_t *queue;
static cover_query_t *queue_tail;
static int terminate;
static intptr_t tid;
static uintptr_t queue_mutex;
static uintptr_t queue_cond;

#ifdef ANDROID
#define DEFAULT_DISABLE_CACHE 1
#define DEFAULT_SAVE_TO_MUSIC_FOLDERS 1
static int artwork_disable_cache = DEFAULT_DISABLE_CACHE;
static int artwork_save_to_music_folders = DEFAULT_SAVE_TO_MUSIC_FOLDERS;
#else
#define DEFAULT_DISABLE_CACHE 0
#define DEFAULT_SAVE_TO_MUSIC_FOLDERS 0
static int artwork_disable_cache = DEFAULT_DISABLE_CACHE;
static int artwork_save_to_music_folders = DEFAULT_SAVE_TO_MUSIC_FOLDERS;
#endif

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
static time_t default_reset_time;

#define DEFAULT_FILEMASK "front.png;front.jpg;folder.png;folder.jpg;cover.png;cover.jpg;f.png;f.jpg;*front*.png;*front*.jpg;*cover*.png;*cover*.jpg;*folder*.png;*folder*.jpg;*.png;*.jpg"
#define DEFAULT_FOLDERS "art;scans;covers;artwork;artworks"

static char *artwork_filemask;
static char *artwork_folders;

static char *album_tf;
static char *artist_tf;

#if defined(USE_IMLIB2) || defined(USE_LIBJPEG)
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
#endif

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

#ifdef USE_LIBPNG
static uint_fast32_t blerp_pixel (const png_byte *row, const png_byte *next_row, const uint_fast32_t x_index, const uint_fast32_t next_x_index,
                                 const uint_fast32_t weight, const uint_fast32_t weightx, const uint_fast32_t weighty, const uint_fast32_t weightxy)
{
    return row[x_index]*weight + row[next_x_index]*weightx + next_row[x_index]*weighty + next_row[next_x_index]*weightxy;
}
#endif

#if defined(USE_LIBPNG) || defined(USE_LIBJPEG)
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
#endif

#ifdef USE_LIBJPEG
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
#endif

#ifdef USE_LIBPNG
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

                uint_fast32_t alpha = 0, alphax = 0, alphay = 0, alphaxy = 0;
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

#if defined(USE_IMLIB2) || defined(USE_LIBJPEG) || defined(USE_LIBPNG)
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

    int err = -1;

#ifdef USE_LIBJPEG
    err = jpeg_resize (in, out, img_size);
    if (err) {
        unlink (out);
    }
#endif
#ifdef USE_LIBPNG
    if (err) {
        err = png_resize (in, out, img_size);
    }
    if (err) {
        unlink (out);
    }
#endif
#endif
    cache_unlock ();
    return err;
}
#endif

// esc_char is needed to prevent using file path separators,
// e.g. to avoid writing arbitrary files using "../../../filename"
static char
esc_char (char c) {
#ifndef WIN32
    if (c == '/') {
        return '\\';
    }
#else
    if (c == '\\') {
        return '_';
    }
#endif
    return c;
}

static int
make_cache_dir_path (const char *artist, char *outpath, int outsize) {
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

    if (make_cache_root_path (outpath, outsize) < 0) {
        return -1;
    }

    const size_t size_left = outsize - strlen (outpath);
    int path_length;
    path_length = snprintf (outpath+strlen (outpath), size_left, "covers2/%s/", esc_artist);
    if (path_length >= size_left) {
        trace ("Cache path truncated at %d bytes\n", (int)size_left);
        return -1;
    }

    return 0;
}

static int
make_cache_path (const char *filepath, const char *album, const char *artist, char *outpath, int outsize) {
    outpath[0] = '\0';

    if (!album || !*album) {
        if (filepath) {
            album = filepath;
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

    if (make_cache_dir_path (artist, outpath, outsize-NAME_MAX)) {
        return -1;
    }

    int name_size = outsize - (int)strlen (outpath);
    int max_album_chars = min (NAME_MAX, name_size) - (int)sizeof ("1.jpg.part");
    if (max_album_chars <= 0) {
        trace ("Path buffer not long enough for %s and filename\n", outpath);
        return -1;
    }

    char esc_album[max_album_chars+1];
    const char *palbum = strlen (album) > max_album_chars ? album+strlen (album)-max_album_chars : album;
    size_t i = 0;
    do {
        esc_album[i] = esc_char (palbum[i]);
    } while (palbum[i++]);

    sprintf (outpath+strlen (outpath), "%s%s", esc_album, ".jpg");
    return 0;
}

static void
query_free (cover_query_t *query)
{
    free (query);
}

static void
cache_reset_callback (int error, ddb_cover_query_t *query, ddb_cover_info_t *cover) {
    /* All scaled artwork is now (including this second) obsolete */
    deadbeef->mutex_lock (queue_mutex);

    if (query->user_data == &cache_reset_time) {
        /* All artwork is now (including this second) obsolete */
        deadbeef->conf_set_int64 ("artwork.cache_reset_time", cache_reset_time);
        deadbeef->sendmessage (DB_EV_PLAYLIST_REFRESH, 0, 0, 0);
    }
    deadbeef->mutex_unlock (queue_mutex);

    /* Wait for a new second to start before proceeding */
    while (time (NULL) == cache_reset_time) {
        usleep (100000);
    }
}

static cover_callback_t *
new_query_callback (ddb_cover_callback_t cb, ddb_cover_query_t *info) {
    if (!cb) {
        return NULL;
    }

    cover_callback_t *callback = calloc (1, sizeof (cover_callback_t));
    if (!callback) {
        trace ("artwork callback alloc failed\n");
        cb (-1, info, NULL);
        return NULL;
    }

    callback->cb = cb;
    callback->info = info;
    callback->next = NULL;
    return callback;
}

static int
strings_equal (const char *s1, const char *s2)
{
    return (s1 == s2) || (s1 && s2 && !strcasecmp (s1, s2));
}

static int
queries_equal (ddb_cover_query_t *q1, ddb_cover_query_t *q2) {
    if (q1->track == q2->track) {
        return 1;
    }
    return 0;
}

static void
enqueue_query (ddb_cover_query_t *new_query, const ddb_cover_callback_t cb)
{
    for (cover_query_t *q = queue; q; q = q->next) {
        if (queries_equal (new_query, q->callbacks->info)) {
            // append top existing pending query
            cover_callback_t **last_callback = &q->callbacks;
            while (*last_callback && (*last_callback)->cb != cache_reset_callback) {
                last_callback = & (*last_callback)->next;
            }
            if (!*last_callback) {
                *last_callback = new_query_callback (cb, new_query);
                return;
            }
        }
    }

    // add new query
    cover_query_t *q = calloc (1, sizeof (cover_query_t));
    if (q) {
        q->callbacks = new_query_callback (cb, new_query);
    }

    if (!q) {
        if (cb) {
            cb (-1, new_query, NULL);
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

static int
vfs_scan_results (struct dirent *entry, const char *container_uri, ddb_cover_info_t *cover)
{
    /* VFS container, double check the match in case scandir didn't implement filtering */
    if (filter_custom (entry)) {
        trace ("found cover %s in %s\n", entry->d_name, container_uri);
        size_t len = strlen (container_uri) + strlen(entry->d_name) + 2;
        cover->filename = malloc (len);
        snprintf (cover->filename, len, "%s:%s", container_uri, entry->d_name);
        return 0;
    }

    return -1;
}

static int
dir_scan_results (struct dirent **files, int files_count, const char *container, ddb_cover_info_t *cover)
{
    /* Local file in a directory */
    for (size_t i = 0; i < files_count; i++) {
        trace ("found cover %s in local folder\n", files[0]->d_name);
        size_t len = strlen (container) + strlen(files[i]->d_name) + 2;
        cover->filename = malloc (len);
        snprintf (cover->filename, len, "%s/%s", container, files[i]->d_name);
        struct stat stat_struct;
        if (!stat (cover->filename, &stat_struct) && S_ISREG (stat_struct.st_mode) && stat_struct.st_size > 0) {
            return 0;
        }
        else {
            free (cover->filename);
            cover->filename = NULL;
        }
    }

    return -1;
}

static int
scan_local_path (char *mask, const char *local_path, const char *uri, DB_vfs_t *vfsplug, ddb_cover_info_t *cover)
{
    filter_custom_mask = mask;
    struct dirent **files;
    int (* custom_scandir)(const char *, struct dirent ***, int (*)(const struct dirent *), int (*)(const struct dirent **, const struct dirent **));
    custom_scandir = vfsplug ? vfsplug->scandir : scandir;
    int files_count = custom_scandir (local_path, &files, filter_custom, NULL);
    if (files_count > 0) {
        int err = -1;
        if (uri) {
            err = vfs_scan_results (files[0], uri, cover);
        }
        else {
            err = dir_scan_results (files, files_count, local_path, cover);
        }

        for (size_t i = 0; i < files_count; i++) {
            free (files[i]);
        }
        free (files);

        return err;
    }

    return -1;
}

static const char *filter_strcasecmp_name = NULL;

static int
filter_strcasecmp (const struct dirent *f)
{
    return !strcasecmp (filter_strcasecmp_name, f->d_name);
}

// FIXME: this returns only one path that matches subfolder. Usually that's enough, but can be improved.
static char *
get_case_insensitive_path (const char *local_path, const char *subfolder, DB_vfs_t *vfsplug) {
    filter_strcasecmp_name = subfolder;
    struct dirent **files;
    int (* custom_scandir)(const char *, struct dirent ***, int (*)(const struct dirent *), int (*)(const struct dirent **, const struct dirent **));
    custom_scandir = vfsplug ? vfsplug->scandir : scandir;
    int files_count = custom_scandir (local_path, &files, filter_strcasecmp, NULL);
    if (files_count > 0) {
        size_t l = strlen (local_path) + strlen (files[0]->d_name) + 2;
        char *ret = malloc (l);
        snprintf (ret, l, "%s/%s", local_path, files[0]->d_name);

        for (size_t i = 0; i < files_count; i++) {
            free (files[i]);
        }
        free (files);

        return ret;
    }
    return NULL;
}

static int
local_image_file (const char *local_path, const char *uri, DB_vfs_t *vfsplug, ddb_cover_info_t *cover)
{
    if (!artwork_filemask) {
        return -1;
    }

    char *p;

    char *filemask = strdup (artwork_filemask);
    strcpy (filemask, artwork_filemask);
    const char *filemask_end = filemask + strlen (filemask);
    while ((p = strrchr (filemask, ';'))) {
        *p = '\0';
    }

    char *folders = strdup (artwork_folders);
    strcpy (folders, artwork_folders);
    const char *folders_end = folders + strlen (folders);
    while ((p = strrchr (folders, ';'))) {
        *p = '\0';
    }

    int root = 1;
    char *folder = folders;
    while (folder < folders_end) {
        char *path;
        if (root) {
            root = 0;
            path = strdup (local_path);
        }
        else {
            path = get_case_insensitive_path (local_path, folder, vfsplug);
            folder += strlen (folder)+1;
        }
        trace ("scanning %s for artwork\n", path);
        for (char *mask = filemask; mask < filemask_end; mask += strlen (mask)+1) {
            if (mask[0] && !scan_local_path (mask, path, uri, vfsplug, cover)) {
                free (filemask);
                free (folders);
                free (path);
                return 0;
            }
        }
        if (!scan_local_path ("*.jpg", path, uri, vfsplug, cover)
            || !scan_local_path ("*.jpeg", path, uri, vfsplug, cover)
            || !scan_local_path ("*.png", path, uri, vfsplug, cover)) {
            free (filemask);
            free (folders);
            free (path);
            return 0;
        }
        free (path);
    }

    trace ("No cover art files in local folder\n");
    free (filemask);
    free (folders);
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
id3v2_artwork (const DB_id3v2_frame_t *f, int minor_version, int type)
{
    if ((minor_version > 2 && strcmp (f->id, "APIC")) || (minor_version == 2 && strcmp (f->id, "PIC"))) {
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
    const uint8_t *end = f->data + f->size;
    int enc = *data;
    data++;

    if (minor_version > 2) {
        // mime type is ascii always, the enc above is for the picture type
        const uint8_t *mime_end = id3v2_skip_str (0, data, end);
        if (!mime_end) {
            trace ("artwork: corrupted id3v2 APIC frame\n");
            return NULL;
        }
        if (type == -1 || *mime_end == type) {
            trace ("artwork: picture type=%d\n", *mime_end);
            return NULL;
        }
        trace ("artwork: mime-type=%s, picture type: %d\n", data, *mime_end);
        data = mime_end;
    }
    else {
        data += 3; // image format
    }
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

    size_t sz = end - ++data;
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

static FLAC__IOCallbacks flac_iocb = {
    .read = flac_io_read,
    .write = NULL,
    .seek = flac_io_seek,
    .tell = flac_io_tell,
    .eof = NULL,
    .close = NULL
};

static int
flac_extract_art (const char *filename, const char *outname, ddb_cover_info_t *cover) {
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
        if (!artwork_disable_cache) {
            if (!write_file (outname, (char *)pic->data, pic->data_length)) {
                cover->filename = strdup (outname);
                err = 0;
            }
        }
        else {
            cover->blob = malloc (pic->data_length);
            memcpy (cover->blob, pic->data, pic->data_length);
            cover->blob_size = pic->data_length;
            cover->blob_image_size = pic->data_length;
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
id3_extract_art (const char *fname, const char *outname, ddb_cover_info_t *cover) {
    int err = -1;

    DB_id3v2_tag_t id3v2_tag;
    memset (&id3v2_tag, 0, sizeof (id3v2_tag));
    DB_FILE *id3v2_fp = deadbeef->fopen (fname);
    if (id3v2_fp && !deadbeef->junk_id3v2_read_full (NULL, &id3v2_tag, id3v2_fp)) {
        int minor_version = id3v2_tag.version[0];
        DB_id3v2_frame_t *fprev = NULL;
        for (DB_id3v2_frame_t *f = id3v2_tag.frames; f; f = f->next) {
            const uint8_t *image_data = id3v2_artwork (f, minor_version, 3);
            if (!image_data) {
                // try any cover if front is not available
                image_data = id3v2_artwork (f, minor_version, 0);
            }
            if (image_data) {
                const size_t sz = f->size - (image_data - f->data);
                if (sz <= 0) {
                    continue;
                }
                if (!artwork_disable_cache) {
                    trace ("will write id3v2 APIC (%d bytes) into %s\n", (int)sz, outname);
                    if (!write_file (outname, (const char *)image_data, sz)) {
                        cover->filename = strdup(outname);
                        err = 0;
                        break;
                    }
                }
                else {
                    // steal the frame memory from DB_id3v2_tag_t
                    if (fprev) {
                        fprev->next = f->next;
                    }
                    else {
                        id3v2_tag.frames = f->next;
                    }

                    cover->blob = (char *)f;
                    cover->blob_size = f->size;
                    cover->blob_image_offset = (uint64_t)((char *)image_data - (char *)cover->blob);
                    cover->blob_image_size = sz;
                    err = 0;
                    break;
                }
            }
            fprev = f;
        }
    }
    deadbeef->junk_id3v2_free (&id3v2_tag);
    if (id3v2_fp) {
        deadbeef->fclose (id3v2_fp);
    }
    return err;
}

static int
apev2_extract_art (const char *fname, const char *outname, ddb_cover_info_t *cover) {
    int err = -1;
    DB_apev2_tag_t apev2_tag;
    memset (&apev2_tag, 0, sizeof (apev2_tag));
    DB_FILE *apev2_fp = deadbeef->fopen (fname);
    if (apev2_fp && !deadbeef->junk_apev2_read_full (NULL, &apev2_tag, apev2_fp)) {
        DB_apev2_frame_t *fprev = NULL;
        for (DB_apev2_frame_t *f = apev2_tag.frames; f; f = f->next) {
            const uint8_t *image_data = apev2_artwork (f);
            if (image_data) {
                const size_t sz = f->size - (image_data - f->data);
                if (sz <= 0) {
                    continue;
                }
                trace ("will write apev2 cover art (%d bytes) into %s\n", sz, outname);
                if (!artwork_disable_cache) {
                    if (!write_file (outname, (const char *)image_data, sz)) {
                        cover->filename = strdup (outname);
                        err = 0;
                        break;
                    }
                }
                else {
                    // steal the frame memory from DB_id3v2_tag_t
                    if (fprev) {
                        fprev->next = f->next;
                    }
                    else {
                        apev2_tag.frames = f->next;
                    }

                    cover->blob = (char *)f;
                    cover->blob_size = f->size;
                    cover->blob_image_offset = (uint64_t)((char *)image_data - (char *)cover->blob);
                    cover->blob_image_size = sz;
                    err = 0;
                    break;
                }
            }
            fprev = f;
        }

    }
    deadbeef->junk_apev2_free (&apev2_tag);
    if (apev2_fp) {
        deadbeef->fclose (apev2_fp);
    }
    return err;
}

#ifdef USE_MP4FF
static uint32_t
mp4_fp_read (void *user_data, void *buffer, uint32_t length) {
    DB_FILE* stream = user_data;
    uint32_t ret = (uint32_t)deadbeef->fread (buffer, 1, length, stream);
    return ret;
}

static uint32_t
mp4_fp_seek (void *user_data, uint64_t position) {
    DB_FILE* stream = user_data;
    return deadbeef->fseek (stream, (int64_t)position, SEEK_SET);
}

static int
mp4_extract_art (const char *fname, const char *outname, ddb_cover_info_t *cover) {
    int ret = -1;
    if (!strcasestr (fname, ".mp4") && !strcasestr (fname, ".m4a") && !strcasestr (fname, ".m4b")) {
        return -1;
    }

    DB_FILE* fp = deadbeef->fopen (fname);
    if (!fp) {
        return -1;
    }

    mp4ff_callback_t cb = {
        .read = mp4_fp_read,
        .write = NULL,
        .seek = mp4_fp_seek,
        .truncate = NULL,
        .user_data = fp
    };
    mp4ff_t *mp4 = mp4ff_open_read_coveronly (&cb);
    if (!mp4) {
        deadbeef->fclose (fp);
        return -1;
    }

    mp4ff_cover_art_t* art_list = mp4ff_cover_get (mp4);
    mp4ff_cover_art_item_t *f;
    mp4ff_cover_art_item_t *fprev = NULL;
    for (f = art_list->items; f; f = f->next) {
        if (!f->next) {
            break;
        }
        fprev = f;
    }
    if (f) {
        uint32_t sz = f->size;
        char* image_blob = f->data;
        trace ("will write mp4 cover art (%u bytes) into %s\n", sz, outname);
        if (!artwork_disable_cache) {
            if (!write_file (outname, image_blob, sz)) {
                ret = 0;
                cover->filename = strdup (outname);
            }
        }
        else {
            // steal the frame memory from mp4ff_cover_art_t
            art_list->tail = fprev;
            if (fprev) {
                fprev->next = NULL;
            }
            else {
                art_list->items = NULL;
            }
            cover->blob = (char *)image_blob;
            cover->blob_size = sz;
            cover->blob_image_size = sz;
            ret = 0;
        }
    }

    mp4ff_close (mp4);
    deadbeef->fclose (fp);
    return ret;
}
#endif

static int
web_lookups (const char *artist, const char *album, const char *cache_path, ddb_cover_info_t *cover)
{
    if (!cache_path) {
        return 0;
    }
#if USE_VFS_CURL
    if (artwork_enable_lfm) {
        if (!fetch_from_lastfm (artist, album, cache_path)) {
            cover->filename = strdup (cache_path);
            return 1;
        }
        if (errno == ECONNABORTED) {
            return 0;
        }
    }

    if (artwork_enable_mb) {
        if (!fetch_from_musicbrainz (artist, album, cache_path)) {
            cover->filename = strdup (cache_path);
            return 1;
        }
        if (errno == ECONNABORTED) {
            return 0;
        }
    }

    if (artwork_enable_aao) {
        if (!fetch_from_albumart_org (artist, album, cache_path)) {
            cover->filename = strdup (cache_path);
            return 1;
        }
        if (errno == ECONNABORTED) {
            return 0;
        }
    }
#endif

    return -1;
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

// might be used from process_query, when cache impl is finalized
#if 0
static int
path_more_recent (const char *fname, const time_t placeholder_mtime)
{
    struct stat stat_buf;
    return !stat (fname, &stat_buf) && stat_buf.st_mtime > placeholder_mtime;
}

// returns 1 if enough time passed since the last attemp to find the requested cover
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
#endif

// Behavior:
// Local cover: path is returned
// Found in cache: path is returned
// Embedded cover: !cache_disabled ? save_to_cash&return_path : return blob
// Web cover: save_to_local ? save_to_local&return_path : ( !cache_disabled ? save_to_cache&return_path : NOP )
static int
process_query (const char *filepath, const char *album, const char *artist, ddb_cover_info_t *cover)
{
    char cache_path_buf[PATH_MAX];
    char *cache_path = NULL;

    if (!artwork_disable_cache) {
        cache_path = cache_path_buf;
        make_cache_path (filepath, album, artist, cache_path, sizeof (cache_path_buf));
    }

#if 0
#warning FIXME not needed during development; also this assumes that disk cache is used for everything
    /* Flood control, don't retry missing artwork for an hour unless something changes */
    struct stat placeholder_stat;
    if (!stat (cache_path, &placeholder_stat) && placeholder_stat.st_mtime + 60*60 > time (NULL)) {
        int recheck = recheck_missing_artwork (filepath, placeholder_stat.st_mtime);
        if (!recheck) {
            return 0;
        }
    }
#endif

    int islocal = deadbeef->is_local_file (filepath);

    if (artwork_enable_embedded && islocal) {
#ifdef USE_METAFLAC
        // try to load embedded from flac metadata
        trace ("trying to load artwork from Flac tag for %s\n", filepath);
        if (!flac_extract_art (filepath, cache_path, cover)) {
            return 1;
        }
#endif

        // try to load embedded from id3v2
        trace ("trying to load artwork from id3v2 tag for %s\n", filepath);
        if (!id3_extract_art (filepath, cache_path, cover)) {
            return 1;
        }

        // try to load embedded from apev2
        trace ("trying to load artwork from apev2 tag for %s\n", filepath);
        if (!apev2_extract_art (filepath, cache_path, cover)) {
            return 1;
        }

#ifdef USE_MP4FF
        // try to load embedded from mp4
        trace ("trying to load artwork from mp4 tag for %s\n", filepath);
        if (!mp4_extract_art (filepath, cache_path, cover)) {
            return 1;
        }
#endif
    }

    if (artwork_enable_local && islocal) {
        char *fname_copy = strdup (filepath);
        if (fname_copy) {
            char *vfs_fname = vfs_path (fname_copy);
            if (vfs_fname) {
                /* Search inside scannable VFS containers */
                DB_vfs_t *plugin = scandir_plug (vfs_fname);
                if (plugin && !local_image_file (vfs_fname, fname_copy, plugin, cover)) {
                    free (fname_copy);
                    return 1;
                }
            }

            /* Search in file directory */
            if (!local_image_file (dirname (vfs_fname ? vfs_fname : fname_copy), NULL, NULL, cover)) {
                free (fname_copy);
                return 1;
            }

            free (fname_copy);
        }
    }

    if (!cache_path) {
        return 0;
    }

#ifdef USE_VFS_CURL
    /* Web lookups */
    if (artwork_enable_wos && strlen (filepath) > 3 && !strcasecmp (filepath+strlen (filepath)-3, ".ay")) {
        if (!fetch_from_wos (album, cache_path)) {
            cover->filename = strdup(cache_path);
            return 1;
        }
        if (errno == ECONNABORTED) {
            return 0;
        }
    }

    int res = web_lookups (artist, album, cache_path, cover);
    if (res >= 0) {
        return res;
    }

    if (album) {
        /* Try stripping parenthesised text off the end of the album name */
        char *p = strpbrk (album, "([");
        if (p) {
            *p = '\0';
            int res = web_lookups (artist, album, cache_path, cover);
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
send_query_callbacks (cover_callback_t *callback, ddb_cover_info_t *cover) {
    if (cover) {
        cover->refc = 0;
        cover_callback_t *c = callback;
        while (c) {
            cover->refc++;
            c = c->next;
        }
    }
    while (callback) {
        callback->cb (cover ? 0 : -1, callback->info, cover);
        cover_callback_t *next = callback->next;
        free (callback);
        callback = next;
    }
}

static cover_query_t *
query_pop (void) {
    cover_query_t *query = queue;
    queue = queue ? queue->next : NULL;
    if (!queue) {
        queue_tail = NULL;
    }
    return query;
}

static void
queue_clear (void) {
    while (queue) {
        cover_query_t *next = queue->next;
        send_query_callbacks (queue->callbacks, NULL);
        query_free (queue);
        queue = next;
    }
    queue_tail = NULL;
}

static void
cover_info_free (ddb_cover_info_t *cover) {
    cover->refc--;
    if (cover->refc != 0) {
        return;
    }
    if (cover->type) {
        free (cover->type);
    }
    if (cover->filename) {
        free (cover->filename);
    }
    if (cover->blob) {
        free (cover->blob);
    }
    free (cover);
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
        // FIXME: use deadbeef->cond_wait
        pthread_cond_wait ((pthread_cond_t *)queue_cond, (pthread_mutex_t *)queue_mutex);
        trace ("artwork fetcher: cond signalled, process queue\n");

        /* Loop until queue is empty */
        while (queue) {
            deadbeef->mutex_unlock (queue_mutex);

            ddb_cover_query_t *info = queue->callbacks->info;

            /* Process this query, hopefully writing a file into cache */
            ddb_cover_info_t *cover = calloc (sizeof (ddb_cover_info_t), 1);

            if (!album_tf) {
                album_tf = deadbeef->tf_compile ("%album%");
            }
            if (!artist_tf) {
                artist_tf = deadbeef->tf_compile ("%artist%");
            }

            deadbeef->pl_lock ();
            char *filepath = strdupa (deadbeef->pl_find_meta (info->track, ":URI"));
            deadbeef->pl_unlock ();

            char album[1000];
            char artist[1000];
            ddb_tf_context_t ctx;
            ctx._size = sizeof (ddb_tf_context_t);
            ctx.it = info->track;
            deadbeef->tf_eval (&ctx, album_tf, album, sizeof (album));
            deadbeef->tf_eval (&ctx, artist_tf, artist, sizeof (artist));
            
            int cover_found = process_query (filepath, album, artist, cover);

            deadbeef->mutex_lock (queue_mutex);
            cover_query_t *query = query_pop ();
            deadbeef->mutex_unlock (queue_mutex);

            if (!query) {
                break;
            }

            /* Make all the callbacks (and free the chain), with data if a file was written */
            if (cover_found) {
                trace ("artwork fetcher: cover art file found: %s\n", cover->filename);
                send_query_callbacks (query->callbacks, cover);
            }
            else {
                trace ("artwork fetcher: no cover art found\n");
                send_query_callbacks (query->callbacks, NULL);
            }
            query_free (query);

            /* Look for what to do next */
            deadbeef->mutex_lock (queue_mutex);
        }
    }
    deadbeef->mutex_unlock (queue_mutex);
    trace ("artwork fetcher: terminate thread\n");
}

static void
cover_get (ddb_cover_query_t *query, ddb_cover_callback_t callback) {
    deadbeef->mutex_lock (queue_mutex);
    enqueue_query (query, callback);
    deadbeef->mutex_unlock (queue_mutex);
}

static void
artwork_reset (void) {
    trace ("artwork: reset queue\n");
    deadbeef->mutex_lock (queue_mutex);
    queue_clear ();
    deadbeef->mutex_unlock (queue_mutex);
}

static void
get_fetcher_preferences (void)
{
    artwork_disable_cache = deadbeef->conf_get_int ("artwork.disable_cache", DEFAULT_DISABLE_CACHE);
    artwork_save_to_music_folders = deadbeef->conf_get_int ("artwork.save_to_music_folders", DEFAULT_SAVE_TO_MUSIC_FOLDERS);

    artwork_enable_embedded = deadbeef->conf_get_int ("artwork.enable_embedded", 1);
    artwork_enable_local = deadbeef->conf_get_int ("artwork.enable_localfolder", 1);
    if (artwork_enable_local) {
        deadbeef->conf_lock ();
        const char *new_artwork_filemask = deadbeef->conf_get_str_fast ("artwork.filemask", NULL);
        if (!new_artwork_filemask || !new_artwork_filemask[0]) {
            new_artwork_filemask = DEFAULT_FILEMASK;
        }
        if (!strings_equal (artwork_filemask, new_artwork_filemask)) {
            char *old_artwork_filemask = artwork_filemask;
            artwork_filemask = strdup (new_artwork_filemask);
            if (old_artwork_filemask) {
                free (old_artwork_filemask);
            }
        }

        const char *new_artwork_folders = deadbeef->conf_get_str_fast ("artwork.folders", NULL);
        if (!new_artwork_folders || !new_artwork_folders[0]) {
            new_artwork_folders = DEFAULT_FOLDERS;
        }
        if (!strings_equal (artwork_folders, new_artwork_folders)) {
            char *old_artwork_folders = artwork_folders;
            artwork_folders = strdup (new_artwork_folders);
            if (old_artwork_folders) {
                free (old_artwork_folders);
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
        if (!strings_equal (new_nocover_path, nocover_path)) {
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
artwork_configchanged (void)
{
    cache_configchanged ();

    int old_artwork_disable_cache = artwork_disable_cache;

    int old_artwork_enable_embedded = artwork_enable_embedded;
    int old_artwork_enable_local = artwork_enable_local;
    char *old_artwork_filemask = strdup(artwork_filemask ? artwork_filemask : "");
    char *old_artwork_folders = strdup(artwork_folders ? artwork_folders : "");
#ifdef USE_VFS_CURL
    int old_artwork_enable_lfm = artwork_enable_lfm;
    int old_artwork_enable_mb = artwork_enable_mb;
    int old_artwork_enable_aao = artwork_enable_aao;
    int old_artwork_enable_wos = artwork_enable_wos;
#endif
    int old_missing_artwork = missing_artwork;
    const char *old_nocover_path = nocover_path;
//    int old_scale_towards_longer = scale_towards_longer;

    get_fetcher_preferences ();

    if (old_artwork_disable_cache != artwork_disable_cache || old_missing_artwork != missing_artwork || old_nocover_path != nocover_path) {
        trace ("artwork config changed, invalidating default artwork...\n");
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
        strcmp(old_artwork_filemask, artwork_filemask) ||
        strcmp(old_artwork_folders, artwork_folders)
        ) {
        trace ("artwork config changed, invalidating cache...\n");
        deadbeef->mutex_lock (queue_mutex);

        // Submit a query for NULL image, with a callback that would reset the cache,
        // on the correct thread.
        ddb_cover_query_t *q = calloc (sizeof (ddb_cover_query_t), 1);
        q->user_data = &cache_reset_time;
        enqueue_query (q, cache_reset_callback);

        artwork_abort_http_request ();
        deadbeef->mutex_unlock (queue_mutex);
    }
    free (old_artwork_filemask);
    free (old_artwork_folders);
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
invalidate_playitem_cache (DB_plugin_action_t *action, int ctx)
{
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    if (!plt)
        return -1;

    DB_playItem_t *it = deadbeef->plt_get_first (plt, PL_MAIN);
    while (it) {
        if (deadbeef->pl_is_selected (it)) {
            deadbeef->pl_lock ();
            const char *url = strdupa (deadbeef->pl_find_meta (it, ":URI"));
            deadbeef->pl_unlock ();

            char album[1000];
            char artist[1000];
            ddb_tf_context_t ctx;
            ctx._size = sizeof (ddb_tf_context_t);
            ctx.it = it;
            deadbeef->tf_eval (&ctx, album_tf, album, sizeof (album));
            deadbeef->tf_eval (&ctx, artist_tf, artist, sizeof (artist));
            char cache_path[PATH_MAX];
            if (!make_cache_path (url, album, artist, cache_path, PATH_MAX)) {
                char subdir_path[PATH_MAX];
                make_cache_dir_path (artist, subdir_path, PATH_MAX);
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
    if (artwork_folders) {
        free (artwork_folders);
    }

    if (album_tf) {
        deadbeef->tf_free (album_tf);
        album_tf = NULL;
    }
    if (artist_tf) {
        deadbeef->tf_free (artist_tf);
        artist_tf = NULL;
    }

    stop_cache_cleaner ();

    return 0;
}

static int
artwork_plugin_start (void)
{
    get_fetcher_preferences ();
    cache_reset_time = deadbeef->conf_get_int64 ("artwork.cache_reset_time", 0);

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

#define STR(x) #x

static const char settings_dlg[] =
// on android, cache is always off and music is saved to music folders by default
#ifndef ANDROID
    "property \"Disable disk cache\" checkbox artwork.disable_cache 0;\n"
    "property \"Save extracted covers to music folders\" checkbox artwork.save_to_music_folders 0;\n"
#endif
    "property \"Fetch from embedded tags\" checkbox artwork.enable_embedded 1;\n"
    "property \"Fetch from local folder\" checkbox artwork.enable_localfolder 1;\n"
    "property \"Local file mask\" entry artwork.filemask \"" DEFAULT_FILEMASK "\";\n"
    "property \"Artwork folders\" entry artwork.folders \"" DEFAULT_FOLDERS "\";\n"
#ifdef USE_VFS_CURL
    "property \"Fetch from Last.fm\" checkbox artwork.enable_lastfm 0;\n"
    "property \"Fetch from MusicBrainz\" checkbox artwork.enable_musicbrainz 0;\n"
    "property \"Fetch from Albumart.org\" checkbox artwork.enable_albumartorg 0;\n"
    "property \"Fetch from World of Spectrum (AY files only)\" checkbox artwork.enable_wos 0;\n"
#endif
// android doesn't display any image when cover is not found, and positioning algorithm is really different,
// therefore the below options are useless
#ifndef ANDROID
    "property box vbox[2] spacing=4 border=8;\n"
    "property box hbox[1] height=-1;"
    "property \"When no artwork is found\" select[3] artwork.missing_artwork 1 \"leave blank\" \"use DeaDBeeF default cover\" \"display custom image\";"
    "property \"Custom image path\" file artwork.nocover_path \"\";\n"
    "property \"Scale artwork towards longer side\" checkbox artwork.scale_towards_longer 1;\n"
#endif
;

// define plugin interface
static ddb_artwork_plugin_t plugin = {
    .plugin.plugin.api_vmajor = DB_API_VERSION_MAJOR,
    .plugin.plugin.api_vminor = DB_API_VERSION_MINOR,
    .plugin.plugin.version_major = DDB_ARTWORK_MAJOR_VERSION,
    .plugin.plugin.version_minor = DDB_ARTWORK_MINOR_VERSION,
    .plugin.plugin.type = DB_PLUGIN_MISC,
    .plugin.plugin.id = "artwork2",
    .plugin.plugin.name = "Album Artwork",
    .plugin.plugin.descr = "Loads album artwork from embedded tags, local directories, or internet services",
    .plugin.plugin.copyright =
        "Album Art plugin for DeaDBeeF\n"
        "Copyright (C) 2009-2011 Viktor Semykin <thesame.ml@gmail.com>\n"
        "Copyright (C) 2009-2016 Alexey Yakovenko <waker@users.sourceforge.net>\n"
        "Copyright (C) 2014-2016 Ian Nartowicz <deadbeef@nartowicz.co.uk>\n"
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
    .cover_get = cover_get,
    .reset = artwork_reset,
    .cover_info_free = cover_info_free,
};

DB_plugin_t *
artwork_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}
