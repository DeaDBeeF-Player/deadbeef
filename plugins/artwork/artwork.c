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
#include <errno.h>
#include <dirent.h>
#include <unistd.h>
#include <fnmatch.h>
#include <pthread.h>
#include <sys/stat.h>
#ifdef __linux__
    #include <sys/prctl.h>
#endif
#if HAVE_SYS_CDEFS_H
    #include <sys/cdefs.h>
#endif
#if HAVE_SYS_SYSLIMITS_H
    #include <sys/syslimits.h>
#endif
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
#include "albumartorg.h"
#include "wos.h"
#include "cache.h"
#include "artwork.h"

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(...)

DB_functions_t *deadbeef;
DB_FILE *current_file;

static DB_artwork_plugin_t plugin;
static char default_cover[PATH_MAX];

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
    cover_callback_t callback;
    struct cover_query_s *next;
} cover_query_t;

static cover_query_t *queue;
static cover_query_t *queue_tail;
static int terminate;
static int clear_queue;
static intptr_t tid;
static uintptr_t queue_mutex;
static uintptr_t queue_cond;
#ifdef USE_IMLIB2
    static uintptr_t imlib_mutex;
#endif

static int artwork_enable_embedded;
static int artwork_enable_local;
#ifdef USE_VFS_CURL
    static int artwork_enable_lfm;
    static int artwork_enable_aao;
    static int artwork_enable_wos;
#endif
static int scale_towards_longer;
static time_t artwork_reset_time;
static time_t artwork_scaled_reset_time;

#define DEFAULT_FILEMASK "*cover*.jpg;*front*.jpg;*folder*.jpg;*cover*.png;*front*.png;*folder*.png"
#define MAX_FILEMASK_LENGTH 200
static char artwork_filemask[MAX_FILEMASK_LENGTH];

