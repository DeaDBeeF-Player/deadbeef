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
#ifdef __linux__
#include <sys/prctl.h>
#endif
#include <errno.h>
#include <dirent.h>
#include <unistd.h>
#include <fnmatch.h>
#include <inttypes.h>
#if HAVE_SYS_CDEFS_H
#include <sys/cdefs.h>
#endif
#if HAVE_SYS_SYSLIMITS_H
#include <sys/syslimits.h>
#endif
#include "../../deadbeef.h"
#include "artwork.h"
#ifdef USE_VFS_CURL
#include "lastfm.h"
#include "albumartorg.h"
#include "wos.h"
#endif

#ifdef USE_IMLIB2
#include <Imlib2.h>
static uintptr_t imlib_mutex;
#else
#include <jpeglib.h>
#include <jerror.h>
//#include <setjmp.h>
#include <png.h>
#endif

#ifdef USE_METAFLAC
#include <FLAC/metadata.h>
#endif

#include "../../strdupa.h"

#define min(x,y) ((x)<(y)?(x):(y))

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(...)

static char default_cover[PATH_MAX];
#define DEFAULT_FILEMASK "*cover*.jpg;*front*.jpg;*folder*.jpg;*cover*.png;*front*.png;*folder*.png"

static DB_artwork_plugin_t plugin;
DB_functions_t *deadbeef;

DB_FILE *current_file;

#define MAX_CALLBACKS 200

typedef struct cover_callback_s {
    artwork_callback cb;
    void *ud;
} cover_callback_t;

typedef struct cover_query_s {
    char *fname;
    char *artist;
    char *album;
    int size;
    cover_callback_t callbacks[MAX_CALLBACKS];
    int numcb;
    struct cover_query_s *next;
} cover_query_t;

typedef struct mutex_cond_s {
    uintptr_t mutex;
    uintptr_t cond;
} mutex_cond_t;

static cover_query_t *queue;
static cover_query_t *queue_tail;
static uintptr_t mutex;
static uintptr_t cond;
static volatile int terminate;
static volatile int clear_queue;
static intptr_t tid;

static int artwork_enable_embedded;
static int artwork_enable_local;
#ifdef USE_VFS_CURL
static int artwork_enable_lfm;
static int artwork_enable_aao;
static int artwork_enable_wos;
#endif
static time_t artwork_reset_time;
static char artwork_filemask[200];

static const char *get_default_cover (void) {
    return default_cover;
}

static int
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
make_cache_dir_path (char *path, int size, const char *artist, int img_size) {
    char esc_artist[PATH_MAX];
    int i;

    if (artist) {
        for (i = 0; artist[i]; i++) {
            esc_artist[i] = esc_char (artist[i]);
        }
        esc_artist[i] = 0;
    }
    else {
        strcpy (esc_artist, "Unknown artist");
    }

    const char *cache = getenv ("XDG_CACHE_HOME");
    int sz;

    if (img_size == -1) {
        sz = snprintf (path, size, cache ? "%s/deadbeef/covers/" : "%s/.cache/deadbeef/covers/", cache ? cache : getenv ("HOME"));
    }
    else {
        sz = snprintf (path, size, cache ? "%s/deadbeef/covers-%d/" : "%s/.cache/deadbeef/covers-%d/", cache ? cache : getenv ("HOME"), img_size);
    }
    path += sz;

    sz += snprintf (path, size-sz, "%s", esc_artist);
    for (char *p = path; *p; p++) {
        if (*p == '/') {
            *p = '_';
        }
    }
    return sz;
}

static int
make_cache_path2 (char *path, int size, const char *fname, const char *album, const char *artist, int img_size) {
    *path = 0;

    int unk = 0;
    int unk_artist = 0;

    if (!album || !(*album)) {
        album = "Unknown album";
        unk = 1;
    }
    if (!artist || !(*artist)) {
        artist = "Unknown artist";
        unk_artist = 1;
    }

    if (unk)
    {
        if (fname) {
            album = fname;
        }
        else if (!unk_artist) {
            album = artist;
        }
        else {
            trace ("not possible to get any unique album name\n");
            return -1;
        }
    }

    char *p = path;
    char esc_album[PATH_MAX];
    const char *palbum = album;
    size_t l = strlen (album);
    if (l > 200) {
        palbum = album + l - 200;
    }
    int i;
    for (i = 0; palbum[i]; i++) {
        esc_album[i] = esc_char (palbum[i]);
    }
    esc_album[i] = 0;

    int sz = make_cache_dir_path (path, size, artist, img_size);
    size -= sz;
    path += sz;
    sz = snprintf (path, size, "/%s.jpg", esc_album);
    for (char *p = path+1; *p; p++) {
        if (*p == '/') {
            *p = '_';
        }
    }
}

static void
make_cache_path (char *path, int size, const char *album, const char *artist, int img_size) {
    make_cache_path2 (path, size, NULL, album, artist, img_size);
}

static void
queue_add (const char *fname, const char *artist, const char *album, int img_size, artwork_callback callback, void *user_data) {
    if (!artist) {
        artist = "";
    }
    if (!album) {
        album = "";
    }
    deadbeef->mutex_lock (mutex);

    for (cover_query_t *q = queue; q; q = q->next) {
        if (!strcasecmp (artist, q->artist) && !strcasecmp (album, q->album) && img_size == q->size) {
            // already in queue, add callback
            if (q->numcb < MAX_CALLBACKS && callback) {
                q->callbacks[q->numcb].cb = callback;
                q->callbacks[q->numcb].ud = user_data;
                q->numcb++;
            }
            deadbeef->mutex_unlock (mutex);
            return;
        }
    }

    trace ("artwork:queue_add %s %s %s %d\n", fname, artist, album, img_size);
    cover_query_t *q = malloc (sizeof (cover_query_t));
    memset (q, 0, sizeof (cover_query_t));
    q->fname = strdup (fname);
    q->artist = strdup (artist);
    q->album = strdup (album);
    q->size = img_size;
    q->callbacks[q->numcb].cb = callback;
    q->callbacks[q->numcb].ud = user_data;
    q->numcb++;
    if (queue_tail) {
        queue_tail->next = q;
        queue_tail = q;
    }
    else {
        queue = queue_tail = q;
    }
    deadbeef->mutex_unlock (mutex);
    deadbeef->cond_signal (cond);
}

