/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009  Alexey Yakovenko

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
#include "junklib.h"
#include <iconv.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "playlist.h"
#include "utf8.h"

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

static uint32_t
extract_i32 (unsigned char *buf)
{
    uint32_t x;
    // big endian extract

    x = buf[0];
    x <<= 8;
    x |= buf[1];
    x <<= 8;
    x |= buf[2];
    x <<= 8;
    x |= buf[3];

    return x;
}

static inline uint32_t
extract_i32_le (unsigned char *buf)
{
    uint32_t x;
    // little endian extract

    x = buf[3];
    x <<= 8;
    x |= buf[2];
    x <<= 8;
    x |= buf[1];
    x <<= 8;
    x |= buf[0];

    return x;
}
static inline uint16_t
extract_i16 (unsigned char *buf)
{
    uint16_t x;
    // big endian extract

    x = buf[0];
    x <<= 8;
    x |= buf[1];
    x <<= 8;

    return x;
}

static inline float
extract_f32 (unsigned char *buf) {
    float f;
    uint32_t *x = (uint32_t *)&f;
    *x = buf[0];
    *x <<= 8;
    *x |= buf[1];
    *x <<= 8;
    *x |= buf[2];
    *x <<= 8;
    *x |= buf[3];
    return f;
}
static const char *junk_genretbl[] = {
    "Blues",
    "Classic Rock",
    "Country",
    "Dance",
    "Disco",
    "Funk",
    "Grunge",
    "Hip-Hop",
    "Jazz",
    "Metal",
    "New Age",
    "Oldies",
    "Other",
    "Pop",
    "R&B",
    "Rap",
    "Reggae",
    "Rock",
    "Techno",
    "Industrial",
    "Alternative",
    "Ska",
    "Death Metal",
    "Pranks",
    "Soundtrack",
    "Euro-Techno",
    "Ambient",
    "Trip-Hop",
    "Vocal",
    "Jazz+Funk",
    "Fusion",
    "Trance",
    "Classical",
    "Instrumental",
    "Acid",
    "House",
    "Game",
    "Sound Clip",
    "Gospel",
    "Noise",
    "AlternRock",
    "Bass",
    "Soul",
    "Punk",
    "Space",
    "Meditative",
    "Instrumental Pop",
    "Instrumental Rock",
    "Ethnic",
    "Gothic",
    "Darkwave",
    "Techno-Industrial",
    "Electronic",
    "Pop-Folk",
    "Eurodance",
    "Dream",
    "Southern Rock",
    "Comedy",
    "Cult",
    "Gangsta",
    "Top 40",
    "Christian Rap",
    "Pop/Funk",
    "Jungle",
    "Native American",
    "Cabaret",
    "New Wave",
    "Psychadelic",
    "Rave",
    "Showtunes",
    "Trailer",
    "Lo-Fi",
    "Tribal",
    "Acid Punk",
    "Acid Jazz",
    "Polka",
    "Retro",
    "Musical",
    "Rock & Roll",
    "Hard Rock",
    "Folk",
    "Folk-Rock",
    "National Folk",
    "Swing",
    "Fast Fusion",
    "Bebob",
    "Latin",
    "Revival",
    "Celtic",
    "Bluegrass",
    "Avantgarde",
    "Gothic Rock",
    "Progressive Rock",
    "Psychedelic Rock",
    "Symphonic Rock",
    "Slow Rock",
    "Big Band",
    "Chorus",
    "Easy Listening",
    "Acoustic",
    "Humour",
    "Speech",
    "Chanson",
    "Opera",
    "Chamber Music",
    "Sonata",
    "Symphony",
    "Booty Bass",
    "Primus",
    "Porn Groove",
    "Satire",
    "Slow Jam",
    "Club",
    "Tango",
    "Samba",
    "Folklore",
    "Ballad",
    "Power Ballad",
    "Rhythmic Soul",
    "Freestyle",
    "Duet",
    "Punk Rock",
    "Drum Solo",
    "Acapella",
    "Euro-House",
    "Dance Hall",
    "Goa",
    "Drum & Bass",
    "Club-House",
    "Hardcore",
    "Terror",
    "Indie",
    "BritPop",
    "Negerpunk",
    "Polsk Punk",
    "Beat",
    "Christian Gangsta",
    "Heavy Metal",
    "Black Metal",
    "Crossover",
    "Contemporary C",
    "Christian Rock",
    "Merengue",
    "Salsa",
    "Thrash Metal",
    "Anime",
    "JPop",
    "SynthPop",
};