static float
scale_dimensions(const int scaled_size, const int width, const int height, unsigned int *scaled_width, unsigned int *scaled_height)
{
    /* Calculate the dimensions of the scaled image */
    float scaling_ratio;
    if (deadbeef->conf_get_int ("artwork.scale_towards_longer", 1) == width > height) {
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
static float cerp(const float p0, const float p1, const float p2, const float p3, const float d, const float d2, const float d3)
{
    /* Cubic Hermite spline (a = -0.5, Catmull-Rom) */
    return p1 - (p0*d3 - 3*p1*d3 + 3*p2*d3 - p3*d3 - 2*p0*d2 + 5*p1*d2 - 4*p2*d2 + p3*d2 + p0*d - p2*d)/2;
}

static int_fast16_t bcerp(const png_byte *row0, const png_byte *row1, const png_byte *row2, const png_byte *row3,
                      const uint_fast32_t x0, const uint_fast32_t x1, const uint_fast32_t x2, const uint_fast32_t x3, const uint_fast8_t component,
                      const float dx, const float dx2, const float dx3, const float dy, const float dy2, const float dy3) {
    const uint_fast32_t index0 = x0 + component;
    const uint_fast32_t index1 = x1 + component;
    const uint_fast32_t index2 = x2 + component;
    const uint_fast32_t index3 = x3 + component;
    const float p0 = cerp(row0[index0], row1[index0], row2[index0], row3[index0], dy, dy2, dy3);
    const float p1 = cerp(row0[index1], row1[index1], row2[index1], row3[index1], dy, dy2, dy3);
    const float p2 = cerp(row0[index2], row1[index2], row2[index2], row3[index2], dy, dy2, dy3);
    const float p3 = cerp(row0[index3], row1[index3], row2[index3], row3[index3], dy, dy2, dy3);
    return cerp(p0, p1, p2, p3, dx, dx2, dx3) + 0.5;
}
#endif
static uint_fast32_t blerp_pixel(const png_byte *row, const png_byte *next_row, const uint_fast32_t x_index, const uint_fast32_t next_x_index,
                                 const uint_fast32_t weight, const uint_fast32_t weightx, const uint_fast32_t weighty, const uint_fast32_t weightxy)
{
    return row[x_index]*weight + row[next_x_index]*weightx + next_row[x_index]*weighty + next_row[next_x_index]*weightxy;
}

static uint_fast32_t *
calculate_quick_dividers(const float scaling_ratio)
{
    /* Replace slow divisions by expected numbers with faster multiply/shift combo */
    const uint16_t max_sample_size = scaling_ratio + 1;
    uint_fast32_t *dividers = malloc((max_sample_size * max_sample_size + 1) * sizeof(uint_fast32_t));
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

METHODDEF(void)
my_error_exit (j_common_ptr cinfo)
{
  /* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
  my_error_mgr_t *myerr = (my_error_mgr_t *) cinfo->err;

//  (*cinfo->err->output_message) (cinfo);

  /* Return control to the setjmp point */
  longjmp(myerr->setjmp_buffer, 1);
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
    cinfo_out.err = cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = my_error_exit;
    if (setjmp(jerr.setjmp_buffer)) {
        trace("failed to scale %s as jpeg\n", outname);
        jpeg_destroy_decompress(&cinfo);
        jpeg_destroy_compress(&cinfo_out);
        if (sample_rows_buffer) {
            free(sample_rows_buffer);
        }
        if (quick_dividers) {
            free(quick_dividers);
        }
        if (fp) {
            fclose(fp);
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
    jpeg_create_compress(&cinfo_out);

    jpeg_stdio_src (&cinfo, fp);
    jpeg_stdio_dest(&cinfo_out, out);

    jpeg_read_header (&cinfo, TRUE);
    jpeg_start_decompress (&cinfo);

    const unsigned int num_components = cinfo.output_components;
    const unsigned int width = cinfo.image_width;
    const unsigned int height = cinfo.image_height;
    unsigned int scaled_width, scaled_height;
    const float scaling_ratio = scale_dimensions(scaled_size, width, height, &scaled_width, &scaled_height);
    if (scaling_ratio >= 65535 || scaled_width < 1 || scaled_width > 32767 || scaled_height < 1 || scaled_width > 32767) {
        trace("scaling ratio (%g) or scaled image dimensions (%ux%u) are invalid\n", scaling_ratio, scaled_width, scaled_height);
        my_error_exit((j_common_ptr)&cinfo);
    }

    cinfo_out.image_width      = scaled_width;
    cinfo_out.image_height     = scaled_height;
    cinfo_out.input_components = num_components;
    cinfo_out.in_color_space   = cinfo.out_color_space;
    jpeg_set_defaults(&cinfo_out);
    jpeg_set_quality(&cinfo_out, 95, TRUE);
    jpeg_start_compress(&cinfo_out, TRUE);

    const uint_fast32_t row_components = width * num_components;
    const uint_fast32_t scaled_row_components = scaled_width * num_components;
    JSAMPLE out_line[scaled_row_components];
    JSAMPROW out_row = out_line;

    if (scaling_ratio > 2) {
        /* Simple (unweighted) area sampling for large downscales */
        quick_dividers = calculate_quick_dividers(scaling_ratio);
        const uint16_t max_sample_height = scaling_ratio + 1;
        sample_rows_buffer = malloc(max_sample_height * row_components * sizeof(JSAMPLE));
        if (!sample_rows_buffer || !quick_dividers) {
            my_error_exit((j_common_ptr)&cinfo);
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
                jpeg_read_scanlines(&cinfo, &rows[row_index], 1);
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

            jpeg_write_scanlines(&cinfo_out, &out_row, 1);
        }

        free(sample_rows_buffer);
        free(quick_dividers);
    }
    else {
#ifndef USE_BICUBIC
        /* Bilinear interpolation for upscales and modest downscales */
        JSAMPLE scanline[row_components];
        JSAMPLE next_scanline[row_components];
        JSAMPROW row = scanline;
        JSAMPROW next_row = next_scanline;
        const float downscale_offset = scaling_ratio < 1 ? 0 : (scaling_ratio - 1) / 2;
        float y_interp = downscale_offset;
        for (uint_fast16_t scaled_y = 0; scaled_y < scaled_height; scaled_y++, y_interp+=scaling_ratio) {
            const uint_fast32_t y = y_interp;
            const uint_fast16_t y_diff = (uint_fast32_t)(y_interp*256) - (y<<8);
            const uint_fast16_t y_remn = 256 - y_diff;

            if (cinfo.output_scanline < y+2) {
                while (cinfo.output_scanline < y+1) {
                    jpeg_read_scanlines(&cinfo, &next_row, 1);
                }
                memcpy(row, next_row, row_components*sizeof(JSAMPLE));
                if (y+2 < height) {
                    jpeg_read_scanlines(&cinfo, &next_row, 1);
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
                    out_line[scaled_x + component] = blerp_pixel(row, next_row, x_index+component, next_x_index+component, weight, weightx, weighty, weightxy) >> 16;
                }
            }

            jpeg_write_scanlines(&cinfo_out, &out_row, 1);
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
        jpeg_read_scanlines(&cinfo, &row1, 1);
        memcpy(row2, row1, row_components*sizeof(JSAMPLE));

        const float downscale_offset = scaling_ratio < 1 ? 0 : (scaling_ratio - 1) / 2;
        float y_interp = downscale_offset;
        for (uint_fast16_t scaled_y = 0; scaled_y < scaled_height; scaled_y++, y_interp+=scaling_ratio) {
            const uint_fast32_t y = y_interp;
            const float dy = y_interp - (int_fast32_t)y_interp;
            const float dy2 = dy * dy;
            const float dy3 = dy2 * dy;

            if (cinfo.output_scanline < y+3) {
                while (cinfo.output_scanline < y) {
                    jpeg_read_scanlines(&cinfo, &row1, 1);
                }
                memcpy(row0, row1, row_components*sizeof(JSAMPLE));
                if (cinfo.output_scanline < y+1) {
                    jpeg_read_scanlines(&cinfo, &row2, 1);
                }
                memcpy(row1, row2, row_components*sizeof(JSAMPLE));
                if (y+2 < height && cinfo.output_scanline < y+2) {
                    jpeg_read_scanlines(&cinfo, &row3, 1);
                }
                memcpy(row2, row3, row_components*sizeof(JSAMPLE));
                if (y+3 < height) {
                    jpeg_read_scanlines(&cinfo, &row3, 1);
                }
            }

            float x_interp = downscale_offset;
            for (uint_fast32_t scaled_x = 0; scaled_x < scaled_row_components; scaled_x+=num_components, x_interp+=scaling_ratio) {
                const uint_fast32_t x = x_interp;
                const uint_fast32_t x1 = x * num_components;
                const uint_fast32_t x0 = x > 0 ? x1-num_components : x1;
                const uint_fast32_t x2 = x+1 < width ? x1+num_components : x1;
                const uint_fast32_t x3 = x+2 < width ? x2+num_components : x2;

                const float dx = x_interp - (int_fast32_t)x_interp;
                const float dx2 = dx * dx;
                const float dx3 = dx2 * dx;

                for (uint_fast8_t component=0; component<num_components; component++) {
                    const int_fast16_t pixel = bcerp(row0, row1, row2, row3, x0, x1, x2, x3, component, dx, dx2, dx3, dy, dy2, dy3);
                    out_row[scaled_x + component] = pixel < 0 ? 0 : pixel > 255 ? 255 : pixel;
                }
            }

            jpeg_write_scanlines(&cinfo_out, &out_row, 1);
        }
#endif
    }

    jpeg_finish_compress(&cinfo_out);

    jpeg_destroy_compress(&cinfo_out);
    jpeg_destroy_decompress(&cinfo);

    fclose(fp);
    fclose(out);

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

    fp = fopen(fname, "rb");
    if (!fp) {
        trace("failed to open %s for reading\n", fname);
        goto error;
    }

    png_ptr = png_create_read_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) {
        trace("failed to create PNG read struct\n");
        goto error;
    }

    if (setjmp(png_jmpbuf((png_ptr))))
    {
        trace("failed to read %s as png\n", fname);
        goto error;
    }

    png_init_io(png_ptr, fp);

    info_ptr = png_create_info_struct (png_ptr);
    if (!info_ptr) {
        trace("failed to create PNG info struct\n");
        goto error;
    }

    png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_STRIP_16|PNG_TRANSFORM_PACKING|PNG_TRANSFORM_EXPAND, NULL);
    png_bytep *row_pointers = png_get_rows(png_ptr, info_ptr);
    png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, NULL, NULL, NULL);

    unsigned int scaled_width, scaled_height;
    const float scaling_ratio = scale_dimensions(scaled_size, width, height, &scaled_width, &scaled_height);
    if (scaling_ratio >= 65535 || scaled_width < 1 || scaled_width > 32767 || scaled_height < 1 || scaled_width > 32767) {
        trace("scaling ratio (%g) or scaled image dimensions (%ux%u) are invalid\n", scaling_ratio, scaled_width, scaled_height);
        goto error;
    }

    out = fopen (outname, "w+b");
    if (!out) {
        trace("failed to open %s for writing\n", outname);
        goto error;
    }

    new_png_ptr = png_create_write_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!new_png_ptr) {
        trace("failed to create png write struct\n");
        goto error;
    }

    if (setjmp(png_jmpbuf((new_png_ptr))))
    {
        trace("failed to write %s as png\n", outname);
        goto error;
    }

    png_init_io(new_png_ptr, out);

    new_info_ptr = png_create_info_struct (new_png_ptr);
    if (!new_info_ptr) {
        trace("failed to create png info struct for writing\n");
        goto error;
    }

    png_set_IHDR(new_png_ptr, new_info_ptr, scaled_width, scaled_height, bit_depth, color_type, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_write_info(new_png_ptr, new_info_ptr);
    png_set_packing(new_png_ptr);

    const uint8_t has_alpha = color_type & PNG_COLOR_MASK_ALPHA;
    const uint8_t num_values = color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA ? 1 : 3;
    const uint8_t num_components = num_values + (has_alpha ? 1 : 0);
    const uint_fast32_t scaled_row_components = scaled_width * num_components;
    out_row = malloc(scaled_row_components * sizeof(png_byte));
    if (!out_row) {
        goto error;
    }

    if (scaling_ratio > 2) {
        /* Simple (unweighted) area sampling for large downscales */
        quick_dividers = calculate_quick_dividers(scaling_ratio);
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

            png_write_row(new_png_ptr, out_row);
        }
    }
    else {
#ifndef USE_BICUBIC
        /* Bilinear interpolation for upscales and modest downscales */
        const float downscale_offset = scaling_ratio < 1 ? 0 : (scaling_ratio - 1) / 2;
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
                        out_row[scaled_x + component] = blerp_pixel(row, next_row, x_index+component, next_x_index+component, weight, weightx, weighty, weightxy) >> 16;
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
                        out_row[scaled_x + component] = blerp_pixel(row, next_row, x_index+component, next_x_index+component, alpha, alphax, alphay, alphaxy) / scaled_alpha;
                    }
                }
            }

            png_write_row(new_png_ptr, out_row);
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

        const float downscale_offset = scaling_ratio < 1 ? 0 : (scaling_ratio - 1) / 2;
        int_fast16_t scaled_alpha = 255;
        float y_interp = downscale_offset;
        for (uint_fast16_t scaled_y = 0; scaled_y < scaled_height; scaled_y++, y_interp+=scaling_ratio) {
            const uint_fast32_t y = y_interp;
            const png_byte *row1 = row_pointers[y];
            const png_byte *row0 = y > 0 ? row_pointers[y-1] : row1;
            const png_byte *row2 = y+1 < height ? row_pointers[y+1] : row1;
            const png_byte *row3 = y+2 < height ? row_pointers[y+2] : row2;

            const float dy = y_interp - (int_fast32_t)y_interp;
            const float dy2 = dy * dy;
            const float dy3 = dy2 * dy;

            float x_interp = downscale_offset;
            for (uint_fast32_t scaled_x = 0; scaled_x < scaled_row_components; scaled_x+=num_components, x_interp+=scaling_ratio) {
                const uint_fast32_t x = x_interp;
                const uint_fast32_t x1 = x * num_components;
                const uint_fast32_t x0 = x > 0 ? x1-num_components : x1;
                const uint_fast32_t x2 = x+1 < width ? x1+num_components : x1;
                const uint_fast32_t x3 = x+2 < width ? x2+num_components : x2;

                const float dx = x_interp - (int_fast32_t)x_interp;
                const float dx2 = dx * dx;
                const float dx3 = dx2 * dx;

                if (has_alpha) {
                    scaled_alpha = bcerp(row0, row1, row2, row3, x0, x1, x2, x3, 3, dx, dx2, dx3, dy, dy2, dy3);
                    out_row[scaled_x + 3] = scaled_alpha < 0 ? 0 : scaled_alpha > 255 ? 255 : scaled_alpha;
                }

                if (scaled_alpha == 255) {
                    for (uint_fast8_t component=0; component<3; component++) {
                        const int_fast16_t pixel = bcerp(row0, row1, row2, row3, x0, x1, x2, x3, component, dx, dx2, dx3, dy, dy2, dy3);
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
                        const int_fast16_t pixel = (bcerp(row0, row1, row2, row3, x0, x1, x2, x3, component, dx, dx2, dx3, dy, dy2, dy3)<<8) / scaled_alpha;
                        out_row[scaled_x + component] = pixel < 0 ? 0 : pixel > 255 ? 255 : pixel;
                    }
                }
            }

            png_write_row(new_png_ptr, out_row);
        }
#endif
    }

    png_write_end(new_png_ptr, new_info_ptr);

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
        png_destroy_write_struct(&new_png_ptr, &new_info_ptr);
    }
    if (out_row) {
        free(out_row);
    }
    if (quick_dividers) {
        free(quick_dividers);
    }

    return err;
}