static void
queue_pop (void) {
    deadbeef->mutex_lock (mutex);
    cover_query_t *next = queue ? queue->next : NULL;
    if (queue) {
        if (queue->fname) {
            free (queue->fname);
        }
        if (queue->artist) {
            free (queue->artist);
        }
        if (queue->album) {
            free (queue->album);
        }
        for (int i = 0; i < queue->numcb; i++) {
            if (queue->callbacks[i].cb) {
                queue->callbacks[i].cb (NULL, NULL, NULL, queue->callbacks[i].ud);
            }
        }
        free (queue);
    }
    queue = next;
    if (!queue) {
        queue_tail = NULL;
    }
    deadbeef->mutex_unlock (mutex);
}

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

struct my_error_mgr {
  struct jpeg_error_mgr pub;	/* "public" fields */

  jmp_buf setjmp_buffer;	/* for return to caller */
};

typedef struct my_error_mgr * my_error_ptr;

METHODDEF(void)
my_error_exit (j_common_ptr cinfo)
{
  /* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
  my_error_ptr myerr = (my_error_ptr) cinfo->err;

//  (*cinfo->err->output_message) (cinfo);

  /* Return control to the setjmp point */
  longjmp(myerr->setjmp_buffer, 1);
}


static int
jpeg_resize (const char *fname, const char *outname, int scaled_size) {
    trace ("resizing %s into %s\n", fname, outname);
    struct jpeg_decompress_struct cinfo;
    struct jpeg_compress_struct cinfo_out;

    memset (&cinfo, 0, sizeof (cinfo));
    memset (&cinfo_out, 0, sizeof (cinfo_out));

    struct my_error_mgr jerr;

    FILE *fp = NULL, *out = NULL;

    cinfo.err = jpeg_std_error (&jerr.pub);
    cinfo_out.err = cinfo.err;
    jerr.pub.error_exit = my_error_exit;
    /* Establish the setjmp return context for my_error_exit to use. */
    if (setjmp(jerr.setjmp_buffer)) {
        trace("failed to scale %s as jpeg\n", outname);
        jpeg_destroy_decompress(&cinfo);
        jpeg_destroy_compress(&cinfo_out);
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
    jpeg_stdio_src (&cinfo, fp);
    jpeg_read_header (&cinfo, TRUE);
    jpeg_start_decompress (&cinfo);

    jpeg_create_compress(&cinfo_out);
    jpeg_stdio_dest(&cinfo_out, out);

    int scaled_width, scaled_height;
    if (deadbeef->conf_get_int ("artwork.scale_towards_longer", 1) == cinfo.image_width > cinfo.image_height) {
        scaled_height = scaled_size;
        scaled_width = scaled_size * cinfo.image_width / cinfo.image_height;
    }
    else {
        scaled_width = scaled_size;
        scaled_height = scaled_size * cinfo.image_height / cinfo.image_width;
    }
    cinfo_out.image_width      = scaled_width;
    cinfo_out.image_height     = scaled_height;
    cinfo_out.input_components = cinfo.output_components;
    cinfo_out.in_color_space   = cinfo.out_color_space;
    jpeg_set_defaults(&cinfo_out);
    jpeg_set_quality(&cinfo_out, 95, TRUE);
    jpeg_start_compress(&cinfo_out, TRUE);

    const uint_fast32_t row_components = cinfo.output_width * cinfo.output_components;
    JSAMPLE scanline[row_components];
    JSAMPLE next_scanline[row_components];
    const uint_fast32_t scaled_row_components = scaled_width * cinfo.output_components;
    JSAMPLE out_line[scaled_row_components];
    JSAMPROW row = scanline;
    JSAMPROW next_row = next_scanline;
    JSAMPROW out_row = out_line;

    /* Bilinear interpolation to improve the scaled image quality a little */
    const float scaling_ratio = (float)cinfo.output_width / scaled_width;
    const float downscale_offset = scaling_ratio > 1.5 ? 0.5 : 0;
    float y_interp = downscale_offset;
    for (uint_fast16_t scaled_y = 0; scaled_y < scaled_height; scaled_y++, y_interp+=scaling_ratio) {
        const uint_fast16_t y = y_interp;
        const uint_fast16_t y_diff = (uint_fast16_t)(y_interp*256) - (y<<8);
        const uint_fast16_t y_remn = 256 - y_diff;

        if (cinfo.output_scanline < y+2) {
            while (cinfo.output_scanline < y+1) {
                jpeg_read_scanlines(&cinfo, &next_row, 1);
            }
            memcpy(row, next_row, row_components*sizeof(JSAMPLE));
            if (y+2 < cinfo.output_height) {
                jpeg_read_scanlines(&cinfo, &next_row, 1);
            }
        }

        float x_interp = downscale_offset;
        for (uint_fast32_t scaled_x = 0; scaled_x < scaled_row_components; scaled_x+=cinfo.output_components, x_interp+=scaling_ratio) {
            const uint_fast16_t x = x_interp;
            const uint_fast32_t x_index = x * cinfo.output_components;
            const uint_fast32_t next_x_index = x+1 < cinfo.output_width ? x_index+cinfo.output_components : x_index;

            const uint_fast16_t x_diff = (uint_fast16_t)(x_interp*256) - (x<<8);
            const uint_fast16_t x_remn = 256 - x_diff;
            const uint_fast32_t weight = x_remn * y_remn;
            const uint_fast32_t weightx = x_diff * y_remn;
            const uint_fast32_t weighty = x_remn * y_diff;
            const uint_fast32_t weightxy = x_diff * y_diff;

            for (uint_fast8_t component=0; component<cinfo.output_components; component++) {
                out_line[scaled_x + component] = blerp_pixel(row, next_row, x_index+component, next_x_index+component, weight, weightx, weighty, weightxy) >> 16;
            }
        }

        jpeg_write_scanlines(&cinfo_out, &out_row, 1);
    }
    while (cinfo.output_scanline < cinfo.output_height) {
        jpeg_read_scanlines(&cinfo, &next_row, 1);
    }

    jpeg_finish_compress(&cinfo_out);
    jpeg_destroy_compress(&cinfo_out);

    jpeg_finish_decompress (&cinfo);
    jpeg_destroy_decompress (&cinfo);

    fclose(fp);
    fclose(out);

    return 0;
}

static int
png_resize (const char *fname, const char *outname, int scaled_size) {
    png_structp png_ptr = NULL, new_png_ptr = NULL;
    png_infop info_ptr = NULL, new_info_ptr = NULL;
    png_byte *out_row = NULL;
    png_uint_32 height, width;
    int bit_depth, color_type;
    uint8_t num_components;
    int err = -1;
    FILE *fp = NULL;
    FILE *out = NULL;
    struct my_error_mgr jerr;
    struct jpeg_compress_struct cinfo_out;

    fp = fopen(fname, "rb");
    if (!fp) {
        goto error;
    }

    png_ptr = png_create_read_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) {
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
        goto error;
    }

    png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_STRIP_16|PNG_TRANSFORM_PACKING|PNG_TRANSFORM_EXPAND, NULL);
    png_bytep *row_pointers = png_get_rows(png_ptr, info_ptr);
    png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, NULL, NULL, NULL);

    int scaled_width, scaled_height;
    if (deadbeef->conf_get_int ("artwork.scale_towards_longer", 1) == width > height) {
        scaled_height = scaled_size;
        scaled_width = scaled_size * width / height;
    }
    else {
        scaled_width = scaled_size;
        scaled_height = scaled_size * height / width;
    }

    out = fopen (outname, "w+b");
    if (!out) {
        fclose (fp);
        goto error;
    }

    new_png_ptr = png_create_write_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!new_png_ptr) {
        fprintf (stderr, "failed to create png write struct\n");
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
        fprintf (stderr, "failed to create png info struct for writing\n");
        goto error;
    }

    if (color_type & PNG_COLOR_MASK_ALPHA) {
        color_type = PNG_COLOR_TYPE_RGB_ALPHA;
        num_components = 4;
    }
    else {
        color_type = PNG_COLOR_TYPE_RGB;
        num_components = 3;
    }
    png_set_IHDR(new_png_ptr, new_info_ptr, scaled_width, scaled_height, bit_depth, color_type, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_write_info(new_png_ptr, new_info_ptr);
    png_set_packing(new_png_ptr);

    const uint_fast32_t scaled_row_components = scaled_width * num_components;
    out_row = malloc(scaled_row_components * sizeof(png_byte));
    if (!out_row) {
        goto error;
    }

    const float scaling_ratio = (float)width / scaled_width;
    const float downscale_offset = scaling_ratio > 1.5 ? 0.5 : 0;
#ifndef USE_BICUBIC
    /* Bilinear interpolation to improve the scaled image quality a little */
    uint_fast32_t scaled_alpha = 255 << 16;
    float y_interp = downscale_offset;
    for (uint_fast16_t scaled_y = 0; scaled_y < scaled_height; scaled_y++, y_interp+=scaling_ratio) {
        const uint_fast16_t y = y_interp;
        const png_byte *row = row_pointers[y];
        const png_byte *next_row = y+1 < height ? row_pointers[y+1] : row;

        const uint_fast16_t y_diff = (uint_fast16_t)(y_interp*256) - (y<<8);
        const uint_fast16_t y_remn = 256 - y_diff;

        float x_interp = downscale_offset;
        for (uint_fast32_t scaled_x = 0; scaled_x < scaled_row_components; scaled_x+=num_components, x_interp+=scaling_ratio) {
            const uint_fast16_t x = x_interp;
            const uint_fast32_t x_index = x * num_components;
            const uint_fast32_t next_x_index = x < width ? x_index+num_components : x_index;

            const uint_fast16_t x_diff = (uint_fast16_t)(x_interp*256) - (x<<8);
            const uint_fast16_t x_remn = 256 - x_diff;
            const uint_fast32_t weight = x_remn * y_remn;
            const uint_fast32_t weightx = x_diff * y_remn;
            const uint_fast32_t weighty = x_remn * y_diff;
            const uint_fast32_t weightxy = x_diff * y_diff;

            uint_fast32_t alpha, alphax, alphay, alphaxy;
            if (num_components == 4) {
                /* Interpolate alpha channel and weight pixels by their alpha */
                alpha = weight * row[x_index + 3];
                alphax = weightx * row[next_x_index + 3];
                alphay = weighty * next_row[x_index + 3];
                alphaxy = weightxy * next_row[next_x_index + 3];
                scaled_alpha = alpha + alphax + alphay + alphaxy;
                out_row[scaled_x + 3] = scaled_alpha >> 16;
            }

            if (scaled_alpha == 255 << 16) {
                /* Simplified calculation for fully opaque pixels */
                for (uint_fast8_t component=0; component<3; component++) {
                    out_row[scaled_x + component] = blerp_pixel(row, next_row, x_index+component, next_x_index+component, weight, weightx, weighty, weightxy) >> 16;
                }
            }
            else if (scaled_alpha == 0) {
                /* For speed, don't preserve the values of fully transparent pixels */
                for (uint_fast8_t component=0; component<3; component++) {
                    out_row[scaled_x + component] = 0;
                }
            }
            else {
                /* Alpha-weight partially transparent pixels to avoid background colour bleeding */
                for (uint_fast8_t component=0; component<3; component++) {
                    out_row[scaled_x + component] = blerp_pixel(row, next_row, x_index+component, next_x_index+component, alpha, alphax, alphay, alphaxy) / scaled_alpha;
                }
            }
        }
        png_write_row(new_png_ptr, out_row);
    }
#else
    /* Pre-multiply alpha onto all pixels (slight approximation) */
    if (num_components == 4) {
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

    /* Bicubic interpolation to improve the scaled image quality */
    int_fast16_t scaled_alpha = 255;
    float y_interp = downscale_offset;
    for (uint_fast16_t scaled_y = 0; scaled_y < scaled_height; scaled_y++, y_interp+=scaling_ratio) {
        const uint_fast16_t y = y_interp;
        const png_byte *row1 = row_pointers[y];
        const png_byte *row0 = y > 0 ? row_pointers[y-1] : row1;
        const png_byte *row2 = y+1 < height ? row_pointers[y+1] : row1;
        const png_byte *row3 = y+2 < height ? row_pointers[y+2] : row2;

        const float dy = y_interp - (long)y_interp;
        const float dy2 = dy * dy;
        const float dy3 = dy2 * dy;

        float x_interp = downscale_offset;
        for (uint_fast32_t scaled_x = 0; scaled_x < scaled_row_components; scaled_x+=num_components, x_interp+=scaling_ratio) {
            const uint_fast16_t x = x_interp;
            const uint_fast32_t x1 = x * num_components;
            const uint_fast32_t x0 = x > 0 ? x1-num_components : x1;
            const uint_fast32_t x2 = x+1 < width ? x1+num_components : x1;
            const uint_fast32_t x3 = x+2 < width ? x2+num_components : x2;

            const float dx = x_interp - (long)x_interp;
            const float dx2 = dx * dx;
            const float dx3 = dx2 * dx;

            if (num_components == 4) {
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
                    const int_fast16_t pixel = bcerp(row0, row1, row2, row3, x0, x1, x2, x3, component, dx, dx2, dx3, dy, dy2, dy3) * 255 / scaled_alpha;
                    out_row[scaled_x + component] = pixel < 0 ? 0 : pixel > 255 ? 255 : pixel;
                }
            }
        }

        png_write_row(new_png_ptr, out_row);
    }
#endif

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
        png_destroy_read_struct (&png_ptr, info_ptr ? &info_ptr : NULL, (png_infopp)NULL);
    }
    if (new_png_ptr) {
        png_destroy_write_struct(&new_png_ptr, new_info_ptr ? &new_info_ptr : NULL);
    }
    if (out_row) {
        free(out_row);
    }

    return err;
}

