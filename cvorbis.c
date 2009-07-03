#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>
#include <string.h>
#include <stdio.h>
#include "codec.h"
#include "cvorbis.h"

static FILE *file;
static OggVorbis_File vorbis_file;
static vorbis_info *vi;
static int cur_bit_stream;

int cvorbis_init (const char *fname) {
    file = NULL;
    vi = NULL;
    cur_bit_stream = -1;

    file = fopen (fname, "rb");
    if (!file) {
        return -1;
    }

    memset (&cvorbis.info, 0, sizeof (fileinfo_t));
    ov_open (file, &vorbis_file, NULL, 0);
    vi = ov_info (&vorbis_file, -1);
    cvorbis.info.bitsPerSample = 16;
    cvorbis.info.dataSize = ov_pcm_total (&vorbis_file, -1) * vi->channels * 2;
    cvorbis.info.channels = vi->channels;
    cvorbis.info.samplesPerSecond = vi->rate;
    return 0;
}

void
cvorbis_free (void) {
    fclose (file);
    ov_clear (&vorbis_file);
    vi = NULL;
}

int
cvorbis_read (char *bytes, int size)
{
    if (!file)
        return -1;
    printf ("vorbis read %d bytes!\n", size);
    for (;;)
    {
        // read ogg
        long ret=ov_read (&vorbis_file, bytes, size, 0, 2, 1, &cur_bit_stream);
        if (ret < 0)
        {
            printf ("WARNING: ogg vorbis decoder tells error %x\n", ret);
            memset (bytes, 0, size);
            return -1;
        }
        else if (ret < size)
        {
            if (ret == 0) {
                ov_raw_seek (&vorbis_file, 0);
            }
            size -= ret;
            bytes += ret;
        }
        else if (ret > size) {
            printf ("read more than requested!\n");
            break;
        }
        else {
            break;
        }
    }
    return 0;
}

codec_t cvorbis = {
    .init = cvorbis_init,
    .free = cvorbis_free,
    .read = cvorbis_read
};