static int
can_be_russian (const signed char *str) {
    int latin = 0;
    int rus = 0;
    for (; *str; str++) {
        if ((*str >= 'A' && *str <= 'Z')
                || *str >= 'a' && *str <= 'z') {
            latin++;
        }
        else if (*str < 0) {
            rus++;
        }
    }
    if (rus > latin/2) {
        return 1;
    }
    return 0;
}

static char *
convstr_id3v2_2to3 (const unsigned char* str, int sz) {
    static char out[2048];
    const char *enc = "iso8859-1";
    char *ret = out;

    // hack to add limited cp1251 recoding support

      if (*str == 1) {
        if (str[1] == 0xff && str[2] == 0xfe) {
            enc = "UCS-2LE";
        }
        else if (str[2] == 0xff && str[1] == 0xfe) {
            enc = "UCS-2BE";
        }
        else {
            fprintf (stderr, "invalid ucs-2 signature %x %x\n", (int)str[1], (int)str[2]);
            return NULL;
        }
    }
    else {
        if (can_be_russian (&str[1])) {
            enc = "cp1251";
        }
    }
    str++;
    sz--;
    iconv_t cd = iconv_open ("utf8", enc);
    if (!cd) {
//        fprintf (stderr, "unknown encoding: %s\n", enc);
        return NULL;
    }
    else {
        size_t inbytesleft = sz;
        size_t outbytesleft = 2047;
        char *pin = (char*)str;
        char *pout = out;
        memset (out, 0, sizeof (out));
        size_t res = iconv (cd, &pin, &inbytesleft, &pout, &outbytesleft);
        iconv_close (cd);
        ret = out;
    }
    return strdup (ret);
}

static char *
convstr_id3v2_4 (const unsigned char* str, int sz) {
    static char out[2048];
    const char *enc = "iso8859-1";
    char *ret = out;

    // hack to add limited cp1251 recoding support

    if (*str == 0) {
        // iso8859-1
        enc = "iso8859-1";
    }
    else if (*str == 3) {
        // utf8
        strncpy (out, str+1, 2047);
        sz--;
        out[min (sz, 2047)] = 0;
        return strdup (out);
    }
    else if (*str == 1) {
        enc = "UTF-16";
    }
    else if (*str == 2) {
        enc = "UTF-16BE";
    }
    else {
        if (can_be_russian (&str[1])) {
            enc = "cp1251";
        }
    }
    str++;
    sz--;
    iconv_t cd = iconv_open ("utf8", enc);
    if (!cd) {
        // printf ("unknown encoding: %s\n", enc);
        return NULL;
    }
    else {
        size_t inbytesleft = sz;
        size_t outbytesleft = 2047;
        char *pin = (char*)str;
        char *pout = out;
        memset (out, 0, sizeof (out));
        size_t res = iconv (cd, &pin, &inbytesleft, &pout, &outbytesleft);
        iconv_close (cd);
        ret = out;
    }
    //fprintf (stderr, "decoded %s\n", out+3);
    return strdup (ret);
}

static const char *
convstr_id3v1 (const char* str, int sz) {
    static char out[2048];
    int i;
    for (i = 0; i < sz; i++) {
        if (str[i] != ' ') {
            break;
        }
    }
    if (i == sz) {
        out[0] = 0;
        return out;
    }

    // check for utf8 (hack)
    iconv_t cd;
    cd = iconv_open ("utf8", "utf8");
    size_t inbytesleft = sz;
    size_t outbytesleft = 2047;
    char *pin = (char*)str;
    char *pout = out;
    memset (out, 0, sizeof (out));
    size_t res = iconv (cd, &pin, &inbytesleft, &pout, &outbytesleft);
    iconv_close (cd);
    if (res == 0) {
        strncpy (out, str, 2047);
        out[min (sz, 2047)] = 0;
        return out;
    }

    const char *enc = "iso8859-1";
    if (can_be_russian (str)) {
        enc = "cp1251";
    }
    cd = iconv_open ("utf8", enc);
    if (!cd) {
        // fprintf (stderr, "unknown encoding: %s\n", enc);
        return NULL;
    }
    else {
        size_t inbytesleft = sz;
        size_t outbytesleft = 2047;
        char *pin = (char*)str;
        char *pout = out;
        memset (out, 0, sizeof (out));
        size_t res = iconv (cd, &pin, &inbytesleft, &pout, &outbytesleft);
        iconv_close (cd);
    }
    return out;
}