#endif

#define BUFFER_SIZE 4096

static int
copy_file (const char *in, const char *out, int img_size) {
    trace ("copying %s to %s\n", in, out);

    if (img_size != -1) {
#ifdef USE_IMLIB2
        deadbeef->mutex_lock (imlib_mutex);
        // need to scale, use imlib2
        Imlib_Image img = imlib_load_image_immediately (in);
        if (!img) {
            trace ("file %s not found, or imlib2 can't load it\n", in);
            deadbeef->mutex_unlock (imlib_mutex);
            return -1;
        }
        imlib_context_set_image(img);
        int w = imlib_image_get_width ();
        int h = imlib_image_get_height ();
        int sw, sh;
        if (deadbeef->conf_get_int ("artwork.scale_towards_longer", 1)) {
            if (w > h) {
                sh = img_size;
                sw = img_size * w / h;
            }
            else {
                sw = img_size;
                sh = img_size * h / w;
            }
        }
        else {
            if (w < h) {
                sh = img_size;
                sw = img_size * w / h;
            }
            else {
                sw = img_size;
                sh = img_size * h / w;
            }
        }
        int is_jpeg = imlib_image_format() && imlib_image_format()[0] == 'j';
        Imlib_Image scaled = imlib_create_image (sw, sh);
        imlib_context_set_image (scaled);
        imlib_blend_image_onto_image (img, 1, 0, 0, w, h, 0, 0, sw, sh);
        Imlib_Load_Error err = 0;
        imlib_image_set_format (is_jpeg ? "jpg" : "png");
        if (is_jpeg)
            imlib_image_attach_data_value ("quality", NULL, 95, NULL);
        imlib_save_image_with_error_return (out, &err);
        if (err != 0) {
            trace ("imlib save %s returned %d\n", out, err);
            imlib_free_image ();
            imlib_context_set_image(img);
            imlib_free_image ();
            deadbeef->mutex_unlock (imlib_mutex);
            return -1;
        }
        imlib_free_image ();
        imlib_context_set_image(img);
        imlib_free_image ();
        deadbeef->mutex_unlock (imlib_mutex);
#else
        int res = jpeg_resize (in, out, img_size);
        if (res != 0) {
            unlink (out);
            res = png_resize (in, out, img_size);
            if (res != 0) {
                unlink (out);
                return -1;
            }
        }
#endif
        return 0;
    }

    FILE *fin = fopen (in, "rb");
    if (!fin) {
        trace ("artwork: failed to open file %s for reading\n", in);
        return -1;
    }
    FILE *fout = fopen (out, "w+b");
    if (!fout) {
        fclose (fin);
        trace ("artwork: failed to open file %s for writing\n", out);
        return -1;
    }
    char *buf = malloc (BUFFER_SIZE);
    if (!buf) {
        trace ("artwork: failed to alloc %d bytes\n", BUFFER_SIZE);
        fclose (fin);
        fclose (fout);
        return -1;
    }

    fseek (fin, 0, SEEK_END);
    size_t sz = ftell (fin);
    rewind (fin);

    while (sz > 0) {
        int rs = min (sz, BUFFER_SIZE);
        if (fread (buf, rs, 1, fin) != 1) {
            trace ("artwork: failed to read file %s\n", in);
            break;
        }
        if (fwrite (buf, rs, 1, fout) != 1) {
            trace ("artwork: failed to write file %s\n", out);
            break;
        }
        sz -= rs;
    }
    free (buf);
    fclose (fin);
    fclose (fout);
    if (sz > 0) {
        unlink (out);
    }
    return 0;
}

