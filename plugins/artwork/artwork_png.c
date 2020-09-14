//
//  artwork_png.c
//  artwork
//
//  Created by Alexey Yakovenko on 9/14/20.
//  Copyright © 2020 Alexey Yakovenko. All rights reserved.
//

#include "artwork_png.h"
#ifdef USE_LIBPNG
#include <png.h>
#endif

#ifdef USE_LIBPNG
int
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