static void
str_trim_right (uint8_t *str, int len) {
    uint8_t *p = str + len - 1;
    while (p >= str && *p <= 0x20) {
        p--;
    }
    p++;
    *p = 0;
}

// should read both id3v1 and id3v1.1
int
junk_read_id3v1 (playItem_t *it, FILE *fp) {
    if (!it || !fp) {
        fprintf (stderr, "bad call to junk_read_id3v1!\n");
        return -1;
    }
    uint8_t buffer[128];
    // try reading from end
    fseek (fp, -128, SEEK_END);
    if (fread (buffer, 1, 128, fp) != 128) {
        return -1;
    }
    if (strncmp (buffer, "TAG", 3)) {
        return -1; // no tag
    }
    char title[31];
    char artist[31];
    char album[31];
    char year[5];
    char comment[31];
    uint8_t genreid;
    uint8_t tracknum;
    const char *genre;
    memset (title, 0, 31);
    memset (artist, 0, 31);
    memset (album, 0, 31);
    memset (year, 0, 5);
    memset (comment, 0, 31);
    memcpy (title, &buffer[3], 30);
    str_trim_right (title, 30);
    memcpy (artist, &buffer[3+30], 30);
    str_trim_right (artist, 30);
    memcpy (album, &buffer[3+60], 30);
    str_trim_right (album, 30);
    memcpy (year, &buffer[3+90], 4);
    str_trim_right (year, 4);
    memcpy (comment, &buffer[3+94], 30);
    str_trim_right (comment, 30);
    genreid = buffer[3+124];
    tracknum = 0xff;
    if (comment[28] == 0) {
        tracknum = comment[29];
    }
//    255 = "None",
//    "CR" = "Cover" (id3v2)
//    "RX" = "Remix" (id3v2)

    if (genreid == 0xff) {
        genre = "None";
    }
    else if (genreid <= 147) {
        genre = junk_genretbl[genreid];
    }

    // add meta
//    fprintf (stderr, "%s - %s - %s - %s - %s - %s\n", title, artist, album, year, comment, genre);
    pl_add_meta (it, "title", convstr_id3v1 (title, strlen (title)));
    pl_add_meta (it, "artist", convstr_id3v1 (artist, strlen (artist)));
    pl_add_meta (it, "album", convstr_id3v1 (album, strlen (album)));
    pl_add_meta (it, "year", year);
    pl_add_meta (it, "comment", convstr_id3v1 (comment, strlen (comment)));
    pl_add_meta (it, "genre", convstr_id3v1 (genre, strlen (genre)));
    if (tracknum != 0xff) {
        char s[4];
        snprintf (s, 4, "%d", tracknum);
        pl_add_meta (it, "track", s);
    }

// FIXME: that should be accounted for
//    if (it->endoffset < 128) {
//        it->endoffset = 128;
//    }

    return 0;
}

