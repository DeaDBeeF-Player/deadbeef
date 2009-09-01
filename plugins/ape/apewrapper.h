/*
  apedec - Monkey's Audio Decoder plugin for DeaDBeeF player
  http://deadbeef.sourceforge.net

  Copyright (C) 2009 Alexey Yakovenko

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

  Note: DeaDBeeF player itself uses different license
*/
#ifndef __APEWRAPPER_H
#define __APEWRAPPER_H

#ifdef __cplusplus
extern "C" {
#endif

// decode process:
// 1. get input format:
//    WAVEFORMATEX wfe;
//    GetInfo(APE_INFO_WAVEFORMATEX, (intptr_t)&wfe)
// 2. get wav header
//    int size = GetInfo(APE_INFO_WAV_HEADER_BYTES)
//    char buf[size];
//    GetInfo (APE_INFO_WAV_HEADER_DATA, (intptr_t)buf, size);
// 3. allocate space for readbuffer
//    int bufsize = GetInfo(APE_INFO_BLOCK_ALIGN) * BLOCKS_PER_DECODE;
//    char readbuf[bufsize];
// 4. get total number of blocks
//    int blocksleft = GetInfo(APE_DECOMPRESS_TOTAL_BLOCKS);
// 5. decompress
//    while (blocksleft > 0) {
//      int ndecoded;
//      GetData (readbuf, BLOCKS_PER_DECODE, &ndecoded);
//      nblocksleft -= ndecoded;
//    }
// 6. terminate output
//    if (GetInfo(APE_INFO_WAV_TERMINATING_BYTES) > 0) {
//      GetInfo(APE_INFO_WAV_TERMINATING_DATA, (intptr_t)readbuf, GetInfo(APE_INFO_WAV_TERMINATING_BYTES));
//    }
#ifndef APE_MACLIB_H

typedef struct tWAVEFORMATEX
{
    uint16_t        wFormatTag;         /* format type */
    uint16_t        nChannels;          /* number of channels (i.e. mono, stereo...) */
    uint32_t       nSamplesPerSec;     /* sample rate */
    uint32_t       nAvgBytesPerSec;    /* for buffer estimation */
    uint16_t        nBlockAlign;        /* block size of data */
    uint16_t        wBitsPerSample;     /* number of bits per sample of mono data */
    uint16_t        cbSize;             /* the count in bytes of the size of */
                    /* extra information (after cbSize) */
} WAVEFORMATEX;

enum APE_DECOMPRESS_FIELDS
{
    APE_INFO_FILE_VERSION = 1000,               // version of the APE file * 1000 (3.93 = 3930) [ignored, ignored]
    APE_INFO_COMPRESSION_LEVEL = 1001,          // compression level of the APE file [ignored, ignored]
    APE_INFO_FORMAT_FLAGS = 1002,               // format flags of the APE file [ignored, ignored]
    APE_INFO_SAMPLE_RATE = 1003,                // sample rate (Hz) [ignored, ignored]
    APE_INFO_BITS_PER_SAMPLE = 1004,            // bits per sample [ignored, ignored]
    APE_INFO_BYTES_PER_SAMPLE = 1005,           // number of bytes per sample [ignored, ignored]
    APE_INFO_CHANNELS = 1006,                   // channels [ignored, ignored]
    APE_INFO_BLOCK_ALIGN = 1007,                // block alignment [ignored, ignored]
    APE_INFO_BLOCKS_PER_FRAME = 1008,           // number of blocks in a frame (frames are used internally)  [ignored, ignored]
    APE_INFO_FINAL_FRAME_BLOCKS = 1009,         // blocks in the final frame (frames are used internally) [ignored, ignored]
    APE_INFO_TOTAL_FRAMES = 1010,               // total number frames (frames are used internally) [ignored, ignored]
    APE_INFO_WAV_HEADER_BYTES = 1011,           // header bytes of the decompressed WAV [ignored, ignored]
    APE_INFO_WAV_TERMINATING_BYTES = 1012,      // terminating bytes of the decompressed WAV [ignored, ignored]
    APE_INFO_WAV_DATA_BYTES = 1013,             // data bytes of the decompressed WAV [ignored, ignored]
    APE_INFO_WAV_TOTAL_BYTES = 1014,            // total bytes of the decompressed WAV [ignored, ignored]
    APE_INFO_APE_TOTAL_BYTES = 1015,            // total bytes of the APE file [ignored, ignored]
    APE_INFO_TOTAL_BLOCKS = 1016,               // total blocks of audio data [ignored, ignored]
    APE_INFO_LENGTH_MS = 1017,                  // length in ms (1 sec = 1000 ms) [ignored, ignored]
    APE_INFO_AVERAGE_BITRATE = 1018,            // average bitrate of the APE [ignored, ignored]
    APE_INFO_FRAME_BITRATE = 1019,              // bitrate of specified APE frame [frame index, ignored]
    APE_INFO_DECOMPRESSED_BITRATE = 1020,       // bitrate of the decompressed WAV [ignored, ignored]
    APE_INFO_PEAK_LEVEL = 1021,                 // peak audio level (obsolete) (-1 is unknown) [ignored, ignored]
    APE_INFO_SEEK_BIT = 1022,                   // bit offset [frame index, ignored]
    APE_INFO_SEEK_BYTE = 1023,                  // byte offset [frame index, ignored]
    APE_INFO_WAV_HEADER_DATA = 1024,            // error code [buffer *, max bytes]
    APE_INFO_WAV_TERMINATING_DATA = 1025,       // error code [buffer *, max bytes]
    APE_INFO_WAVEFORMATEX = 1026,               // error code [waveformatex *, ignored]
    APE_INFO_IO_SOURCE = 1027,                  // I/O source (CIO *) [ignored, ignored]
    APE_INFO_FRAME_BYTES = 1028,                // bytes (compressed) of the frame [frame index, ignored]
    APE_INFO_FRAME_BLOCKS = 1029,               // blocks in a given frame [frame index, ignored]
    APE_INFO_TAG = 1030,                        // point to tag (CAPETag *) [ignored, ignored]
    
    APE_DECOMPRESS_CURRENT_BLOCK = 2000,        // current block location [ignored, ignored]
    APE_DECOMPRESS_CURRENT_MS = 2001,           // current millisecond location [ignored, ignored]
    APE_DECOMPRESS_TOTAL_BLOCKS = 2002,         // total blocks in the decompressors range [ignored, ignored]
    APE_DECOMPRESS_LENGTH_MS = 2003,            // total blocks in the decompressors range [ignored, ignored]
    APE_DECOMPRESS_CURRENT_BITRATE = 2004,      // current bitrate [ignored, ignored]
    APE_DECOMPRESS_AVERAGE_BITRATE = 2005,      // average bitrate (works with ranges) [ignored, ignored]

    APE_INTERNAL_INFO = 3000                    // for internal use -- don't use (returns APE_FILE_INFO *) [ignored, ignored]
};
#endif

void *
ape_decompress_create (const char *fname);

void
ape_decompress_destroy (void *d);

int
ape_decompress_get_info_int (void *d, int id);

int
ape_decompress_get_info_data (void *d, int id, void *ptr);

int
ape_decompress_get_info_data_sized (void *d, int id, void *ptr, int size);

int
ape_decompress_getdata (void *d, char *buffer, int nblocks);

int
ape_decompress_seek (void *d, int nblockoffs);

#ifdef __cplusplus
}
#endif

#endif // __APEWRAPPER_H
