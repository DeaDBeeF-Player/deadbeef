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
#  include "../../config.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <libgen.h>
#include <unistd.h>
#include <inttypes.h>
#if HAVE_SYS_CDEFS_H
#include <sys/cdefs.h>
#endif
#if HAVE_SYS_SYSLIMITS_H
#include <sys/syslimits.h>
#endif
#include "../../deadbeef.h"
#include "file_utils.h"
#include "artwork.h"

#ifdef USE_IMLIB2
#include <Imlib2.h>
static uintptr_t imlib_mutex;
#else
#include <jpeglib.h>
#include <jerror.h>
//#include <setjmp.h>
#include <png.h>
#endif

#define min(x,y) ((x)<(y)?(x):(y))

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(...)

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
}
#endif

static int
check_dir (const char *dir, mode_t mode)
{
    char *tmp = strdup (dir);
    char *slash = tmp;
    struct stat stat_buf;
    do
    {
        slash = strstr (slash+1, "/");
        if (slash)
            *slash = 0;
        if (-1 == stat (tmp, &stat_buf))
        {
            trace ("creating dir %s\n", tmp);
            if (0 != mkdir (tmp, mode))
            {
                trace ("Failed to create %s (%d)\n", tmp, errno);
                free (tmp);
                return 0;
            }
        }
        if (slash)
            *slash = '/';
    } while (slash);
    free (tmp);
    return 1;
}

int ensure_dir(const char *path)
{
    char dir[PATH_MAX];
    strcpy(dir, path);
    dirname(dir);
    trace("ensure folder %s exists\n", dir);
    if (!check_dir(dir, 0755)) {
        trace ("failed to create folder %s\n", dir);
        return 0;
    }
    return 1;
}

int scale_file (const char *in, const char *out, int img_size)
{
    trace ("scaling %s to %s\n", in, out);

    if (img_size < 1 || img_size > 32767) {
        trace ("%d is not a valid scaled image size\n", img_size);
        return -1;
    }

    if (!ensure_dir(out)) {
        return -1;
    }

#ifdef USE_IMLIB2
    deadbeef->mutex_lock (imlib_mutex);
    const int imlib_err = imlib_resize(in, out, img_size);
    deadbeef->mutex_unlock (imlib_mutex);
    return imlib_err;
#else
    int res = jpeg_resize (in, out, img_size);
    if (res != 0) {
        unlink (out);
        res = png_resize (in, out, img_size);
        if (res != 0) {
            unlink (out);
        }
    }
    return res;
#endif
}

#define BUFFER_SIZE 4096
int copy_file (const char *in, const char *out)
{
    trace ("copying %s to %s\n", in, out);

    char *buf = malloc(BUFFER_SIZE);
    if (!buf) {
        trace("artwork: failed to alloc %d bytes\n", BUFFER_SIZE);
        return -1;
    }

    if (!ensure_dir(out)) {
        return -1;
    }

    char tmp_out[PATH_MAX];
    snprintf(tmp_out, PATH_MAX, "%s.part", out);

    DB_FILE *fin = deadbeef->fopen (in);
    if (!fin) {
        trace("artwork: failed to open file %s for reading\n", in);
        return -1;
    }
    FILE *fout = fopen (tmp_out, "w+b");
    if (!fout) {
        deadbeef->fclose(fin);
        trace("artwork: failed to open file %s for writing\n", tmp_out);
        return -1;
    }

    int bytes_read;
    do {
        bytes_read = deadbeef->fread(buf, 1, BUFFER_SIZE, fin);
        if (bytes_read < 0) {
            trace("artwork: failed to read file %s\n", tmp_out);
            break;
        }
        if (bytes_read > 0 && fwrite(buf, bytes_read, 1, fout) != 1) {
            trace("artwork: failed to write file %s\n", tmp_out);
            bytes_read = -1;
            break;
        }
    } while (bytes_read == BUFFER_SIZE);

    deadbeef->fclose(fin);
    fclose(fout);
    free(buf);

    if (bytes_read > 0) {
        const int err = rename(tmp_out, out);
        if (err != 0) {
            trace("artwork: rename error %d: failed to move %s to %s: %s\n", err, tmp_out, out, strerror(err));
        }
    }

    unlink(tmp_out);
    return 0;
}

int write_file(const char *out, const char *data, const size_t data_length)
{
    if (!ensure_dir(out)) {
        return -1;
    }

    char tmp_path[1024];
    snprintf(tmp_path, sizeof(tmp_path), "%s.part", out);

    FILE *fp = fopen(tmp_path, "w+b");
    if (!fp) {
        trace ("artwork: failed to open %s for writing\n", tmp_path);
        return -1;
    }

    if (fwrite(data, 1, data_length, fp) != data_length) {
        trace ("artwork: failed to write picture into %s\n", tmp_path);
        fclose(fp);
        unlink(tmp_path);
        return -1;
    }

    fclose (fp);
    const int err = rename (tmp_path, out);
    unlink (tmp_path);

    if (err != 0) {
        trace ("Failed to move %s to %s: %s\n", tmp_path, out, strerror (err));
        return -1;
    }

    return 0;
}