int
junk_read_ape (playItem_t *it, FILE *fp) {
//    fprintf (stderr, "trying to read ape tag\n");
    // try to read footer, position must be already at the EOF right before
    // id3v1 (if present)
    uint8_t header[32];
    if (fseek (fp, -32, SEEK_CUR) == -1) {
        return -1; // something bad happened
    }

    if (fread (header, 1, 32, fp) != 32) {
        return -1; // something bad happened
    }
    if (strncmp (header, "APETAGEX", 8)) {
        return -1; // no ape tag here
    }

    // end of footer must be 0
//    if (memcmp (&header[24], "\0\0\0\0\0\0\0\0", 8)) {
//        fprintf (stderr, "bad footer\n");
//        return -1;
//    }

    uint32_t version = extract_i32_le (&header[8]);
    int32_t size = extract_i32_le (&header[12]);
    uint32_t numitems = extract_i32_le (&header[16]);
    uint32_t flags = extract_i32_le (&header[20]);

    //fprintf (stderr, "APEv%d, size=%d, items=%d, flags=%x\n", version, size, numitems, flags);
    // now seek to beginning of the tag (exluding header)
    if (fseek (fp, -size, SEEK_CUR) == -1) {
        fprintf (stderr, "failed to seek to tag start (-%d)\n", size);
        return -1;
    }
    int i;
    for (i = 0; i < numitems; i++) {
        uint8_t buffer[8];
        if (fread (buffer, 1, 8, fp) != 8) {
            return -1;
        }
        uint32_t itemsize = extract_i32_le (&buffer[0]);
        uint32_t itemflags = extract_i32_le (&buffer[4]);
        // read key until 0 (stupid and slow)
        char key[256];
        int keysize = 0;
        while (keysize <= 255) {
            if (fread (&key[keysize], 1, 1, fp) != 1) {
                return -1;
            }
            if (key[keysize] == 0) {
                break;
            }
            if (key[keysize] < 0x20) {
                return -1; // non-ascii chars and chars with codes 0..0x1f not allowed in ape item keys
            }
            keysize++;
        }
        key[255] = 0;
        // read value
        char value[itemsize+1];
        if (fread (value, 1, itemsize, fp) != itemsize) {
            return -1;
        }
        value[itemsize] = 0;
        if (!u8_valid (value, itemsize, NULL)) {
            strcpy (value, "<bad encoding>");
        }
        // add metainfo only if it's textual
        int valuetype = ((itemflags & (0x3<<1)) >> 1);
        if (valuetype == 0) {
            if (!strcasecmp (key, "artist")) {
                pl_add_meta (it, "artist", value);
            }
            else if (!strcasecmp (key, "title")) {
                pl_add_meta (it, "title", value);
            }
            else if (!strcasecmp (key, "album")) {
                pl_add_meta (it, "album", value);
            }
            else if (!strcasecmp (key, "track")) {
                pl_add_meta (it, "track", value);
            }
            else if (!strcasecmp (key, "year")) {
                pl_add_meta (it, "year", value);
            }
            else if (!strcasecmp (key, "genre")) {
                pl_add_meta (it, "genre", value);
            }
            else if (!strcasecmp (key, "comment")) {
                pl_add_meta (it, "genre", value);
            }
        }
    }

    return 0;
}

static void
id3v2_string_read (int version, uint8_t *out, int sz, int unsync, uint8_t **pread) {
    if (!unsync) {
        memcpy (out, *pread, sz);
        *pread += sz;
        out[sz] = 0;
        out[sz+1] = 0;
        return;
    }
    uint8_t prev = 0;
    while (sz > 0) {
        if (prev == 0xff && !*(*pread)) {
            prev = 0;
            (*pread)++;
            continue;
        }
        prev = *out = *(*pread);
        (*pread)++;
        out++;
        sz--;
    }
    *out = 0;
}

int
junk_get_leading_size (FILE *fp) {
    uint8_t header[10];
    int pos = ftell (fp);
    if (fread (header, 1, 10, fp) != 10) {
        fseek (fp, pos, SEEK_SET);
        return -1; // too short
    }
    fseek (fp, pos, SEEK_SET);
    if (strncmp (header, "ID3", 3)) {
        return -1; // no tag
    }
//    uint8_t version_major = header[3];
//    uint8_t version_minor = header[4];
//    if (version_major > 4 || version_major < 2) {
//        return -1; // unsupported
//    }
    uint8_t flags = header[5];
    if (flags & 15) {
        return -1; // unsupported
    }
//    int unsync = (flags & (1<<7)) ? 1 : 0;
//    int extheader = (flags & (1<<6)) ? 1 : 0;
//    int expindicator = (flags & (1<<5)) ? 1 : 0;
    int footerpresent = (flags & (1<<4)) ? 1 : 0;
    // check for bad size
    if ((header[9] & 0x80) || (header[8] & 0x80) || (header[7] & 0x80) || (header[6] & 0x80)) {
        return -1; // bad header
    }
    uint32_t size = (header[9] << 0) | (header[8] << 7) | (header[7] << 14) | (header[6] << 21);
    //fprintf (stderr, "junklib: leading junk size %d\n", size);
    return size + 10 + 10 * footerpresent;
}