#endif

#ifdef USE_IMLIB2
static int
imlib_resize(const char *in, const char *out, int img_size)
{
    Imlib_Image img = imlib_load_image_immediately (in);
    if (!img) {
        trace ("file %s not found, or imlib2 can't load it\n", in);
        return -1;
    }
    imlib_context_set_image(img);

    int w = imlib_image_get_width ();
    int h = imlib_image_get_height ();
    int sw, sh;
    scale_dimensions(img_size, w, h, &sw, &sh);
    if (sw < 1 || sw > 32767 || sh < 1 || sh > 32767) {
        trace ("%d/%d scaled image is too large\n", sw, sh);
        imlib_free_image ();
        return -1;
    }

    int is_jpeg = imlib_image_format() && imlib_image_format()[0] == 'j';
    Imlib_Image scaled = imlib_create_cropped_scaled_image(0, 0, w, h, sw, sh);
    if (!scaled) {
        trace ("imlib2 can't create scaled image\n", in);
        imlib_free_image ();
        return -1;
    }
    imlib_context_set_image(scaled);

    imlib_image_set_format(is_jpeg ? "jpg" : "png");
    if (is_jpeg)
        imlib_image_attach_data_value("quality", NULL, 95, NULL);
    Imlib_Load_Error err = 0;
    imlib_save_image_with_error_return(out, &err);
    if (err != 0) {
        trace ("imlib save %s returned %d\n", out, err);
        imlib_free_image ();
        imlib_context_set_image(img);
        imlib_free_image ();
        return -1;
    }

    imlib_free_image ();
    imlib_context_set_image(img);
    imlib_free_image ();
    return 0;
}
#endif

