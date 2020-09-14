//
//  artwork_jpeg.c
//  artwork
//
//  Created by Alexey Yakovenko on 9/14/20.
//  Copyright © 2020 Alexey Yakovenko. All rights reserved.
//

#include "artwork_jpeg.h"
#ifdef USE_LIBJPEG
#include <jpeglib.h>
#endif

#ifdef USE_LIBJPEG
typedef struct {
    struct jpeg_error_mgr pub;    /* "public" fields */
    jmp_buf setjmp_buffer;    /* for return to caller */
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