int
junk_read_id3v2 (playItem_t *it, FILE *fp) {
    int title_added = 0;
    if (!it || !fp) {
        fprintf (stderr, "bad call to junk_read_id3v2!\n");
        return -1;
    }
    rewind (fp);
    uint8_t header[10];
    if (fread (header, 1, 10, fp) != 10) {
        return -1; // too short
    }
    if (strncmp (header, "ID3", 3)) {
        return -1; // no tag
    }
    uint8_t version_major = header[3];
    uint8_t version_minor = header[4];
    if (version_major > 4 || version_major < 2) {
//        fprintf (stderr, "id3v2.%d.%d is unsupported\n", version_major, version_minor);
        return -1; // unsupported
    }
    uint8_t flags = header[5];
    if (flags & 15) {
        return -1; // unsupported
    }
    int unsync = (flags & (1<<7)) ? 1 : 0;
    int extheader = (flags & (1<<6)) ? 1 : 0;
    int expindicator = (flags & (1<<5)) ? 1 : 0;
    int footerpresent = (flags & (1<<4)) ? 1 : 0;
    // check for bad size
    if ((header[9] & 0x80) || (header[8] & 0x80) || (header[7] & 0x80) || (header[6] & 0x80)) {
        return -1; // bad header
    }
    uint32_t size = (header[9] << 0) | (header[8] << 7) | (header[7] << 14) | (header[6] << 21);
    // FIXME: that should be accounted for
//    int startoffset = size + 10 + 10 * footerpresent;
//    if (startoffset > it->startoffset) {
//        it->startoffset = startoffset;
//    }

//    fprintf (stderr, "tag size: %d\n", size);


    // try to read full tag if size is small enough
    if (size > 1000000) {
        return -1;
    }
    uint8_t tag[size];
    if (fread (tag, 1, size, fp) != size) {
        return -1; // bad size
    }
    uint8_t *readptr = tag;
    int crcpresent = 0;
//    fprintf (stderr, "version: 2.%d.%d, unsync: %d, extheader: %d, experimental: %d\n", version_major, version_minor, unsync, extheader, expindicator);
    
    if (extheader) {
        if (size < 6) {
            return -1; // bad size
        }
        uint32_t sz = (readptr[3] << 0) | (header[2] << 8) | (header[1] << 16) | (header[0] << 24);
        readptr += 4;
        if (size < sz) {
            return -1; // bad size
        }
        uint16_t extflags = (readptr[1] << 0) | (readptr[0] << 8);
        readptr += 2;
        uint32_t pad = (readptr[3] << 0) | (header[2] << 8) | (header[1] << 16) | (header[0] << 24);
        readptr += 4;
        if (extflags & 0x80000000) {
            crcpresent = 1;
        }
        if (crcpresent && sz != 10) {
            return -1; // bad header
        }
        readptr += 4; // skip crc
    }
    char * (*convstr)(const unsigned char *, int);
    if (version_major == 3) {
        convstr = convstr_id3v2_2to3;
    }
    else {
        convstr = convstr_id3v2_4;
    }
    char *artist = NULL;
    char *album = NULL;
    char *band = NULL;
    char *track = NULL;
    char *title = NULL;
    char *vendor = NULL;
    int err = 0;
    while (readptr - tag <= size - 4) {
        if (version_major == 3 || version_major == 4) {
            char frameid[5];
            memcpy (frameid, readptr, 4);
            frameid[4] = 0;
            readptr += 4;
            if (readptr - tag >= size - 4) {
                err = 1;
                break;
            }
            uint32_t sz = (readptr[3] << 0) | (readptr[2] << 8) | (readptr[1] << 16) | (readptr[0] << 24);
            readptr += 4;
            //fprintf (stderr, "got frame %s, size %d, pos %d, tagsize %d\n", frameid, sz, readptr-tag, size);
            if (readptr - tag >= size - sz) {
                err = 1;
                break; // size of frame is more than size of tag
            }
            if (sz < 1) {
//                err = 1;
                break; // frame must be at least 1 byte long
            }
            uint16_t flags = (readptr[1] << 0) | (readptr[0] << 8);
            readptr += 2;
//            fprintf (stderr, "found id3v2.3 frame: %s, size=%d\n", frameid, sz);
            if (!strcmp (frameid, "TPE1")) {
                if (sz > 1000) {
                    err = 1;
                    break; // too large
                }
                char str[sz+2];
                id3v2_string_read (version_major, &str[0], sz, unsync, &readptr);
                artist = convstr (str, sz);
            }
            else if (!strcmp (frameid, "TPE2")) {
                if (sz > 1000) {
                    err = 1;
                    break; // too large
                }
                char str[sz+2];
                id3v2_string_read (version_major, &str[0], sz, unsync, &readptr);
                band = convstr (str, sz);
            }
            else if (!strcmp (frameid, "TRCK")) {
                if (sz > 1000) {
                    err = 1;
                    break; // too large
                }
                char str[sz+2];
                id3v2_string_read (version_major, &str[0], sz, unsync, &readptr);
                track = convstr (str, sz);
            }
            else if (!strcmp (frameid, "TIT2")) {
                if (sz > 1000) {
                    err = 1;
                    break; // too large
                }
                char str[sz+2];
                id3v2_string_read (version_major, &str[0], sz, unsync, &readptr);
                title = convstr (str, sz);
            }
            else if (!strcmp (frameid, "TALB")) {
                if (sz > 1000) {
                    err = 1;
                    break; // too large
                }
                char str[sz+2];
                id3v2_string_read (version_major, &str[0], sz, unsync, &readptr);
                album = convstr (str, sz);
            }
            else {
                readptr += sz;
            }
        }
        else if (version_major == 2) {
            char frameid[4];
            memcpy (frameid, readptr, 3);
            frameid[3] = 0;
            readptr += 3;
            if (readptr - tag >= size - 3) {
                break;
            }
            uint32_t sz = (readptr[2] << 0) | (readptr[1] << 8) | (readptr[0] << 16);
            readptr += 3;
            if (readptr - tag >= size - sz) {
                break; // size of frame is less than size of tag
            }
            if (sz < 1) {
                break; // frame must be at least 1 byte long
            }
//            fprintf (stderr, "found id3v2.2 frame: %s, size=%d\n", frameid, sz);
            if (!strcmp (frameid, "TEN")) {
                char str[sz+2];
                memcpy (str, readptr, sz);
                str[sz] = 0;
                vendor = convstr (str, sz);
            }
            else if (!strcmp (frameid, "TT2")) {
                if (sz > 1000) {
                    continue;
                }
                char str[sz+2];
                memcpy (str, readptr, sz);
                str[sz] = 0;
                title = convstr (str, sz);
            }
            else if (!strcmp (frameid, "TAL")) {
                if (sz > 1000) {
                    continue;
                }
                char str[sz+2];
                memcpy (str, readptr, sz);
                str[sz] = 0;
                album = convstr (str, sz);
            }
            else if (!strcmp (frameid, "TP1")) {
                if (sz > 1000) {
                    continue;
                }
                char str[sz+2];
                memcpy (str, readptr, sz);
                str[sz] = 0;
                artist = convstr (str, sz);
            }
            else if (!strcmp (frameid, "TP2")) {
                if (sz > 1000) {
                    continue;
                }
                char str[sz+2];
                memcpy (str, readptr, sz);
                str[sz] = 0;
                band = convstr (str, sz);
            }
            else if (!strcmp (frameid, "TRK")) {
                if (sz > 1000) {
                    continue;
                }
                char str[sz+2];
                memcpy (str, readptr, sz);
                str[sz] = 0;
                track = convstr (str, sz);
            }
            readptr += sz;
        }
        else {
//            fprintf (stderr, "id3v2.%d (unsupported!)\n", version_minor);
        }
    }
    if (!err) {
        if (artist) {
            pl_add_meta (it, "artist", artist);
            free (artist);
        }
        if (album) {
            pl_add_meta (it, "album", album);
            free (album);
        }
        if (band) {
            pl_add_meta (it, "band", band);
            free (band);
        }
        if (track) {
            pl_add_meta (it, "track", track);
            free (track);
        }
        if (title) {
            pl_add_meta (it, "title", title);
            free (title);
        }
        if (vendor) {
            pl_add_meta (it, "vendor", vendor);
            free (vendor);
        }
        if (!title) {
            pl_add_meta (it, "title", NULL);
        }
        return 0;
    }
    return -1;
}

const char *
junk_detect_charset (const char *s) {
    // check if that's already utf8
    if (u8_valid (s, strlen (s), NULL)) {
        return NULL; // means no recoding required
    }
    // check if that could be non-latin1 (too many nonascii chars)
    if (can_be_russian (s)) {
        return "cp1251";
    }
    return "iso8859-1";
}

void
junk_recode (const char *in, int inlen, char *out, int outlen, const char *cs) {
    iconv_t cd = iconv_open ("utf8", cs);
    if (!cd) {
        return;
    }
    else {
        size_t inbytesleft = inlen;
        size_t outbytesleft = outlen;
        char *pin = (char*)in;
        char *pout = out;
        memset (out, 0, outlen);
        size_t res = iconv (cd, &pin, &inbytesleft, &pout, &outbytesleft);
        iconv_close (cd);
    }
}