static const char *filter_custom_mask = NULL;

static int
filter_custom (const struct dirent *f)
{
// FNM_CASEFOLD is not defined on solaris. On other platforms it is.
// It should be safe to define it as FNM_INGORECASE if it isn't defined.
#ifndef FNM_CASEFOLD
#define FNM_CASEFOLD FNM_IGNORECASE
#endif
    if (!fnmatch (filter_custom_mask, f->d_name, FNM_CASEFOLD)) {
        return 1;
    }
    return 0;
}

static int
filter_jpg (const struct dirent *f)
{
    const char *ext = strrchr (f->d_name, '.');
    if (!ext)
        return 0;
    if (!strcasecmp (ext, ".jpg") || !strcasecmp (ext, ".jpeg")) {
        return 1;
    }

    return 0;
}

static uint8_t *
id3v2_skip_str (int enc, uint8_t *ptr, uint8_t *end) {
    if (enc == 0 || enc == 3) {
        while (ptr < end && *ptr) {
            ptr++;
        }
        ptr++;
        if (ptr >= end) {
            return NULL;
        }
        return ptr;
    }
    else {
        while (ptr < end-1 && (ptr[0] || ptr[1])) {
            ptr += 2;
        }
        ptr += 2;
        if (ptr >= end) {
            return NULL;
        }
        return ptr;
    }
    return NULL;
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

static FLAC__IOCallbacks iocb = {
    .read = flac_io_read,
    .write = NULL,
    .seek = flac_io_seek,
    .tell = flac_io_tell,
    .eof = flac_io_eof,
    .close = flac_io_close,
};
#endif

static void
fetcher_thread (void *none)
{
#ifdef __linux__
    prctl (PR_SET_NAME, "deadbeef-artwork", 0, 0, 0, 0);
#endif
    for (;;) {
        trace ("artwork: waiting for signal\n");
        deadbeef->cond_wait (cond, mutex);
        trace ("artwork: cond signalled\n");
        deadbeef->mutex_unlock (mutex);
        while (!terminate && queue && !clear_queue) {
            cover_query_t *param = queue;
            char path [PATH_MAX];
            struct dirent **files;
            int files_count;

            make_cache_dir_path (path, sizeof (path), param->artist, -1);
            trace ("cache folder: %s\n", path);
            if (!check_dir (path, 0755)) {
                queue_pop ();
                trace ("failed to create folder for %s %s\n", param->album, param->artist);
                continue;
            }
            if (param->size != -1) {
                make_cache_dir_path (path, sizeof (path), param->artist, param->size);
                trace ("cache folder: %s\n", path);
                if (!check_dir (path, 0755)) {
                    queue_pop ();
                    trace ("failed to create folder for %s %s\n", param->album, param->artist);
                    continue;
                }
            }

            trace ("fetching cover for %s %s\n", param->album, param->artist);
            char cache_path[1024];
            make_cache_path2 (cache_path, sizeof (cache_path), param->fname, param->album, param->artist, -1);
            int got_pic = 0;

            if (deadbeef->is_local_file (param->fname)) {
                if (artwork_enable_embedded) {
                    // try to load embedded from id3v2
                    {
                        trace ("trying to load artwork from id3v2 tag for %s\n", param->fname);
                        DB_id3v2_tag_t tag;
                        memset (&tag, 0, sizeof (tag));
                        DB_FILE *fp = deadbeef->fopen (param->fname);
                        current_file = fp;
                        if (fp) {
                            int res = deadbeef->junk_id3v2_read_full (NULL, &tag, fp);
                            if (!res) {
                                for (DB_id3v2_frame_t *f = tag.frames; f; f = f->next) {
                                    if (!strcmp (f->id, "APIC")) {
                                        if (f->size < 20) {
                                            trace ("artwork: id3v2 APIC frame is too small\n");
                                            continue;
                                        }

                                        uint8_t *data = f->data;

                                        if (tag.version[0] == 4 && (f->flags[1] & 1)) {
                                            data += 4;
                                        }
#if 0
                                        printf ("version: %d, flags: %d %d\n", (int)tag.version[0], (int)f->flags[0], (int)f->flags[1]);
                                        for (int i = 0; i < 20; i++) {
                                            printf ("%c", data[i] < 0x20 ? '?' : data[i]);
                                        }
                                        printf ("\n");
                                        for (int i = 0; i < 20; i++) {
                                            printf ("%02x ", data[i]);
                                        }
                                        printf ("\n");
#endif
                                        uint8_t *end = f->data + f->size;
                                        int enc = *data;
                                        data++; // enc
                                        // mime-type must always be ASCII - hence enc is 0 here
                                        uint8_t *mime_end = id3v2_skip_str (enc, data, end);
                                        if (!mime_end) {
                                            trace ("artwork: corrupted id3v2 APIC frame\n");
                                            continue;
                                        }
                                        if (strcasecmp (data, "image/jpeg") && strcasecmp (data, "image/png") && strcasecmp (data, "image/gif")) {
                                            trace ("artwork: unsupported mime type: %s\n", data);
                                            continue;
                                        }
                                        if (*mime_end != 3) {
                                            trace ("artwork: picture type=%d\n", *mime_end);
                                            continue;
                                        }
                                        trace ("artwork: mime-type=%s, picture type: %d\n", data, *mime_end);
                                        data = mime_end;
                                        data++; // picture type
                                        data = id3v2_skip_str (enc, data, end); // description
                                        if (!data) {
                                            trace ("artwork: corrupted id3v2 APIC frame\n");
                                            continue;
                                        }
                                        int sz = f->size - (data - f->data);

                                        char tmp_path[1024];
                                        trace ("will write id3v2 APIC into %s\n", cache_path);
                                        snprintf (tmp_path, sizeof (tmp_path), "%s.part", cache_path);
                                        FILE *out = fopen (tmp_path, "w+b");
                                        if (!out) {
                                            trace ("artwork: failed to open %s for writing\n", tmp_path);
                                            break;
                                        }
                                        if (fwrite (data, 1, sz, out) != sz) {
                                            trace ("artwork: failed to write id3v2 picture into %s\n", tmp_path);
                                            fclose (out);
                                            unlink (tmp_path);
                                            break;
                                        }
                                        fclose (out);
                                        int err = rename (tmp_path, cache_path);
                                        if (err != 0) {
                                            trace ("Failed not move %s to %s: %s\n", tmp_path, cache_path, strerror (err));
                                            unlink (tmp_path);
                                            break;
                                        }
                                        unlink (tmp_path);
                                        got_pic = 1;
                                        break;
                                    }
                                }
                            }

                            deadbeef->junk_id3v2_free (&tag);
                            current_file = NULL;
                            deadbeef->fclose (fp);
                        }
                    }

                    // try to load embedded from apev2
                    {
                        trace ("trying to load artwork from apev2 tag for %s\n", param->fname);
                        DB_apev2_tag_t tag;
                        memset (&tag, 0, sizeof (tag));
                        DB_FILE *fp = deadbeef->fopen (param->fname);
                        current_file = fp;
                        if (fp) {
                            int res = deadbeef->junk_apev2_read_full (NULL, &tag, fp);
                            if (!res) {
                                for (DB_apev2_frame_t *f = tag.frames; f; f = f->next) {
                                    if (!strcasecmp (f->key, "cover art (front)")) {
                                        uint8_t *name = f->data, *ext = f->data, *data = f->data;
                                        uint8_t *end = f->data + f->size;
                                        while (data < end && *data)
                                            data++;
                                        if (data == end) {
                                            trace ("artwork: apev2 cover art frame has no name\n");
                                            continue;
                                        }
                                        int sz = end - ++data;
                                        if (sz < 20) {
                                            trace ("artwork: apev2 cover art frame is too small\n");
                                            continue;
                                        }
                                        ext = strrchr (name, '.');
                                        if (!ext || !*++ext) {
                                            trace ("artwork: apev2 cover art name has no extension\n");
                                            continue;
                                        }
                                        if (strcasecmp (ext, "jpeg") && strcasecmp (ext, "jpg") && strcasecmp (ext, "png")) {
                                            trace ("artwork: unsupported file type: %s\n", ext);
                                            continue;
                                        }
                                        trace ("found apev2 cover art of %d bytes (%s)\n", sz, ext);
                                        char tmp_path[1024];
                                        char cache_path[1024];
                                        make_cache_path2 (cache_path, sizeof (cache_path), param->fname, param->album, param->artist, -1);
                                        trace ("will write apev2 cover art into %s\n", cache_path);
                                        snprintf (tmp_path, sizeof (tmp_path), "%s.part", cache_path);
                                        FILE *out = fopen (tmp_path, "w+b");
                                        if (!out) {
                                            trace ("artwork: failed to open %s for writing\n", tmp_path);
                                            break;
                                        }
                                        if (fwrite (data, 1, sz, out) != sz) {
                                            trace ("artwork: failed to write apev2 picture into %s\n", tmp_path);
                                            fclose (out);
                                            unlink (tmp_path);
                                            break;
                                        }
                                        fclose (out);
                                        int err = rename (tmp_path, cache_path);
                                        if (err != 0) {
                                            trace ("Failed not move %s to %s: %s\n", tmp_path, cache_path, strerror (err));
                                            unlink (tmp_path);
                                            break;
                                        }
                                        unlink (tmp_path);
                                        got_pic = 1;
                                        break;
                                    }
                                }
                            }

                            deadbeef->junk_apev2_free (&tag);
                            current_file = NULL;
                            deadbeef->fclose (fp);
                        }
                    }

#ifdef USE_METAFLAC
                    // try to load embedded from flac metadata
                    for (;;)
                    {
                        const char *filename = param->fname;
                        FLAC__Metadata_Chain *chain = FLAC__metadata_chain_new();
                        int is_ogg = 0;
                        if(strlen(filename) >= 4 && (0 == strcmp(filename+strlen(filename)-4, ".oga") || 0 == strcasecmp(filename+strlen(filename)-4, ".ogg"))) {
                            is_ogg = 1;
                        }

                        DB_FILE *file = deadbeef->fopen (filename);
                        if (!file) {
                            break;
                        }

                        int res = 0;
                        if (is_ogg) {
#if USE_OGG
                            res = FLAC__metadata_chain_read_ogg_with_callbacks(chain, (FLAC__IOHandle)file, iocb);
#endif
                        }
                        else
                        {
                            res = FLAC__metadata_chain_read_with_callbacks(chain, (FLAC__IOHandle)file, iocb);
                        }

                        if(!res) {
                            trace ("artwork: failed to read metadata from flac: %s\n", filename);
                            deadbeef->fclose (file);
                            FLAC__metadata_chain_delete(chain);
                            break;
                        }
                        deadbeef->fclose (file);
                        FLAC__StreamMetadata *picture = 0;
                        FLAC__Metadata_Iterator *iterator = FLAC__metadata_iterator_new();
                        FLAC__metadata_iterator_init(iterator, chain);

                        do {
                            FLAC__StreamMetadata *block = FLAC__metadata_iterator_get_block(iterator);
                            if(block->type == FLAC__METADATA_TYPE_PICTURE) {
                                picture = block;
                            }
                        } while(FLAC__metadata_iterator_next(iterator) && 0 == picture);

                        if (!picture) {
                            trace ("%s doesn't have an embedded cover\n", param->fname);
                            break;
                        }
                        FLAC__StreamMetadata_Picture *pic = &picture->data.picture;
                        trace ("found flac cover art of %d bytes (%s)\n", pic->data_length, pic->description);
                        char tmp_path[1024];
                        char cache_path[1024];
                        make_cache_path2 (cache_path, sizeof (cache_path), param->fname, param->album, param->artist, -1);
                        trace ("will write flac cover art into %s\n", cache_path);
                        snprintf (tmp_path, sizeof (tmp_path), "%s.part", cache_path);
                        FILE *out = fopen (tmp_path, "w+b");
                        if (!out) {
                            trace ("artwork: failed to open %s for writing\n", tmp_path);
                            break;
                        }
                        if (fwrite (pic->data, 1, pic->data_length, out) != pic->data_length) {
                            trace ("artwork: failed to write flac picture into %s\n", tmp_path);
                            fclose (out);
                            unlink (tmp_path);
                            break;
                        }
                        fclose (out);
                        int err = rename (tmp_path, cache_path);
                        if (err != 0) {
                            trace ("Failed not move %s to %s: %s\n", tmp_path, cache_path, strerror (err));
                            unlink (tmp_path);
                            break;
                        }
                        unlink (tmp_path);
                        got_pic = 1;

                        if (chain) {
                            FLAC__metadata_chain_delete(chain);
                        }
                        if (iterator) {
                            FLAC__metadata_iterator_delete(iterator);
                        }
                        break;
                    }
#endif
                }

                if (!got_pic && artwork_enable_local) {
                    /* Searching in track directory */
                    strncpy (path, param->fname, sizeof (path));
                    char *slash = strrchr (path, '/');
                    if (slash) {
                        *slash = 0; // assuming at least one slash exist
                    }
                    trace ("scanning directory: %s\n", path);
                    char mask[200] = "";
                    char *p = artwork_filemask;
                    while (p) {
                        *mask = 0;
                        char *e = strchr (p, ';');
                        if (e) {
                            strncpy (mask, p, e-p);
                            mask[e-p] = 0;
                            e++;
                        }
                        else {
                            strcpy (mask, p);
                        }
                        if (*mask) {
                            filter_custom_mask = mask;
                            files_count = scandir (path, &files, filter_custom, NULL);
                            if (files_count != 0) {
                                break;
                            }
                        }
                        p = e;
                    }
                    if (files_count == 0) {
                        files_count = scandir (path, &files, filter_jpg, alphasort);
                    }

                    if (files_count > 0) {
                        trace ("found cover for %s - %s in local folder\n", param->artist, param->album);
                        strcat (path, "/");
                        strcat (path, files[0]->d_name);
                        char cache_path[PATH_MAX];
                        char tmp_path[PATH_MAX];
                        char cache_path_dir[PATH_MAX];
                        make_cache_path2 (cache_path, sizeof (cache_path), param->fname, param->album, param->artist, -1);
                        strcpy (cache_path_dir, cache_path);
                        char *slash = strrchr (cache_path_dir, '/');
                        if (slash) {
                            *slash = 0;
                        }
                        trace ("check_dir: %s\n", cache_path_dir);
                        if (check_dir (cache_path_dir, 0755)) {
                            snprintf (tmp_path, sizeof (tmp_path), "%s.part", cache_path);
                            copy_file (path, tmp_path, -1);
                            int err = rename (tmp_path, cache_path);
                            if (err != 0) {
                                trace ("artwork: rename error %d: failed to move %s to %s: %s\n", err, tmp_path, cache_path, strerror (err));
                                unlink (tmp_path);
                            }
                            int i;
                            for (i = 0; i < files_count; i++) {
                                free (files [i]);
                            }
                            got_pic = 1;
                        }
                    }
                }
            }

#ifdef USE_VFS_CURL
            if (!got_pic) {
                if (artwork_enable_wos) {

                    char *dot = strrchr (param->fname, '.');
                    if (dot && !strcasecmp (dot, ".ay") && !fetch_from_wos (param->album, cache_path)) {
                        got_pic = 1;
                    }
                }
                if (!got_pic && artwork_enable_lfm) {
                    if (!fetch_from_lastfm (param->artist, param->album, cache_path)) {
                        got_pic = 1;
                    }
                    else {
                        // try to fix parentheses
                        char *fixed_alb = strdupa (param->album);
                        char *openp = strchr (fixed_alb, '(');
                        if (openp && openp != fixed_alb) {
                            *openp = 0;
                            if (!fetch_from_lastfm (param->artist, fixed_alb, cache_path)) {
                                got_pic = 1;
                            }
                        }
                    }
                }
                if (!got_pic && artwork_enable_aao && !fetch_from_albumart_org (param->artist, param->album, cache_path)) {
                    got_pic = 1;
                }
            }
#endif

            if (got_pic) {
                trace ("downloaded art for %s %s\n", param->album, param->artist);
                if (param->size != -1) {
                    make_cache_dir_path (path, sizeof (path), param->artist, param->size);
                    trace ("cache folder: %s\n", path);
                    if (!check_dir (path, 0755)) {
                        trace ("failed to create folder %s\n", path);
                        queue_pop ();
                        continue;
                    }
                    char scaled_path[1024];
                    make_cache_path2 (scaled_path, sizeof (scaled_path), param->fname, param->album, param->artist, param->size);
                    copy_file (cache_path, scaled_path, param->size);
                }
                for (int i = 0; i < param->numcb; i++) {
                    if (param->callbacks[i].cb) {
                        param->callbacks[i].cb (param->fname, param->artist, param->album, param->callbacks[i].ud);
                        param->callbacks[i].cb = NULL;
                    }
                }
            }
            queue_pop ();
        }
        if (clear_queue) {
            trace ("artwork: received queue clear request\n");
            while (queue) {
                queue_pop ();
            }
            clear_queue = 0;
            trace ("artwork: queue clear done\n");
            continue;
        }
        if (terminate) {
            break;
        }
    }
}

static char *
find_image (const char *path) {
    struct stat stat_buf;
    if (0 == stat (path, &stat_buf)) {
        int cache_period = deadbeef->conf_get_int ("artwork.cache.period", 48);
        time_t tm = time (NULL);
        // invalidate cache every 2 days
        if ((cache_period > 0 && (tm - stat_buf.st_mtime > cache_period * 60 * 60))
                || artwork_reset_time > stat_buf.st_mtime) {
            fprintf(stderr,"deleting cached file %s\n", path);
            unlink (path);
            return NULL;
        }

        return strdup (path);
    }
    return NULL;
}

static char*
get_album_art (const char *fname, const char *artist, const char *album, int size, artwork_callback callback, void *user_data)
{
    char path [1024];

    make_cache_path2 (path, sizeof (path), fname, album, artist, size);
    char *p = find_image (path);
    if (p) {
        if (callback) {
            callback (NULL, NULL, NULL, user_data);
        }
        return p;
    }

    if (size != -1) {
        // check if we have unscaled image
        char unscaled_path[1024];
        make_cache_path2 (unscaled_path, sizeof (unscaled_path), fname, album, artist, -1);
        p = find_image (unscaled_path);
        if (p) {
            free (p);
            char dir[1024];
            make_cache_dir_path (dir, sizeof (dir), artist, size);
            if (!check_dir (dir, 0755)) {
                trace ("failed to create folder for %s\n", dir);
            }
            else {
                int res = copy_file (unscaled_path, path, size);
                if (!res) {
                    if (callback) {
                        callback (NULL, NULL, NULL, user_data);
                    }
                    return strdup (path);
                }
            }
        }
    }

    queue_add (fname, artist, album, size, callback, user_data);
    return NULL;
}

static void
sync_callback (const char *fname, const char *artist, const char *album, void *user_data) {
    mutex_cond_t *mc = (mutex_cond_t *)user_data;
    deadbeef->mutex_lock (mc->mutex);
    deadbeef->cond_signal (mc->cond);
    deadbeef->mutex_unlock (mc->mutex);
}

static char*
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

static void
artwork_reset (int fast) {
    if (fast) {
//        if (current_file) {
//            deadbeef->fabort (current_file);
//        }
        deadbeef->mutex_lock (mutex);
        while (queue && queue->next) {
            cover_query_t *next = queue->next->next;
            free (queue->next->fname);
            free (queue->next->artist);
            free (queue->next->album);
            for (int i = 0; i < queue->next->numcb; i++) {
                if (queue->next->callbacks[i].cb == sync_callback) {
                    sync_callback (NULL, NULL, NULL, queue->next->callbacks[i].ud);
                }
            }
            queue->next = next;
            if (next == NULL) {
                queue_tail = queue;
            }
        }
        deadbeef->mutex_unlock (mutex);
    }
    else {
        trace ("artwork: reset\n");
        clear_queue = 1;
        deadbeef->cond_signal (cond);
        trace ("artwork: waiting for clear to complete\n");
        while (clear_queue) {
            usleep (100000);
        }
    }
}

static int
artwork_configchanged (void) {
    int new_artwork_enable_embedded = deadbeef->conf_get_int ("artwork.enable_embedded", 1);
    int new_artwork_enable_local = deadbeef->conf_get_int ("artwork.enable_localfolder", 1);
#ifdef USE_VFS_CURL
    int new_artwork_enable_lfm = deadbeef->conf_get_int ("artwork.enable_lastfm", 0);
    int new_artwork_enable_aao = deadbeef->conf_get_int ("artwork.enable_albumartorg", 0);
    int new_artwork_enable_wos = deadbeef->conf_get_int ("artwork.enable_wos", 0);
#endif

    char new_artwork_filemask[200];
    deadbeef->conf_get_str ("artwork.filemask", DEFAULT_FILEMASK, new_artwork_filemask, sizeof (new_artwork_filemask));

    if (new_artwork_enable_embedded != artwork_enable_embedded
            || new_artwork_enable_local != artwork_enable_local
#ifdef USE_VFS_CURL
            || new_artwork_enable_lfm != artwork_enable_lfm
            || new_artwork_enable_aao != artwork_enable_aao
            || new_artwork_enable_wos != artwork_enable_wos
#endif
            || strcmp (new_artwork_filemask, artwork_filemask)) {
        trace ("artwork config changed, invalidating cache...\n");
        artwork_enable_embedded = new_artwork_enable_embedded;
        artwork_enable_local = new_artwork_enable_local;
#ifdef USE_VFS_CURL
        artwork_enable_lfm = new_artwork_enable_lfm;
        artwork_enable_aao = new_artwork_enable_aao;
        artwork_enable_wos = new_artwork_enable_wos;
#endif
        artwork_reset_time = time (NULL);
        strcpy (artwork_filemask, new_artwork_filemask);
        deadbeef->conf_set_int64 ("artwork.cache_reset_time", artwork_reset_time);
        artwork_reset (0);
        deadbeef->sendmessage (DB_EV_PLAYLIST_REFRESH, 0, 0, 0);
    }

    return 0;
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
artwork_plugin_start (void)
{
    deadbeef->conf_lock ();

    const char *def_art = deadbeef->conf_get_str_fast ("gtkui.nocover_pixmap", NULL);
    if (!def_art) {
        snprintf (default_cover, sizeof (default_cover), "%s/noartwork.png", deadbeef->get_pixmap_dir ());
    }
    else {
        strcpy (default_cover, def_art);
    }
    terminate = 0;

    artwork_enable_embedded = deadbeef->conf_get_int ("artwork.enable_embedded", 1);
    artwork_enable_local = deadbeef->conf_get_int ("artwork.enable_localfolder", 1);
#ifdef USE_VFS_CURL
    artwork_enable_lfm = deadbeef->conf_get_int ("artwork.enable_lastfm", 0);
    artwork_enable_aao = deadbeef->conf_get_int ("artwork.enable_albumartorg", 0);
    artwork_enable_wos = deadbeef->conf_get_int ("artwork.enable_wos", 0);
#endif
    artwork_reset_time = deadbeef->conf_get_int64 ("artwork.cache_reset_time", 0);

    deadbeef->conf_get_str ("artwork.filemask", DEFAULT_FILEMASK, artwork_filemask, sizeof (artwork_filemask));

    deadbeef->conf_unlock ();

    artwork_filemask[sizeof(artwork_filemask)-1] = 0;

    mutex = deadbeef->mutex_create_nonrecursive ();
#ifdef USE_IMLIB2
    imlib_mutex = deadbeef->mutex_create_nonrecursive ();
#endif
    cond = deadbeef->cond_create ();
    tid = deadbeef->thread_start_low_priority (fetcher_thread, NULL);

    return 0;
}

static int
artwork_plugin_stop (void)
{
    if (current_file) {
        deadbeef->fabort (current_file);
    }
    if (tid) {
        terminate = 1;
        deadbeef->cond_signal (cond);
        deadbeef->thread_join (tid);
        tid = 0;
    }
    while (queue) {
        queue_pop ();
    }
    if (mutex) {
        deadbeef->mutex_free (mutex);
        mutex = 0;
    }
#ifdef USE_IMLIB2
    if (imlib_mutex) {
        deadbeef->mutex_free (imlib_mutex);
        imlib_mutex = 0;
    }
#endif
    if (cond) {
        deadbeef->cond_free (cond);
        cond = 0;
    }

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
    .get_album_art = get_album_art,
    .reset = artwork_reset,
    .get_default_cover = get_default_cover,
    .get_album_art_sync = get_album_art_sync,
    .make_cache_path = make_cache_path,
    .make_cache_path2 = make_cache_path2,
};

DB_plugin_t *
artwork_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}

