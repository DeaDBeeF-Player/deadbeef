/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2010 Alexey Yakovenko <waker@users.sourceforge.net>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <string.h>
#include <stdio.h>
#include "codec.h"
#include "cwav.h"

typedef struct {
    uint32_t riffMagic;
    uint32_t size;
    uint32_t waveMagic;
    uint32_t fmtMagic;
    uint32_t chunkSize;
    uint16_t formatTag;
    uint16_t channels;
    uint32_t samplesPerSecond;
    uint32_t bytesPerSecond;
    uint16_t blockAlignment;
    uint16_t bitsPerSample;
    uint32_t dataMagic;
    uint32_t dataSize;
} wavHeader_t;

static FILE *file;

int cwav_init (const char *fname, int track, float start, float end) {

    file = fopen (fname, "rb");
    if (!file) {
        return -1;
    }

    wavHeader_t header;

    // read WAV header
    if (fread (&header, sizeof (header), 1, file) != 1)
    {
        printf ("WARNING: WAV header not found in %s\n", fname);
        fclose (file);
        file = NULL;
        return -1;
    }
    cwav.info.bitsPerSample = header.bitsPerSample;
    cwav.info.channels = header.channels;
    //cwav.info.dataSize = header.dataSize;
    cwav.info.samplesPerSecond = header.samplesPerSecond;

    return 0;
}

void
cwav_free (void)
{
    if (file)
    {
        fclose (file);
        file = NULL;
    }
}

int
cwav_read (char *bytes, int size) {
    int bytesread = 0;
    int first = 1;
    for (;;) {
        int bytesread = fread (bytes, 1, size, file);
        if (bytesread == size) {
            break;
        }
        size -= bytesread;
        bytes += bytesread;
        fseek (file, sizeof (wavHeader_t), SEEK_SET);
    }
    return 0;
}

codec_t cwav = {
    .init = cwav_init,
    .free = cwav_free,
    .read = cwav_read
};

