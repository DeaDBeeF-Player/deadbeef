#ifndef _ASF_H
#define _ASF_H

#include <inttypes.h>
#include <deadbeef/deadbeef.h>

/* ASF codec IDs */
#define ASF_CODEC_ID_WMAV1      0x160
#define ASF_CODEC_ID_WMAV2      0x161
#define ASF_CODEC_ID_WMAPRO     0x162
#define ASF_CODEC_ID_WMAVOICE   0x00A

enum asf_error_e {
    ASF_ERROR_INTERNAL       = -1,  /* incorrect input to API calls */
    ASF_ERROR_OUTOFMEM       = -2,  /* some malloc inside program failed */
    ASF_ERROR_EOF            = -3,  /* unexpected end of file */
    ASF_ERROR_IO             = -4,  /* error reading or writing to file */
    ASF_ERROR_INVALID_LENGTH = -5,  /* length value conflict in input data */
    ASF_ERROR_INVALID_VALUE  = -6,  /* other value conflict in input data */
    ASF_ERROR_INVALID_OBJECT = -7,  /* ASF object missing or in wrong place */
    ASF_ERROR_OBJECT_SIZE    = -8,  /* invalid ASF object size (too small) */
    ASF_ERROR_SEEKABLE       = -9,  /* file not seekable */
    ASF_ERROR_SEEK           = -10,  /* file is seekable but seeking failed */
    ASF_ERROR_ENCRYPTED      = -11  /* file is encrypted */
};

struct asf_waveformatex_s {
    uint32_t packet_size;
    uint32_t max_packet_size;
    int audiostream;
    uint16_t codec_id;
    uint16_t channels;
    uint32_t rate;
    uint32_t bitrate;
    uint16_t blockalign;
    uint16_t bitspersample;
    uint16_t datalen;
    uint64_t numpackets;
    uint8_t data[46];
    uint64_t play_duration;
    uint64_t send_duration;
    uint64_t preroll;
    uint32_t flags;
    int32_t first_frame_timestamp;
};
typedef struct asf_waveformatex_s asf_waveformatex_t;

int asf_read_packet(uint8_t** audiobuf, int* audiobufsize, int* packetlength, 
                    asf_waveformatex_t* wfx, DB_FILE *fp);
                    
int asf_get_timestamp(int *duration, DB_FILE *fp);

int asf_seek(int ms, asf_waveformatex_t* wfx, DB_FILE *fp, int64_t first_frame_offset, int *skip_ms);

#endif /* _ASF_H */
