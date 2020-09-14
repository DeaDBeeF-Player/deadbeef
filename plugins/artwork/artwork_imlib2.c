//
//  artwork_imlib2.c
//  artwork
//
//  Created by Alexey Yakovenko on 9/14/20.
//  Copyright © 2020 Alexey Yakovenko. All rights reserved.
//

#include "artwork_imlib2.h"
#ifdef USE_IMLIB2
#include <Imlib2.h>
#endif

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
    cache_unlock ();
    return err;
#endif
}
#endif
