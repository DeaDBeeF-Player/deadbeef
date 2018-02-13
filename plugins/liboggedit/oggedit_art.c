/*
  This file is part of Deadbeef Player source code
  http://deadbeef.sourceforge.net

  DeaDBeeF Ogg Edit library album art functions

  Copyright (C) 2014 Ian Nartowicz <deadbeef@nartowicz.co.uk>

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
#include <config.h>
#endif
#include <stdlib.h>
#include <string.h>
#include "oggedit_internal.h"

const char *oggedit_album_art_type(const uint32_t type)
{
    switch (type) {
        case 1:
            return "32x32 pixels file icon";
        case 2:
            return "other file icon";
        case 3:
            return "front cover";
        case 4:
            return "back cover";
        case 5:
            return "leaflet page";
        case 6:
            return "media";
        case 7:
            return "lead artist/lead performer/soloist";
        case 8:
            return "artist/performer";
        case 9:
            return "conductor";
        case 10:
            return "band/orchestra";
        case 11:
            return "composer";
        case 12:
            return "lyricist/text writer";
        case 13:
            return "recording location";
        case 14:
            return "during recording";
        case 15:
            return "during performance";
        case 16:
            return "movie/video screen capture";
        case 17:
            return "bright coloured fish";
        case 18:
            return "illustration";
        case 19:
            return "band/artist logotype";
        case 20:
            return "publisher/studio logotype";
        default:
            return "other";
    }
}

static char *btoa(const unsigned char *binary, const size_t binary_length)
{
    const char b64[64] = {
        'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
        'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z',
        '0','1','2','3','4','5','6','7','8','9','+','/'
    };

    char *ascii_base64 = malloc((binary_length-1)/3 * 4 + 5);
    if (!ascii_base64)
        return NULL;

    char *p = ascii_base64;
    const unsigned char *q = binary;
    const unsigned char *binary_end = binary + binary_length;
    while (q+2 < binary_end) {
        uint32_t chunk = q[0]<<16 | q[1]<<8 | q[2];
        p[3] = b64[chunk & 0x3f];
        p[2] = b64[chunk>>6 & 0x3f];
        p[1] = b64[chunk>>12 & 0x3f];
        p[0] = b64[chunk>>18 & 0x3f];
        p+=4, q+=3;
    }

    if (q < binary_end) {
        uint16_t remainder = q[0]<<8 | (q+1 == binary_end ? '\0' : q[1]);
        p[3] = '=';
        p[2] = q+1 == binary_end ? '=' : b64[remainder<<2 & 0x3f];
        p[1] = b64[remainder>>4 & 0x3f];
        p[0] = b64[remainder>>10 & 0x3f];
        p+=4;
    }
    *p = '\0';

    return ascii_base64;
}

char *oggedit_album_art_tag(DB_FILE *fp, int *res)
{
    if (!fp) {
        *res = OGGEDIT_FILE_NOT_OPEN;
        return NULL;
    }

    const int64_t data_length = fp->vfs->getlength(fp);
    if (data_length < 50 || data_length > 10000000) {
        fp->vfs->close(fp);
        *res = OGGEDIT_BAD_FILE_LENGTH;
        return NULL;
    }

    char *data = malloc(data_length);
    if (!data) {
        fp->vfs->close(fp);
        *res = OGGEDIT_ALLOCATION_FAILURE;
        return NULL;
    }

    const size_t data_read = fp->vfs->read(data, 1, data_length, fp);
    fp->vfs->close(fp);
    if (data_read != data_length) {
        free(data);
        *res = OGGEDIT_CANT_READ_IMAGE_FILE;
        return NULL;
    }

    oggpack_buffer opb;
    oggpackB_writeinit(&opb);
    oggpackB_write(&opb, 3, 32);
    _oggpackB_string(&opb, memcmp(data, "\x89PNG\x0D\x0A\x1A\x0A", 8) ? "image/jpeg" : "image/png");
    _oggpackB_string(&opb, "Album art added from DeaDBeeF");
    oggpackB_write(&opb, 1, 32); /* workaround for opusfile bug */
    oggpackB_write(&opb, 1, 32);
    oggpackB_write(&opb, 1, 32);
    oggpackB_write(&opb, 0, 32);
    oggpackB_write(&opb, data_length, 32);
    _oggpack_chars(&opb, data, data_length);
    free(data);
    if (oggpack_writecheck(&opb)) {
        *res = OGGEDIT_ALLOCATION_FAILURE;
        return NULL;
    }

    char *tag = btoa(oggpackB_get_buffer(&opb), oggpackB_bytes(&opb));
    oggpackB_writeclear(&opb);
    return tag;
}