static int
scale_file (const char *in, const char *out, int img_size)
{
    trace("artwork: scaling %s to %s\n", in, out);

    if (img_size < 1 || img_size > 32767) {
        trace ("%d is not a valid scaled image size\n", img_size);
        return -1;
    }

    if (!ensure_dir(out)) {
        return -1;
    }

    cache_lock();
#ifdef USE_IMLIB2
    deadbeef->mutex_lock(imlib_mutex);
    const int imlib_err = imlib_resize(in, out, img_size);
    deadbeef->mutex_unlock(imlib_mutex);
    cache_unlock();
    return imlib_err;
#else
    int err = jpeg_resize(in, out, img_size);
    if (err != 0) {
        unlink(out);
        err = png_resize(in, out, img_size);
        if (err != 0) {
            unlink (out);
        }
    }
    cache_unlock();
    return err;
#endif
}

static char
esc_char (char c) {
    if (c < 1
        || (c >= 'a' && c <= 'z')
        || (c >= 'A' && c <= 'Z')
        || (c >= '0' && c <= '9')
        || c == ' '
        || c == '_'
        || c == '-') {
        return c;
    }
    return '_';
}

static int
make_cache_dir_path (char *path, const int size, const char *artist, const int img_size) {
    char esc_artist[NAME_MAX+1];
    if (artist) {
        size_t i = 0;
        while (artist[i] && i < NAME_MAX) {
            esc_artist[i] = esc_char(artist[i]);
            i++;
        }
        esc_artist[i] = '\0';
    }
    else {
        strcpy(esc_artist, "Unknown artist");
    }

    if (make_cache_root_path(path, size) < 0) {
        return -1;
    }

    const size_t size_left = size - strlen(path);
    int path_length;
    if (img_size == -1) {
        path_length = snprintf(path+strlen(path), size_left, "covers/%s/", esc_artist);
    }
    else {
        path_length = snprintf(path+strlen(path), size_left, "covers-%d/%s/", img_size, esc_artist);
    }
    if (path_length >= size_left) {
        trace("Cache path truncated at %d bytes\n", size);
        return -1;
    }

    return 0;
}

static int
make_cache_path2 (char *path, const int size, const char *fname, const char *album, const char *artist, const int img_size) {
    path[0] = '\0';

    if (!album || !*album) {
        if (fname) {
            album = fname;
        }
        else if (artist && *artist) {
            album = artist;
        }
        else {
            trace("not possible to get any unique album name\n");
            return -1;
        }
    }
    if (!artist || !*artist) {
        artist = "Unknown artist";
    }

    if (make_cache_dir_path(path, size-NAME_MAX, artist, img_size)) {
        return -1;
    }

    const int max_album_chars = min(NAME_MAX, size - strlen(path)) - sizeof("1.jpg.part");
    if (max_album_chars <= 0) {
        trace("Path buffer not long enough for %s and filename\n", path);
        return -1;
    }

    char esc_album[max_album_chars+1];
    const char *palbum = strlen(album) > max_album_chars ? album+strlen(album)-max_album_chars : album;
    size_t i = 0;
    do {
        esc_album[i] = esc_char(palbum[i]);
    } while (palbum[i++]);

    sprintf(path+strlen(path), "%s%s", esc_album, ".jpg");
    return 0;
}

static void
make_cache_path (char *path, int size, const char *album, const char *artist, int img_size) {
    make_cache_path2 (path, size, NULL, album, artist, img_size);
}

static const char *
get_default_cover (void) {
    return default_cover;
}

static void
query_clear(cover_query_t *query)
{
    if (query->fname) {
        free(query->fname);
    }
    if (query->artist) {
        free(query->artist);
    }
    if (query->album) {
        free(query->album);
    }
    free(query);
}

static int
params_match(const char *s1, const char *s2)
{
    return s1 == s2 || s1 && s2 && !strcasecmp(s1, s2);
}

