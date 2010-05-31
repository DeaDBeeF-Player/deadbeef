/*
 * ttadec.h
 *
 * Description:	 TTAv1 decoder definitions and prototypes
 * Developed by: Alexander Djourik <ald@true-audio.com>
 *               Pavel Zhilin <pzh@true-audio.com>
 *
 * Copyright (c) 2004 True Audio Software. All rights reserved.
 *
 */

/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the True Audio Software nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef TTADEC_H_
#define TTADEC_H_

#ifdef _WIN32
#pragma pack(1)
#define __ATTRIBUTE_PACKED__
#else
#define __ATTRIBUTE_PACKED__	__attribute__((packed))
#endif

#define TTA1_SIGN	0x31415454
#define FRAME_TIME	1.04489795918367346939
#define MAX_ORDER	8

#ifndef WAVE_FORMAT_PCM
#define WAVE_FORMAT_PCM	1
#endif

#ifdef _WIN32
	typedef unsigned __int64 uint64;
#else
	typedef unsigned long long uint64;
#endif

const unsigned int crc32_table[256] = {
	0x00000000, 0x77073096, 0xee0e612c, 0x990951ba,
	0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
	0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
	0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
	0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de,
	0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
	0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec,
	0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
	0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
	0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
	0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940,
	0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
	0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116,
	0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
	0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
	0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
	0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a,
	0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
	0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818,
	0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
	0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
	0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
	0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c,
	0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
	0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2,
	0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
	0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
	0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
	0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086,
	0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
	0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4,
	0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
	0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
	0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
	0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8,
	0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
	0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe,
	0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
	0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
	0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
	0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252,
	0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
	0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60,
	0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
	0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
	0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
	0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04,
	0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
	0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a,
	0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
	0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
	0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
	0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e,
	0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
	0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c,
	0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
	0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
	0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
	0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0,
	0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
	0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6,
	0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
	0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
	0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
}; 

const unsigned int bit_mask[] = {
	0x00000000, 0x00000001, 0x00000003, 0x00000007,
	0x0000000f, 0x0000001f, 0x0000003f, 0x0000007f,
	0x000000ff, 0x000001ff, 0x000003ff, 0x000007ff,
	0x00000fff, 0x00001fff, 0x00003fff, 0x00007fff,
	0x0000ffff, 0x0001ffff, 0x0003ffff, 0x0007ffff,
	0x000fffff, 0x001fffff, 0x003fffff, 0x007fffff,
	0x00ffffff, 0x01ffffff, 0x03ffffff, 0x07ffffff,
	0x0fffffff, 0x1fffffff, 0x3fffffff, 0x7fffffff,
	0xffffffff
};

const unsigned int bit_shift[] = {
	0x00000001, 0x00000002, 0x00000004, 0x00000008,
	0x00000010, 0x00000020, 0x00000040, 0x00000080,
	0x00000100, 0x00000200, 0x00000400, 0x00000800,
	0x00001000, 0x00002000, 0x00004000, 0x00008000,
	0x00010000, 0x00020000, 0x00040000, 0x00080000,
	0x00100000, 0x00200000, 0x00400000, 0x00800000,
	0x01000000, 0x02000000, 0x04000000, 0x08000000,
	0x10000000, 0x20000000, 0x40000000, 0x80000000,
	0x80000000, 0x80000000, 0x80000000, 0x80000000,
	0x80000000, 0x80000000, 0x80000000, 0x80000000
};

const unsigned int *shift_16 = bit_shift + 4;

typedef unsigned char byte;

#ifdef _BIG_ENDIAN
#define	ENDSWAP_INT16(x)	(((((x)>>8)&0xFF)|(((x)&0xFF)<<8)))
#define	ENDSWAP_INT32(x)	(((((x)>>24)&0xFF)|(((x)>>8)&0xFF00)|(((x)&0xFF00)<<8)|(((x)&0xFF)<<24)))
#define WRITE_BUFFER(x, bsize, out) { \
	if (bsize > 2) *out++ = (byte)(*x >> 16); \
	if (bsize > 1) *out++ = (byte)(*x >> 8); \
	*out++ = (byte) *x; }
#else
#define	ENDSWAP_INT16(x)	(x)
#define	ENDSWAP_INT32(x)	(x)
#define WRITE_BUFFER(x, bsize, out) { \
	*out++ = (byte) *x; \
	if (bsize > 1) *out++ = (byte)(*x >> 8); \
	if (bsize > 2) *out++ = (byte)(*x >> 16); }
#endif

#define PREDICTOR1(x, k)	((int)((((uint64)x << k) - x) >> k))
#define DEC(x)			(((x)&1)?(++(x)>>1):(-(x)>>1))

typedef struct {
	unsigned int TTAid;
	unsigned short AudioFormat;
	unsigned short NumChannels;
	unsigned short BitsPerSample;
	unsigned int SampleRate;
	unsigned int DataLength;
	unsigned int CRC32;
} __ATTRIBUTE_PACKED__ tta_hdr;

typedef struct {
	unsigned int k0;
	unsigned int k1;
	unsigned int sum0;
	unsigned int sum1;
} adapt;