static void
query_add(const char *fname, const char *artist, const char *album, const int img_size, const artwork_callback callback, void *user_data)
{
    for (cover_query_t *q = queue; q; q = q->next) {
        if (params_match(artist, q->artist) && params_match(album, q->album) && q->size == img_size) {
            trace("artwork queue: already in queue - add to callbacks\n");
            if (callback) {
                cover_callback_t *extra_callback = malloc(sizeof(cover_callback_t));
                if (extra_callback) {
                    extra_callback->cb = callback;
                    extra_callback->ud = user_data;
                    extra_callback->next = NULL;
                    cover_callback_t *last_callback = &q->callback;
                    while (last_callback->next) {
                        last_callback = last_callback->next;
                    }
                    last_callback->next = extra_callback;
                }
                else {
                    callback(NULL, NULL, NULL, user_data);
                }
            }
            return;
        }
    }

    trace("artwork:query_add %s %s %s %d\n", fname, artist, album, img_size);
    cover_query_t *q = malloc(sizeof(cover_query_t));
    if (q) {
        q->fname = fname && *fname ? strdup(fname) : NULL;
        q->artist = artist ? strdup(artist) : NULL;
        q->album = album ? strdup(album) : NULL;
        q->size = img_size;
        q->next = NULL;
        q->callback.cb = callback;
        q->callback.ud = user_data;
        q->callback.next = NULL;

        if (!q->fname || artist && !q->artist || album && !q->album) {
            query_clear(q);
            q = NULL;
        }
    }

    if (!q) {
        if (callback) {
            callback(NULL, NULL, NULL, user_data);
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
    deadbeef->cond_signal(queue_cond);
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
    return !fnmatch(filter_custom_mask, f->d_name, FNM_CASEFOLD);
}

static char *test_mask(char *mask, const char *filename_dir)
{
    char *artwork_path = NULL;
    filter_custom_mask = mask;
    struct dirent **files;
    const int files_count = scandir(filename_dir, &files, filter_custom, NULL);
    if (files_count >= 0) {
        for (size_t i = 0; i < files_count; i++) {
            trace("found cover %s in local folder\n", files[0]->d_name);
            if (!artwork_path) {
                artwork_path = malloc(strlen(filename_dir) + 1 + strlen(files[i]->d_name) + 1);
                if (artwork_path) {
                    sprintf(artwork_path, "%s/%s", filename_dir, files[i]->d_name);
                    struct stat stat_struct;
                    if (stat(artwork_path, &stat_struct) || !S_ISREG(stat_struct.st_mode) || stat_struct.st_size == 0) {
                        free(artwork_path);
                        artwork_path = NULL;
                    }
                }
            }
            free(files[i]);
        }
        free(files);
    }

    return artwork_path;
}

static char *local_image_file(const char *directory, int (* scandir)(const char *dir, struct dirent ***namelist, int (*selector)(const struct dirent *), int (*cmp)(const struct dirent **, const struct dirent **)))
{
    trace("scanning %s for artwork\n", directory);
    char filemask[MAX_FILEMASK_LENGTH];
    strcpy(filemask, artwork_filemask);
    const char *filemask_end = filemask + strlen(filemask);
    char *p;
    while (p = strrchr(filemask, ';')) {
        *p = '\0';
    }

    char *artwork_path;
    for (char *mask = filemask; mask < filemask_end; mask += strlen(mask)+1) {
        if (mask[0] && (artwork_path = test_mask(mask, directory))) {
            return artwork_path;
        }
    }
    if ((artwork_path = test_mask("*.jpg", directory)) || (artwork_path = test_mask("*.jpeg", directory))) {
        return artwork_path;
    }

    trace("No cover art files in local folder\n");
    return NULL;
}

static const uint8_t *
id3v2_skip_str (const int enc, const uint8_t *ptr, const uint8_t *end) {
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
id3v2_artwork(const DB_id3v2_frame_t *f, const int minor_version)
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
//    if (strcasecmp(data, "image/jpeg") &&
#ifdef USE_IMLIB2
//        strcasecmp(data, "image/gif") &&
//        strcasecmp(data, "image/tiff") &&
#endif
//        strcasecmp(data, "image/png")) {
//        trace ("artwork: unsupported mime type: %s\n", data);
//        return NULL;
//    }
    if (*mime_end != 3) {
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
apev2_artwork(const DB_apev2_frame_t *f)
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

    const int sz = end - ++data;
    if (sz < 20) {
        trace ("artwork: apev2 cover art frame is too small\n");
        return NULL;
    }

//    uint8_t *ext = strrchr (f->data, '.');
//    if (!ext || !*++ext) {
//        trace ("artwork: apev2 cover art name has no extension\n");
//        return NULL;
//    }

//    if (strcasecmp(ext, "jpeg") &&
//        strcasecmp(ext, "jpg") &&
#ifdef USE_IMLIB2
//        strcasecmp(ext, "gif") &&
//        strcasecmp(ext, "tif") &&
//        strcasecmp(ext, "tiff") &&
#endif
//        strcasecmp(ext, "png")) {
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

static FLAC__StreamMetadata_Picture *
flac_extract_art(FLAC__Metadata_Chain *chain, const char *filename)
{
    DB_FILE *file = deadbeef->fopen (filename);
    if (!file) {
        trace ("artwork: failed to open %s\n", filename);
        return NULL;
    }

    FLAC__IOCallbacks iocb = {
        .read = flac_io_read,
        .write = NULL,
        .seek = flac_io_seek,
        .tell = flac_io_tell,
        .eof = NULL,
        .close = NULL
    };
    int res = FLAC__metadata_chain_read_with_callbacks(chain, (FLAC__IOHandle)file, iocb);
#if USE_OGG
    if (!res && FLAC__metadata_chain_status(chain) == FLAC__METADATA_SIMPLE_ITERATOR_STATUS_NOT_A_FLAC_FILE) {
        res = FLAC__metadata_chain_read_ogg_with_callbacks(chain, (FLAC__IOHandle)file, iocb);
    }
#endif
    deadbeef->fclose (file);
    if (!res) {
        trace ("artwork: failed to read metadata from flac: %s\n", filename);
        return NULL;
    }

    FLAC__StreamMetadata *picture = 0;
    FLAC__Metadata_Iterator *iterator = FLAC__metadata_iterator_new();
    if (!iterator) {
        return NULL;
    }
    FLAC__metadata_iterator_init(iterator, chain);
    do {
        FLAC__StreamMetadata *block = FLAC__metadata_iterator_get_block(iterator);
        if (block->type == FLAC__METADATA_TYPE_PICTURE) {
            picture = block;
        }
    } while(FLAC__metadata_iterator_next(iterator) && 0 == picture);
    FLAC__metadata_iterator_delete(iterator);
    if (!picture) {
        trace ("%s doesn't have an embedded cover\n", filename);
        return NULL;
    }

    return &picture->data.picture;
}
#endif

static int
process_scaled_query(const cover_query_t *query)
{
    char cache_path[PATH_MAX];
    make_cache_path2(cache_path, sizeof(cache_path), query->fname, query->album, query->artist, -1);

    struct stat stat_buf;
    if (!stat(cache_path, &stat_buf) && S_ISREG(stat_buf.st_mode) && stat_buf.st_size > 0) {
        char scaled_path[PATH_MAX];
        make_cache_path2(scaled_path, sizeof(scaled_path), query->fname, query->album, query->artist, query->size);
        trace("artwork: scaling %s into %s (%d pixels)\n", cache_path, scaled_path, query->size);
        if (*scaled_path && !scale_file(cache_path, scaled_path, query->size)) {
            return 1;
        }
    }

    return 0;
}

static int
process_query(const cover_query_t *query)
{
    char cache_path[PATH_MAX];
    make_cache_path2(cache_path, sizeof(cache_path), query->fname, query->album, query->artist, -1);
    trace("artwork: query cover for %s %s to %s\n", query->album, query->artist, cache_path);

    struct stat placeholder_stat;
    int flood_control = !stat(cache_path, &placeholder_stat) && placeholder_stat.st_size == 0 && placeholder_stat.st_mtime + 60*10 > time(NULL);
    int looked_for_pic = 0;

    if (deadbeef->is_local_file(query->fname) && artwork_enable_embedded) {
        if (flood_control) {
            /* Override flood control if the track file has changed */
            struct stat fname_stat;
            if (!stat(query->fname, &fname_stat) && fname_stat.st_mtime > placeholder_stat.st_mtime) {
                flood_control = 0;
            }
        }

        if (!flood_control) {
            looked_for_pic = 1;

            // try to load embedded from id3v2
            trace("trying to load artwork from id3v2 tag for %s\n", query->fname);
            DB_id3v2_tag_t id3v2_tag;
            memset(&id3v2_tag, 0, sizeof(id3v2_tag));
            DB_FILE *id3v2_fp = deadbeef->fopen(query->fname);
            if (id3v2_fp && !deadbeef->junk_id3v2_read_full(NULL, &id3v2_tag, id3v2_fp)) {
                const int minor_version = id3v2_tag.version[0];
                for (DB_id3v2_frame_t *f = id3v2_tag.frames; f; f = f->next) {
                    const uint8_t *image_data = id3v2_artwork(f, minor_version);
                    if (image_data) {
                        const size_t sz = f->size - (image_data - f->data);
                        trace("will write id3v2 APIC (%d bytes) into %s\n", sz, cache_path);
                        if (sz > 0 && !write_file(cache_path, image_data, sz)) {
                            return 1;
                        }
                    }
                }

                deadbeef->junk_id3v2_free(&id3v2_tag);
                deadbeef->fclose(id3v2_fp);
            }

            // try to load embedded from apev2
            trace("trying to load artwork from apev2 tag for %s\n", query->fname);
            DB_apev2_tag_t apev2_tag;
            memset(&apev2_tag, 0, sizeof(apev2_tag));
            DB_FILE *apev2_fp = deadbeef->fopen(query->fname);
            if (apev2_fp && !deadbeef->junk_apev2_read_full(NULL, &apev2_tag, apev2_fp)) {
                for (DB_apev2_frame_t *f = apev2_tag.frames; f; f = f->next) {
                    const uint8_t *image_data = apev2_artwork(f);
                    if (image_data) {
                        const size_t sz = f->size - (image_data - f->data);
                        trace("will write apev2 cover art (%d bytes) into %s\n", sz, cache_path);
                        if (sz > 0 && !write_file(cache_path, image_data, sz)) {
                            return 1;
                        }
                        break;
                    }
                }

                deadbeef->junk_apev2_free(&apev2_tag);
                deadbeef->fclose(apev2_fp);
            }

#ifdef USE_METAFLAC
            // try to load embedded from flac metadata
            trace("trying to load artwork from Flac tag for %s\n", query->fname);
            FLAC__Metadata_Chain *chain = FLAC__metadata_chain_new();
            if (chain) {
                FLAC__StreamMetadata_Picture *pic = flac_extract_art(chain, query->fname);
                if (pic && pic->data_length > 0) {
                    trace("found flac cover art of %d bytes (%s)\n", pic->data_length, pic->description);
                    trace("will write flac cover art into %s\n", cache_path);
                    if (!write_file(cache_path, pic->data, pic->data_length)) {
                        return 1;
                    }
                }
                FLAC__metadata_chain_delete(chain);
            }
#endif
        }
    }

    if (artwork_enable_local) {
        char *fname_copy = strdup(query->fname);
        if (fname_copy) {
            /* Find the directory for whatever sort of URL is provided */
            char *filename_dir = NULL;
            if (fname_copy[0] != '/' && strstr(fname_copy, "file://") != fname_copy) {
                char *p = strstr(fname_copy, "://");
                if (p) {
                    p += 3;
                    char *q = strrchr(p, ':');
                    if (q) {
                        *q = '\0';
                    }
                    filename_dir = dirname(p);
//            DB_vfs_t **vfsplugs = deadbeef->plug_get_vfs_list();
//            for (size_t i = 0; vfsplugs[i]; i++) {
//                if (vfsplugs[i]->is_container && vfsplugs[i]->is_container(fname_copy)) {
//                    fprintf(stderr, "%s is container\n", query->fname);
//                }
//            }
                }
                else {
                    filename_dir = dirname(fname_copy);
                }
            }
            else {
                filename_dir = dirname(fname_copy);
            }

            if (flood_control) {
                /* Override flood control if the directory contents have changed */
                struct stat dir_stat;
                if (!stat(filename_dir, &dir_stat) && dir_stat.st_mtime > placeholder_stat.st_mtime) {
                    flood_control = 0;
                }
            }

            if (!flood_control) {
                /* Searching in track directory */
                looked_for_pic = 1;
                char *artwork = local_image_file(filename_dir, scandir);
                if (artwork) {
                    copy_file(artwork, cache_path);
                    free(artwork);
                    free(fname_copy);
                    return 1;
                }
            }

            free(fname_copy);
        }
    }

#ifdef USE_VFS_CURL
    if (!flood_control) {
        /* Web lookups */
        if (artwork_enable_wos && strlen(query->fname) > 3 && !strcasecmp(query->fname+strlen(query->fname)-3, ".ay")) {
            looked_for_pic = 1;
            if (!fetch_from_wos(query->album, cache_path)) {
                return 1;
            }
        }

        if (artwork_enable_lfm) {
            looked_for_pic = 1;
            if (!fetch_from_lastfm(query->artist, query->album, cache_path)) {
                return 1;
            }
        }

        if (artwork_enable_aao) {
            looked_for_pic = 1;
            if (!fetch_from_albumart_org(query->artist, query->album, cache_path)) {
                return 1;
            }
        }

        if ((artwork_enable_lfm || artwork_enable_aao) && query->album) {
            /* Try stripping parenthesis off the end of the album name */
            const size_t plain_album_length = strcspn(query->album, "(");
            if (plain_album_length > 0 && plain_album_length < strlen(query->album)) {
                char *plain_album = strdup(query->album);
                if (plain_album) {
                    plain_album[plain_album_length] = '\0';
                    if (artwork_enable_lfm && !fetch_from_lastfm(query->artist, plain_album, cache_path)) {
                        return 1;
                    }
                    if (artwork_enable_aao && !fetch_from_albumart_org(query->artist, plain_album, cache_path)) {
                        return 1;
                    }
                    free(plain_album);
                }
            }
        }
    }
#endif

    if (looked_for_pic) {
        /* Touch placeholder */
        write_file(cache_path, NULL, 0);
    }

    return 0;
}

static void
query_complete(const char *fname, const char *artist, const char *album)
{
    cover_query_t *query = queue;
    queue = query->next;
    if (!queue) {
        queue_tail = NULL;
    }

    deadbeef->mutex_unlock(queue_mutex);
    cover_callback_t *callback = &query->callback;
    do {
        if (callback->cb) {
            trace("artwork: making callback with data %s %s %s %p\n", fname, artist, album, callback->ud);
            callback->cb(fname, artist, album, callback->ud);
        }
        callback = callback->next;
    } while (callback);
    query_clear(query);
    deadbeef->mutex_lock(queue_mutex);
}

static void
fetcher_thread (void *none)
{
#ifdef __linux__
    prctl (PR_SET_NAME, "deadbeef-artwork", 0, 0, 0, 0);
#endif

    /* Loop until external terminate command */
    deadbeef->mutex_lock(queue_mutex);
    while (!terminate) {
        trace("artwork fetcher: waiting for signal ...\n");
        pthread_cond_wait((pthread_cond_t *)queue_cond, (pthread_mutex_t *)queue_mutex);

        /* Loop until queue is empty or external command received */
        while (!terminate && !clear_queue && queue) {
            trace("artwork fetcher: cond signalled, process queue\n");
            deadbeef->mutex_unlock(queue_mutex);
            const int cached_art = queue->size == -1 ? process_query(queue) : process_scaled_query(queue);
            deadbeef->mutex_lock(queue_mutex);
            if (cached_art) {
                trace("artwork fetcher: cover art file cached\n");
                query_complete(queue->fname, queue->artist, queue->album);
            }
            else {
                trace("artwork fetcher: no cover art found\n");
                query_complete(NULL, NULL, NULL);
            }
        }

        /* External reset */
        if (clear_queue) {
            trace("artwork fetcher: received queue clear request\n");
            while (queue) {
                query_complete(NULL, NULL, NULL);
            }
            clear_queue = 0;
            deadbeef->cond_signal(queue_cond);
            trace("artwork fetcher: queue clear done\n");
        }
    }
    deadbeef->mutex_unlock(queue_mutex);
    trace("artwork fetcher: terminate thread\n");
}

static const char *
find_image (const char *path, const int scaled) {
    struct stat stat_buf;
    if (stat(path, &stat_buf) || !S_ISREG(stat_buf.st_mode)) {
        return NULL;
    }

    const time_t reset_time = scaled ? artwork_scaled_reset_time : artwork_reset_time;
    if (stat_buf.st_mtime < reset_time) {
        trace("artwork: deleting cached file %s after reset\n", path);
        unlink(path);
        return NULL;
    }

    if (stat_buf.st_size == 0) {
        return NULL;
    }

    return path;
}

static char *
get_album_art (const char *fname, const char *artist, const char *album, int size, artwork_callback callback, void *user_data)
{
    /* Check if the image is already cached */
    char cache_path[PATH_MAX];
    make_cache_path2(cache_path, sizeof(cache_path), fname, album, artist, size);
    const char *p = find_image(cache_path, size == -1 ? 0 : 1);
    if (p) {
        if (callback) {
            callback(NULL, NULL, NULL, user_data);
        }
        trace("Found cached image %s\n", cache_path);
        return strdup(p);
    }

    /* See if we need to make an unscaled image before we make a scaled one */
    deadbeef->mutex_lock(queue_mutex);
    if (size != -1) {
        char unscaled_path[PATH_MAX];
        make_cache_path2(unscaled_path, sizeof(unscaled_path), fname, album, artist, -1);
        if (!find_image(unscaled_path, 0)) {
            query_add(fname, artist, album, -1, NULL, NULL);
        }
    }

    /* Request to fetch the image */
    query_add(fname, artist, album, size, callback, user_data);
    deadbeef->mutex_unlock(queue_mutex);
    return NULL;
}
#if 0
static void
sync_callback (const char *fname, const char *artist, const char *album, void *user_data) {
    mutex_cond_t *mc = (mutex_cond_t *)user_data;
    deadbeef->mutex_lock (mc->mutex);
    deadbeef->cond_signal (mc->cond);
    deadbeef->mutex_unlock (mc->mutex);
}

static char *
get_album_art_sync (const char *fname, const char *artist, const char *album, int size) {
    mutex_cond_t mc;
    mc.mutex = deadbeef->mutex_create ();
    mc.cond = deadbeef->cond_create ();
    deadbeef->mutex_lock (mc.mutex);
    char *image_fname = get_album_art (fname, artist, album, size, sync_callback, &mc);
    while (!image_fname) {
        deadbeef->cond_wait (mc.cond, mc.mutex);
        image_fname = get_album_art (fname, artist, album, size, sync_callback, &mc);
    }
    deadbeef->mutex_unlock (mc.mutex);
    deadbeef->mutex_free (mc.mutex);
    deadbeef->cond_free (mc.cond);
    return image_fname;
}
#endif
static void
artwork_reset (int fast) {
    deadbeef->mutex_lock (queue_mutex);
    if (fast) {
        /* Assume the fetcher already has the first query, nuke the rest */
        trace ("artwork: fast reset\n");
        while (queue && queue->next) {
            cover_query_t *query = queue->next;
            queue->next = queue->next->next;
            query_clear(query);
        }
        queue_tail = queue;
    }
    else {
        trace ("artwork: full reset\n");
        clear_queue = 1;
        deadbeef->cond_signal(queue_cond);
        trace ("artwork: waiting for clear to complete ...\n");
        pthread_cond_wait((pthread_cond_t *)queue_cond, (pthread_mutex_t *)queue_mutex);
    }
    deadbeef->mutex_unlock (queue_mutex);
}

static void
artwork_configchanged (void) {
    cache_configchanged();
    int new_artwork_enable_embedded = deadbeef->conf_get_int ("artwork.enable_embedded", 1);
    int new_artwork_enable_local = deadbeef->conf_get_int ("artwork.enable_localfolder", 1);
#ifdef USE_VFS_CURL
    int new_artwork_enable_lfm = deadbeef->conf_get_int ("artwork.enable_lastfm", 0);
    int new_artwork_enable_aao = deadbeef->conf_get_int ("artwork.enable_albumartorg", 0);
    int new_artwork_enable_wos = deadbeef->conf_get_int ("artwork.enable_wos", 0);
#endif

    char new_artwork_filemask[MAX_FILEMASK_LENGTH];
    deadbeef->conf_get_str ("artwork.filemask", DEFAULT_FILEMASK, new_artwork_filemask, MAX_FILEMASK_LENGTH);
    if (!*new_artwork_filemask) {
        strcpy(new_artwork_filemask, DEFAULT_FILEMASK);
        deadbeef->conf_set_str("artwork.filemask", DEFAULT_FILEMASK);
    }

    if (new_artwork_enable_embedded != artwork_enable_embedded
            || new_artwork_enable_local != artwork_enable_local
#ifdef USE_VFS_CURL
            || new_artwork_enable_lfm != artwork_enable_lfm
            || new_artwork_enable_aao != artwork_enable_aao
            || new_artwork_enable_wos != artwork_enable_wos
#endif
            || strcmp (new_artwork_filemask, artwork_filemask)
            || deadbeef->conf_get_int ("artwork.refresh_now", 0)) {
        trace ("artwork config changed, invalidating cache...\n");
        artwork_enable_embedded = new_artwork_enable_embedded;
        artwork_enable_local = new_artwork_enable_local;
#ifdef USE_VFS_CURL
        artwork_enable_lfm = new_artwork_enable_lfm;
        artwork_enable_aao = new_artwork_enable_aao;
        artwork_enable_wos = new_artwork_enable_wos;
#endif
        artwork_reset_time = artwork_scaled_reset_time = time (NULL);
        deadbeef->conf_set_int64 ("artwork.cache_reset_time", artwork_reset_time);
        deadbeef->conf_set_int64 ("artwork.scaled.cache_reset_time", artwork_scaled_reset_time);
        strcpy (artwork_filemask, new_artwork_filemask);
        artwork_reset (0);
        deadbeef->sendmessage (DB_EV_PLAYLIST_REFRESH, 0, 0, 0);
        deadbeef->conf_set_int("artwork.refresh_now", 0);
    }

    int new_scale_towards_longer = deadbeef->conf_get_int ("artwork.scale_towards_longer", 1);
    if (new_scale_towards_longer != scale_towards_longer) {
        trace ("artwork config changed, invalidating scaled cache...\n");
        artwork_scaled_reset_time = time (NULL);
        deadbeef->conf_set_int64 ("artwork.scaled.cache_reset_time", artwork_scaled_reset_time);
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
invalidate_playitem_cache(DB_plugin_action_t *action, const int ctx)
{
    ddb_playlist_t *plt = deadbeef->plt_get_curr();
    if (!plt)
        return -1;

    DB_playItem_t *it = deadbeef->plt_get_first(plt, PL_MAIN);
    while (it) {
        if (deadbeef->pl_is_selected(it)) {
            deadbeef->pl_lock();
            const char *url = deadbeef->pl_find_meta(it, ":URI");
            const char *artist = deadbeef->pl_find_meta(it, "artist");
            const char *album = deadbeef->pl_find_meta(it, "album");
            const char *title = album ? album : deadbeef->pl_find_meta(it, "title");
            char cache_path[PATH_MAX];
            if (!make_cache_path2(cache_path, PATH_MAX, url, title, artist, -1)) {
                char subdir_path[PATH_MAX];
                make_cache_dir_path(subdir_path, PATH_MAX, artist, -1);
                const char *subdir_name = basename(subdir_path);
                const char *entry_name = basename(cache_path);
                trace("Expire %s from cache\n", cache_path);
                remove_cache_item(cache_path, subdir_path, subdir_name, entry_name);
            }
            deadbeef->pl_unlock();
        }
        deadbeef->pl_item_unref(it);
        it = deadbeef->pl_get_next(it, PL_MAIN);
    }

    deadbeef->plt_unref(plt);
    deadbeef->sendmessage (DB_EV_PLAYLIST_REFRESH, 0, 0, 0);
    return 0;
}

static DB_plugin_action_t *
artwork_get_actions(DB_playItem_t *it)
{
    if (!it) // Only currently show for the playitem context menu
        return NULL;

    trace("artwork_get_actions: checking context menu\n");
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
        deadbeef->mutex_lock (queue_mutex);
        clear_queue = 1;
        terminate = 1;
        deadbeef->cond_signal(queue_cond);
        deadbeef->mutex_unlock(queue_mutex);
        if (current_file) {
            deadbeef->fabort (current_file);
        }
        deadbeef->thread_join (tid);
        tid = 0;
    }
    if (queue_mutex) {
        deadbeef->mutex_free(queue_mutex);
        queue_mutex = 0;
    }
    if (queue_cond) {
        deadbeef->cond_free(queue_cond);
        queue_cond = 0;
    }

    stop_cache_cleaner();

#ifdef USE_IMLIB2
    if (imlib_mutex) {
        deadbeef->mutex_free (imlib_mutex);
        imlib_mutex = 0;
    }
#endif

    return 0;
}

static int
artwork_plugin_start (void)
{
    deadbeef->conf_get_str ("gtkui.nocover_pixmap", "", default_cover, sizeof(default_cover));
    if (!default_cover[0]) {
        snprintf (default_cover, sizeof (default_cover), "%s/noartwork.png", deadbeef->get_pixmap_dir ());
    }
    deadbeef->conf_get_str ("artwork.filemask", DEFAULT_FILEMASK, artwork_filemask, MAX_FILEMASK_LENGTH);
    artwork_enable_embedded = deadbeef->conf_get_int ("artwork.enable_embedded", 1);
    artwork_enable_local = deadbeef->conf_get_int ("artwork.enable_localfolder", 1);
#ifdef USE_VFS_CURL
    artwork_enable_lfm = deadbeef->conf_get_int ("artwork.enable_lastfm", 0);
    artwork_enable_aao = deadbeef->conf_get_int ("artwork.enable_albumartorg", 0);
    artwork_enable_wos = deadbeef->conf_get_int ("artwork.enable_wos", 0);
#endif
    artwork_reset_time = deadbeef->conf_get_int64 ("artwork.cache_reset_time", 0);
    artwork_scaled_reset_time = deadbeef->conf_get_int64 ("artwork.scaled.cache_reset_time", 0);

#ifdef USE_IMLIB2
    imlib_mutex = deadbeef->mutex_create_nonrecursive ();
    if (!imlib_mutex) {
        return -1;
    }
    imlib_set_cache_size(0);
#endif

    terminate = 0;
    queue_mutex = deadbeef->mutex_create_nonrecursive();
    queue_cond = deadbeef->cond_create();
    if (queue_mutex && queue_cond) {
        tid = deadbeef->thread_start_low_priority(fetcher_thread, NULL);
    }
    if (!tid) {
        artwork_plugin_stop();
        return -1;
    }

    start_cache_cleaner();

    return 0;
}

static const char settings_dlg[] =
    "property \"Cache update period (hr)\" entry artwork.cache.period 48;\n"
    "property \"Fetch from embedded tags\" checkbox artwork.enable_embedded 1;\n"
    "property \"Fetch from local folder\" checkbox artwork.enable_localfolder 1;\n"
    "property \"Local cover file mask\" entry artwork.filemask \"" DEFAULT_FILEMASK "\";\n"
#ifdef USE_VFS_CURL
    "property \"Fetch from last.fm\" checkbox artwork.enable_lastfm 0;\n"
    "property \"Fetch from albumart.org\" checkbox artwork.enable_albumartorg 0;\n"
    "property \"Fetch from worldofspectrum.org (AY only)\" checkbox artwork.enable_wos 0;\n"
#endif
    "property \"Scale artwork towards longer side\" checkbox artwork.scale_towards_longer 1;\n"
    "property \"Refresh cached artwork\" checkbox artwork.refresh_now 0;\n"
;

// define plugin interface
static DB_artwork_plugin_t plugin = {
    .plugin.plugin.api_vmajor = 1,
    .plugin.plugin.api_vminor = 0,
    .plugin.plugin.version_major = 1,
    .plugin.plugin.version_minor = 2,
    .plugin.plugin.type = DB_PLUGIN_MISC,
    .plugin.plugin.id = "artwork",
    .plugin.plugin.name = "Album Artwork",
    .plugin.plugin.descr = "Loads album artwork either from local directories or from internet",
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