typedef struct {
	int shift;
	int round;
	int error;
	int mutex;
	int qm[MAX_ORDER+1];
	int dx[MAX_ORDER+1];
	int dl[MAX_ORDER+1];
} fltst;

typedef struct {
	fltst fst;
	adapt rice;
	int last;
} decoder;

/*****************************************************************
 *            ID3 reader definitions and prototypes              *
 *****************************************************************/

#define ID3_VERSION 3

/* ID3 common headers set */

#define TIT2    1
#define TPE1    2
#define TALB    3
#define TRCK    4
#define TYER    5
#define TCON    6
#define COMM    7

/* ID3 tag checked flags */

#define ID3_UNSYNCHRONISATION_FLAG      0x80
#define ID3_EXTENDEDHEADER_FLAG         0x40
#define ID3_EXPERIMENTALTAG_FLAG        0x20
#define ID3_FOOTERPRESENT_FLAG          0x10

/* ID3 frame checked flags */

#define FRAME_COMPRESSION_FLAG          0x0008
#define FRAME_ENCRYPTION_FLAG           0x0004
#define FRAME_UNSYNCHRONISATION_FLAG    0x0002

/* ID3 field text encoding */

#define FIELD_TEXT_ISO_8859_1           0x00
#define FIELD_TEXT_UTF_16               0x01
#define FIELD_TEXT_UTF_16BE             0x02
#define FIELD_TEXT_UTF_8                0x03

typedef struct {
	unsigned char  id[3];
	unsigned char  title[30];
	unsigned char  artist[30];
	unsigned char  album[30];
	unsigned char  year[4];
	unsigned char  comment[28];
	unsigned char  zero;
	unsigned char  track;
	unsigned char  genre;
} __ATTRIBUTE_PACKED__ id3v1_tag;

typedef struct {
	unsigned char  id[3];
	unsigned short version;
	unsigned char  flags;
	unsigned char  size[4];
} __ATTRIBUTE_PACKED__ id3v2_tag;

typedef struct {
	unsigned char  id[4];
	unsigned char  size[4];
	unsigned short flags;
} __ATTRIBUTE_PACKED__ id3v2_frame;

static char *genre[] = {
    "Blues",  "Classic Rock",  "Country", "Dance", "Disco", "Funk",
    "Grunge",  "Hip-Hop",  "Jazz",  "Metal",  "New Age",  "Oldies",
    "Other",  "Pop",  "R&B",  "Rap",  "Reggae",  "Rock",  "Techno",
    "Industrial",  "Alternative",  "Ska",  "Death Metal", "Pranks",
    "Soundtrack",  "Euro-Techno",  "Ambient",  "Trip-Hop", "Vocal",
    "Jazz+Funk",  "Fusion",  "Trance", "Classical", "Instrumental",
    "Acid",   "House",  "Game",  "Sound Clip",  "Gospel",  "Noise",
    "AlternRock",  "Bass",  "Soul",  "Punk", "Space", "Meditative",
    "Instrumental Pop",  "Instrumental Rock",  "Ethnic",  "Gothic",
    "Darkwave",   "Techno-Industrial",   "Electronic",  "Pop-Folk",
    "Eurodance",   "Dream",   "Southern Rock",   "Comedy",  "Cult",
    "Gangsta",  "Top 40",  "Christian Rap",  "Pop/Funk",  "Jungle",
    "Native American",    "Cabaret",   "New Wave",   "Psychedelic",
    "Rave",  "Showtunes", "Trailer", "Lo-Fi", "Tribal","Acid Punk",
    "Acid Jazz",   "Polka",   "Retro",   "Musical",  "Rock & Roll",
    "Hard Rock",  "Folk",  "Folk/Rock",  "National Folk",  "Swing",
    "Fast-Fusion",    "Bebob",    "Latin",   "Revival",   "Celtic",
    "Bluegrass",  "Avantgarde",  "Gothic Rock", "Progressive Rock",
    "Psychedelic Rock",  "Symphonic Rock", "Slow Rock", "Big Band",
    "Chorus",  "Easy Listening",  "Acoustic",  "Humour",  "Speech",
    "Chanson",   "Opera",  "Chamber Music",  "Sonata",  "Symphony",
    "Booty Bass",  "Primus",  "Porn Groove",  "Satire", "Slow Jam",
    "Club", "Tango", "Samba", "Folklore", "Ballad", "Power Ballad",
    "Rhythmic Soul", "Freestyle", "Duet", "Punk Rock", "Drum Solo",
    "A Cappella", "Euro-House", "Dance Hall", "Goa", "Drum & Bass",
    "Club-House",   "Hardcore",   "Terror",   "Indie",   "BritPop",
    "Negerpunk",   "Polsk Punk",  "Beat",  "Christian Gangsta Rap",
    "Heavy Metal",            "Black Metal",           "Crossover",
    "Contemporary Christian",     "Christian Rock",     "Merengue",
    "Salsa",    "Thrash Metal",    "Anime",   "JPop",   "Synthpop"
};

#define GENRES	(sizeof genre / sizeof genre[0])

#endif /* TTADEC_H_ */
