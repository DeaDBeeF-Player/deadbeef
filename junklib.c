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
#include "junklib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define LIBICONV_PLUG
#include <iconv.h>
#include <limits.h>
#include <errno.h>
#include <ctype.h>
#include "playlist.h"
#include "utf8.h"
#include "plugins.h"
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define MAX_TEXT_FRAME_SIZE 1024
#define MAX_APEV2_FRAME_SIZE 100000
#define MAX_ID3V2_FRAME_SIZE 100000

#define UTF8 "utf-8"

#define trace(...) { fprintf(stderr, __VA_ARGS__); }
//#define trace(fmt,...)

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

static uint32_t
extract_i32 (const uint8_t *buf)
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
extract_i16 (const uint8_t *buf)
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

int
junk_iconv (const char *in, int inlen, char *out, int outlen, const char *cs_in, const char *cs_out) {
    iconv_t cd = iconv_open (cs_out, cs_in);
    if (cd == (iconv_t)-1) {
        return -1;
    }
#ifdef __linux__
    char *pin = (char*)in;
#else
    const char *pin = value;
#endif

    size_t inbytesleft = inlen;
    size_t outbytesleft = outlen;

    char *pout = out;
    memset (out, 0, outbytesleft);

    size_t res = iconv (cd, &pin, &inbytesleft, &pout, &outbytesleft);
    int err = errno;
    iconv_close (cd);

    trace ("iconv -f %s -t %s '%s': returned %d, inbytes %d/%d, outbytes %d/%d, errno=%d\n", cs_in, cs_out, in, res, inlen, inbytesleft, outlen, outbytesleft, err);
    if (res == -1) {
        return -1;
    }
    return pout - out;
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
    NULL
};

static int
can_be_russian (const signed char *str) {
    int latin = 0;
    int rus = 0;
    int rus_in_row = 0;
    int max_rus_row = 0;
    for (; *str; str++) {
        if ((*str >= 'A' && *str <= 'Z')
                || *str >= 'a' && *str <= 'z') {
            if (rus_in_row > max_rus_row) {
                max_rus_row = rus_in_row;
            }
            rus_in_row = 0;
            latin++;
        }
        else if (*str < 0) {
            rus_in_row++;
            rus++;
        }
    }
    if (rus > latin/2 || (max_rus_row > 4)) {
        return 1;
    }
    return 0;
}

#if 0
static char *
convstr_id3v2_2to3 (const unsigned char* str, int sz) {
    static char out[2048];
    const char *enc = "iso8859-1";
    char *ret = out;

    // hack to add limited cp1251 recoding support
    if (*str == 1) {
        if (str[1] == 0xff && str[2] == 0xfe) {
            enc = "UCS-2LE";
            str += 2;
            sz -= 2;
        }
        else if (str[2] == 0xff && str[1] == 0xfe) {
            enc = "UCS-2BE";
            str += 2;
            sz -= 2;
        }
        else {
            trace ("invalid ucs-2 signature %x %x\n", (int)str[1], (int)str[2]);
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
    iconv_t cd = iconv_open (UTF8, enc);
    if (cd == (iconv_t)-1) {
        trace ("iconv can't recoode from %s to utf8", enc);
        return strdup ("-");
    }
    else {
        size_t inbytesleft = sz;
        size_t outbytesleft = 2047;
#ifdef __linux__
        char *pin = (char*)str;
#else
        const char *pin = str;
#endif
        char *pout = out;
        memset (out, 0, sizeof (out));
        /*size_t res = */iconv (cd, &pin, &inbytesleft, &pout, &outbytesleft);
        iconv_close (cd);
        ret = out;
    }
    return strdup (ret);
}
#endif

static char *
convstr_id3v2 (int version, uint8_t encoding, const unsigned char* str, int sz) {
    static char out[2048];
    const char *enc = "iso8859-1";
    char *ret = out;

    // hack to add limited cp1251 recoding support

    if (version == 4 && encoding == 3) {
        // utf8
        trace ("utf8\n");
        strncpy (out, str, 2047);
        out[min (sz, 2047)] = 0;
        return strdup (out);
    }
    else if (version == 4 && encoding == 2) {
        trace ("utf16be\n");
        enc = "UTF-16BE";
    }
    else if (encoding == 1) {
        if (version < 4) {
            if (str[0] == 0xff && str[1] == 0xfe) {
                enc = "UCS-2LE";
                str += 2;
                sz -= 2;
            }
            else if (str[1] == 0xff && str[0] == 0xfe) {
                enc = "UCS-2BE";
                str += 2;
                sz -= 2;
            }
            else {
                trace ("invalid ucs-2 signature %x %x\n", (int)str[0], (int)str[1]);
                return NULL;
            }
        }
        else {
            trace ("utf16\n");
            enc = "UTF-16";
        }
    }
#if 0
    // NOTE: some dumb taggers put non-iso8859-1 text with enc=0
    else if (*str == 0) {
        // iso8859-1
        trace ("iso8859-1\n");
        enc = "iso8859-1";
    }
#endif
    else {
        if (can_be_russian (str)) {
            enc = "cp1251";
        }
    }
    iconv_t cd = iconv_open (UTF8, enc);
    if (cd == (iconv_t)-1) {
        trace ("iconv can't recode from %s to utf8\n", enc);
        return strdup ("-");
    }
    else {
        size_t inbytesleft = sz;
        size_t outbytesleft = 2047;
#ifdef __linux__
        char *pin = (char*)str;
#else
        const char *pin = str;
#endif
        char *pout = out;
        memset (out, 0, sizeof (out));
        /*size_t res = */iconv (cd, &pin, &inbytesleft, &pout, &outbytesleft);
        iconv_close (cd);
        ret = out;
    }
//    trace ("decoded %s\n", out+3);
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
    cd = iconv_open (UTF8, UTF8);
    if (cd == (iconv_t)-1) {
        trace ("iconv doesn't support utf8\n");
        return str;
    }
    size_t inbytesleft = sz;
    size_t outbytesleft = 2047;
#ifdef __linux__
        char *pin = (char*)str;
#else
        const char *pin = str;
#endif
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
    cd = iconv_open (UTF8, enc);
    if (cd == (iconv_t)-1) {
        trace ("iconv can't recode from %s to utf8\n", enc);
        return str;
    }
    else {
        size_t inbytesleft = sz;
        size_t outbytesleft = 2047;
#ifdef __linux__
        char *pin = (char*)str;
#else
        const char *pin = str;
#endif
        char *pout = out;
        memset (out, 0, sizeof (out));
        /*size_t res = */iconv (cd, &pin, &inbytesleft, &pout, &outbytesleft);
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
junk_id3v1_read (playItem_t *it, DB_FILE *fp) {
    if (!it || !fp) {
        trace ("bad call to junk_id3v1_read!\n");
        return -1;
    }
    uint8_t buffer[128];
    // try reading from end
    if (deadbeef->fseek (fp, -128, SEEK_END) == -1) {
        return -1;
    }
    if (deadbeef->fread (buffer, 1, 128, fp) != 128) {
        return -1;
    }
    if (memcmp (buffer, "TAG", 3)) {
        return -1; // no tag
    }
    char title[31];
    char artist[31];
    char album[31];
    char year[5];
    char comment[31];
    uint8_t genreid;
    uint8_t tracknum;
    const char *genre = NULL;
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
    else {
        genre = "";
    }

    // add meta
//    trace ("%s - %s - %s - %s - %s - %s\n", title, artist, album, year, comment, genre);
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

    pl_append_meta (it, "tags", "ID3v1");

    return 0;
}

int
junk_id3v1_write (FILE *fp, playItem_t *it) {
    char title[30] = "";
    char artist[30] = "";
    char album[30] = "";
    char year[4] = "";
    char comment[28] = "";
    uint8_t genreid = 0xff;
    uint8_t tracknum = 0;

    const char *meta;

#define conv(name, store) {\
    memset (store, 0x20, sizeof (store));\
    meta = pl_find_meta (it, name);\
    if (meta) {\
        char temp[1000];\
        int l = junk_iconv (meta, strlen (meta), temp, sizeof (temp), UTF8, "ASCII");\
        if (l == -1) {\
            memset (store, 0, sizeof (store));\
        }\
        else {\
            strncpy (store, temp, sizeof (store));\
        }\
    }\
}

    conv ("title", title);
    conv ("artist", artist);
    conv ("album", album);
    conv ("year", year);
    conv ("comment", comment);

#undef conv

    // tracknum
    meta = pl_find_meta (it, "track");
    if (meta) {
        tracknum = atoi (meta);
    }

    // find genre
    meta = pl_find_meta (it, "genre");
    if (meta) {
        for (int i = 0; junk_genretbl[i]; i++) {
            if (!strcasecmp (meta, junk_genretbl[i])) {
                genreid = i;
                break;
            }
        }
    }

    if (fwrite ("TAG", 1, 3, fp) != 3) {
        trace ("junk_id3v1_write: failed to write signature\n");
        return -1;
    }
    if (fwrite (title, 1, sizeof (title), fp) != sizeof (title)) {
        trace ("junk_id3v1_write: failed to write title\n");
        return -1;
    }
    if (fwrite (artist, 1, sizeof (artist), fp) != sizeof (artist)) {
        trace ("junk_id3v1_write: failed to write artist\n");
        return -1;
    }
    if (fwrite (album, 1, sizeof (album), fp) != sizeof (album)) {
        trace ("junk_id3v1_write: failed to write album\n");
        return -1;
    }
    if (fwrite (year, 1, sizeof (year), fp) != sizeof (year)) {
        trace ("junk_id3v1_write: failed to write year\n");
        return -1;
    }
    if (fwrite (comment, 1, sizeof (comment), fp) != sizeof (comment)) {
        trace ("junk_id3v1_write: failed to write comment\n");
        return -1;
    }
    uint8_t zero = 0;
    if (fwrite (&zero, 1, 1, fp) != 1) {
        trace ("junk_id3v1_write: failed to write id3v1.1 marker\n");
        return -1;
    }
    if (fwrite (&tracknum, 1, 1, fp) != 1) {
        trace ("junk_id3v1_write: failed to write track\n");
        return -1;
    }
    if (fwrite (&genreid, 1, 1, fp) != 1) {
        trace ("junk_id3v1_write: failed to write genre\n");
        return -1;
    }
    return 0;
}

int
junk_id3v1_find (DB_FILE *fp) {
    uint8_t buffer[3];
    if (deadbeef->fseek (fp, -128, SEEK_END) == -1) {
        return -1;
    }
    if (deadbeef->fread (buffer, 1, 3, fp) != 3) {
        return -1;
    }
    if (memcmp (buffer, "TAG", 3)) {
        return -1; // no tag
    }
    return deadbeef->ftell (fp) - 3;
}

int
junk_apev2_find (DB_FILE *fp, int32_t *psize, uint32_t *pflags, uint32_t *pnumitems) {
    uint8_t header[32];
    if (deadbeef->fseek (fp, -32, SEEK_END) == -1) {
        return -1; // something bad happened
    }

    if (deadbeef->fread (header, 1, 32, fp) != 32) {
        return -1; // something bad happened
    }
    if (strncmp (header, "APETAGEX", 8)) {
        // try to skip 128 bytes backwards (id3v1)
        if (deadbeef->fseek (fp, -128-32, SEEK_END) == -1) {
            return -1; // something bad happened
        }
        if (deadbeef->fread (header, 1, 32, fp) != 32) {
            return -1; // something bad happened
        }
        if (strncmp (header, "APETAGEX", 8)) {
            return -1; // no ape tag here
        }
    }

    uint32_t version = extract_i32_le (&header[8]);
    int32_t size = extract_i32_le (&header[12]);
    uint32_t numitems = extract_i32_le (&header[16]);
    uint32_t flags = extract_i32_le (&header[20]);

    trace ("APEv%d, size=%d, items=%d, flags=%x\n", version, size, numitems, flags);

    // size contains footer, but not header, so add header size
    if (flags & (1<<31)) {
        size += 32;
    }

    // seek to beginning of the tag/header
    if (deadbeef->fseek (fp, -size, SEEK_CUR) == -1) {
        trace ("failed to seek to tag start (-%d)\n", size);
        return -1;
    }
    *psize = size;
    *pflags = flags;
    *pnumitems = numitems;
    return deadbeef->ftell (fp);
}

int
junk_find_id3v1 (DB_FILE *fp) {
    if (deadbeef->fseek (fp, -128, SEEK_END) == -1) {
        return -1;
    }
    char buffer[3];
    if (deadbeef->fread (buffer, 1, 3, fp) != 3) {
        return -1;
    }
    if (memcmp (buffer, "TAG", 3)) {
        return -1; // no tag
    }
    return deadbeef->ftell (fp) - 3;
}

int
junk_apev2_read_full (playItem_t *it, DB_apev2_tag_t *tag_store, DB_FILE *fp) {
//    trace ("trying to read ape tag\n");
    // try to read footer, position must be already at the EOF right before
    // id3v1 (if present)

    DB_apev2_frame_t *tail = NULL;

    uint8_t header[32];
    if (deadbeef->fseek (fp, -32, SEEK_END) == -1) {
        return -1; // something bad happened
    }

    if (deadbeef->fread (header, 1, 32, fp) != 32) {
        return -1; // something bad happened
    }
    if (strncmp (header, "APETAGEX", 8)) {
        // try to skip 128 bytes backwards (id3v1)
        if (deadbeef->fseek (fp, -128-32, SEEK_END) == -1) {
            return -1; // something bad happened
        }
        if (deadbeef->fread (header, 1, 32, fp) != 32) {
            return -1; // something bad happened
        }
        if (strncmp (header, "APETAGEX", 8)) {
            return -1; // no ape tag here
        }
    }

    // end of footer must be 0
//    if (memcmp (&header[24], "\0\0\0\0\0\0\0\0", 8)) {
//        trace ("bad footer\n");
//        return -1;
//    }

    uint32_t version = extract_i32_le (&header[8]);
    int32_t size = extract_i32_le (&header[12]);
    uint32_t numitems = extract_i32_le (&header[16]);
    uint32_t flags = extract_i32_le (&header[20]);

    trace ("APEv%d, size=%d, items=%d, flags=%x\n", version, size, numitems, flags);
    pl_append_meta (it, "tags", "APEv2");

    // now seek to beginning of the tag (exluding header)
    if (deadbeef->fseek (fp, -size, SEEK_CUR) == -1) {
        trace ("failed to seek to tag start (-%d)\n", size);
        return -1;
    }
    int i;
    for (i = 0; i < numitems; i++) {
        uint8_t buffer[8];
        if (deadbeef->fread (buffer, 1, 8, fp) != 8) {
            return -1;
        }
        uint32_t itemsize = extract_i32_le (&buffer[0]);
        uint32_t itemflags = extract_i32_le (&buffer[4]);

        // read key until 0 (stupid and slow)
        char key[256];
        int keysize = 0;
        while (keysize <= 255) {
            if (deadbeef->fread (&key[keysize], 1, 1, fp) != 1) {
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
        trace ("item %d, size %d, flags %08x, keysize %d, key %s\n", i, itemsize, itemflags, keysize, key);
        // read value
        if (itemsize <= MAX_APEV2_FRAME_SIZE) // just a sanity check
        {
            uint8_t *value = malloc (itemsize+1);
            if (!value) {
                trace ("junk_read_ape_full: failed to allocate %d bytes\n", itemsize+1);
                return -1;
            }
            if (deadbeef->fread (value, 1, itemsize, fp) != itemsize) {
                trace ("junk_read_ape_full: failed to read %d bytes from file\n", itemsize);
                free (value);
                return -1;
            }
            value[itemsize] = 0;

            if (tag_store) {
                DB_apev2_frame_t *frm = malloc (sizeof (DB_apev2_frame_t) + itemsize);
                memset (frm, 0, sizeof (DB_apev2_tag_t));
                frm->flags = flags;
                strcpy (frm->key, key);
                frm->size = size;
                memcpy (frm->data, value, itemsize);
                if (tail) {
                    tail->next = frm;
                }
                else {
                    tag_store->frames = frm;
                }
                tail = frm;
            }

            int valuetype = ((itemflags >> 1) & 3);
            // add metainfo only if it's textual
            if (valuetype == 0 && itemsize < MAX_TEXT_FRAME_SIZE) {
                if (!u8_valid (value, itemsize, NULL)) {
                    trace ("junk_read_ape_full: bad encoding in text frame %s\n", key);
                    continue;
                }

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
                    char *slash = strchr (value, '/');
                    if (slash) {
                        // split into track/number
                        *slash = 0;
                        slash++;
                        pl_add_meta (it, "numtracks", slash);
                    }
                    pl_add_meta (it, "track", value);
                }
                else if (!strcasecmp (key, "year")) {
                    pl_add_meta (it, "year", value);
                }
                else if (!strcasecmp (key, "genre")) {
                    pl_add_meta (it, "genre", value);
                }
                else if (!strcasecmp (key, "composer")) {
                    pl_add_meta (it, "composer", value);
                }
                else if (!strcasecmp (key, "comment")) {
                    pl_add_meta (it, "comment", value);
                }
                else if (!strcasecmp (key, "copyright")) {
                    pl_add_meta (it, "copyright", value);
                }
                else if (!strcasecmp (key, "cuesheet")) {
                    pl_add_meta (it, "cuesheet", value);
                }
                else if (!strncasecmp (key, "replaygain_album_gain", 21)) {
                    it->replaygain_album_gain = atof (value);
                    trace ("album_gain=%s\n", value);
                }
                else if (!strncasecmp (key, "replaygain_album_peak", 21)) {
                    it->replaygain_album_peak = atof (value);
                    trace ("album_peak=%s\n", value);
                }
                else if (!strncasecmp (key, "replaygain_track_gain", 21)) {
                    it->replaygain_track_gain = atof (value);
                    trace ("track_gain=%s\n", value);
                }
                else if (!strncasecmp (key, "replaygain_track_peak", 21)) {
                    it->replaygain_track_peak = atof (value);
                    trace ("track_peak=%s\n", value);
                }
            }
            free (value);
        }
        else {
            // try to skip
            if (0 != deadbeef->fseek (fp, SEEK_CUR, itemsize)) {
                fprintf (stderr, "junklib: corrupted APEv2 tag\n");
                return -1;
            }
        }
    }

    return 0;
}

int
junk_apev2_read (playItem_t *it, DB_FILE *fp) {
    return junk_apev2_read_full (it, NULL, fp);
}

int
junk_id3v2_find (DB_FILE *fp, int *psize) {
    if (deadbeef->fseek (fp, 0, SEEK_SET) == -1) {
        trace ("junk_id3v2_find: seek error\n");
        return -1;
    }
    uint8_t header[10];
    int pos = deadbeef->ftell (fp);
    if (pos == -1) {
        trace ("junk_id3v2_find: ftell error\n");
        return -1;
    }
    if (deadbeef->fread (header, 1, 10, fp) != 10) {
        trace ("junk_id3v2_find: read error\n");
        return -1; // too short
    }
    if (strncmp (header, "ID3", 3)) {
        return -1; // no tag
    }
    uint8_t flags = header[5];
    if (flags & 15) {
        return -1; // unsupported
    }
    int footerpresent = (flags & (1<<4)) ? 1 : 0;
    // check for bad size
    if ((header[9] & 0x80) || (header[8] & 0x80) || (header[7] & 0x80) || (header[6] & 0x80)) {
        return -1; // bad header
    }
    uint32_t size = (header[9] << 0) | (header[8] << 7) | (header[7] << 14) | (header[6] << 21);
    //trace ("junklib: leading junk size %d\n", size);
    *psize = size + 10 + 10 * footerpresent;
    return pos;
}

int
junk_get_leading_size_stdio (FILE *fp) {
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
    uint8_t flags = header[5];
    if (flags & 15) {
        return -1; // unsupported
    }
    int footerpresent = (flags & (1<<4)) ? 1 : 0;
    // check for bad size
    if ((header[9] & 0x80) || (header[8] & 0x80) || (header[7] & 0x80) || (header[6] & 0x80)) {
        return -1; // bad header
    }
    uint32_t size = (header[9] << 0) | (header[8] << 7) | (header[7] << 14) | (header[6] << 21);
    //trace ("junklib: leading junk size %d\n", size);
    return size + 10 + 10 * footerpresent;
}

int
junk_get_leading_size (DB_FILE *fp) {
    uint8_t header[10];
    int pos = deadbeef->ftell (fp);
    if (deadbeef->fread (header, 1, 10, fp) != 10) {
        deadbeef->fseek (fp, pos, SEEK_SET);
        trace ("junk_get_leading_size: file is too short\n");
        return -1; // too short
    }
    deadbeef->fseek (fp, pos, SEEK_SET);
    if (strncmp (header, "ID3", 3)) {
        trace ("junk_get_leading_size: no id3v2 found\n");
        return -1; // no tag
    }
    uint8_t flags = header[5];
    if (flags & 15) {
        trace ("unsupported flags in id3v2\n");
        return -1; // unsupported
    }
    int footerpresent = (flags & (1<<4)) ? 1 : 0;
    // check for bad size
    if ((header[9] & 0x80) || (header[8] & 0x80) || (header[7] & 0x80) || (header[6] & 0x80)) {
        trace ("bad header in id3v2\n");
        return -1; // bad header
    }
    uint32_t size = (header[9] << 0) | (header[8] << 7) | (header[7] << 14) | (header[6] << 21);
    //trace ("junklib: leading junk size %d\n", size);
    return size + 10 + 10 * footerpresent;
}

int
junk_id3v2_unsync (uint8_t *out, int len, int maxlen) {
    uint8_t buf [maxlen];
    uint8_t *p = buf;
    int res = -1;
    for (int i = 0; i < len; i++) {
        *p++ = out[i];
        if (i < len - 1 && out[i] == 0xff && (out[i+1] & 0xe0) == 0xe0) {
            *p++ = 0;
            res = 0;
        }
    }
    if (!res) {
        res = p-buf;
        memcpy (out, buf, res);
    }
    return res;
}

int
junk_id3v2_remove_frames (DB_id3v2_tag_t *tag, const char *frame_id) {
    DB_id3v2_frame_t *prev = NULL;
    for (DB_id3v2_frame_t *f = tag->frames; f; ) {
        DB_id3v2_frame_t *next = f->next;
        if (!strcmp (f->id, frame_id)) {
            if (prev) {
                prev->next = f->next;
            }
            else {
                tag->frames = f->next;
            }
            free (f);
        }
        else {
            prev = f;
        }
        f = next;
    }
    return 0;
}

DB_id3v2_frame_t *
junk_id3v2_add_text_frame_23 (DB_id3v2_tag_t *tag, const char *frame_id, const char *value) {
    // convert utf8 into ucs2_le
    size_t inlen = strlen (value);
    if (!inlen) {
        return NULL;
    }
    size_t outlen = inlen * 3;
    uint8_t out[outlen];

    trace ("junklib: setting 2.3 text frame '%s' = '%s'\n", frame_id, value);

    int encoding = 0;

    int res;
    res = junk_iconv (value, inlen, out, outlen, UTF8, "ISO-8859-1");
    if (res == -1) {
        res = junk_iconv (value, inlen, out+2, outlen-2, UTF8, "UCS-2LE");
        if (res == -1) {
            return NULL;
        }
        out[0] = 0xff;
        out[1] = 0xfe;
        outlen = res + 2;
        trace ("successfully converted to ucs-2le (size=%d, bom: %x %x)\n", res, out[0], out[1]);
        encoding = 1;
    }
    else {
        trace ("successfully converted to iso8859-1 (size=%d)\n", res);
        outlen = res;
    }

    // make a frame
    int size = outlen + 1 + encoding + 1;
    trace ("calculated frame size = %d\n", size);
    DB_id3v2_frame_t *f = malloc (size + sizeof (DB_id3v2_frame_t));
    memset (f, 0, sizeof (DB_id3v2_frame_t));
    strcpy (f->id, frame_id);
    f->size = size;
    f->data[0] = encoding;
    memcpy (&f->data[1], out, outlen);
    f->data[outlen+1] = 0;
    if (encoding == 1) {
        f->data[outlen+2] = 0;
    }
    // append to tag
    DB_id3v2_frame_t *tail;
    for (tail = tag->frames; tail && tail->next; tail = tail->next);
    if (tail) {
        tail->next = f;
    }
    else {
        tag->frames = f;
    }

    return f;
}

DB_id3v2_frame_t *
junk_id3v2_add_text_frame_24 (DB_id3v2_tag_t *tag, const char *frame_id, const char *value) {
    trace ("junklib: setting 2.4 text frame '%s' = '%s'\n", frame_id, value);

    // make a frame
    int outlen = strlen (value);
    if (!outlen) {
        return NULL;
    }

    int size = outlen + 1 + 1;
    trace ("calculated frame size = %d\n", size);
    DB_id3v2_frame_t *f = malloc (size + sizeof (DB_id3v2_frame_t));
    memset (f, 0, sizeof (DB_id3v2_frame_t));
    strcpy (f->id, frame_id);
    // flags are all zero
    f->size = size;
    f->data[0] = 3; // encoding=utf8
    memcpy (&f->data[1], value, outlen);
    f->data[outlen+1] = 0;
    // append to tag
    DB_id3v2_frame_t *tail;
    for (tail = tag->frames; tail && tail->next; tail = tail->next);
    if (tail) {
        tail->next = f;
    }
    else {
        tag->frames = f;
    }

    return f;
}

DB_id3v2_frame_t *
junk_id3v2_add_comment_frame_24 (DB_id3v2_tag_t *tag, const char *lang, const char *descr, const char *value) {
    trace ("junklib: setting 2.4 COMM frame lang=%s, descr='%s', data='%s'\n", lang, value);

    // make a frame
    int descrlen = strlen (descr);
    int outlen = strlen (value);
    int size = 1 + 3 + descrlen + 1 + outlen + 1;
    trace ("calculated frame size = %d\n", size);
    DB_id3v2_frame_t *f = malloc (size + sizeof (DB_id3v2_frame_t));
    memset (f, 0, sizeof (DB_id3v2_frame_t));
    strcpy (f->id, "COMM");
    // flags are all zero
    f->size = size;
    f->data[0] = 3; // encoding=utf8
    memcpy (&f->data[1], lang, 3);
    memcpy (&f->data[4], descr, descrlen+1);
    memcpy (&f->data[4+descrlen+1], value, outlen);
    // append to tag
    DB_id3v2_frame_t *tail;
    for (tail = tag->frames; tail && tail->next; tail = tail->next);
    if (tail) {
        tail->next = f;
    }
    else {
        tag->frames = f;
    }

    return f;
}

// [+] TODO: remove all unsync bytes during conversion, where appropriate
// TODO: some non-Txxx frames might still need charset conversion
// TODO: 2.4 TDTG frame (tagging time) should not be converted, but might be useful to create it
int
junk_id3v2_convert_24_to_23 (DB_id3v2_tag_t *tag24, DB_id3v2_tag_t *tag23) {
    DB_id3v2_frame_t *f24;
    DB_id3v2_frame_t *tail = tag23->frames;

    while (tail && tail->next) {
        tail = tail->next;
    }

    const char *copy_frames[] = {
        "AENC", "APIC",
        "COMM", "COMR", "ENCR",
        "ETCO", "GEOB", "GRID",
        "LINK", "MCDI", "MLLT", "OWNE", "PRIV",
        "POPM", "POSS", "RBUF",
        "RVRB",
        "SYLT", "SYTC",
        "UFID", "USER", "USLT",
        NULL
    };

    // NOTE: 2.4 ASPI, EQU2, RVA2, SEEK, SIGN are discarded for 2.3
    // NOTE: 2.4 PCNT is discarded because it is useless
    // NOTE: all Wxxx frames are copy_frames, handled as special case


    // "TDRC" TDAT with conversion from ID3v2-strct timestamp to DDMM format
    // "TDOR" TORY with conversion from ID3v2-strct timestamp to year
    // TODO: "TIPL" IPLS with conversion to non-text format

    const char *text_frames[] = {
        "TALB", "TBPM", "TCOM", "TCON", "TCOP", "TDLY", "TENC", "TEXT", "TFLT", "TIT1", "TIT2", "TIT3", "TKEY", "TLAN", "TLEN", "TMED", "TOAL", "TOFN", "TOLY", "TOPE", "TOWN", "TPE1", "TPE2", "TPE3", "TPE4", "TPOS", "TPUB", "TRCK", "TRSN", "TRSO", "TSRC", "TSSE", "TXXX", "TDRC", NULL
    };

    // NOTE: 2.4 TMCL (musician credits list) is discarded for 2.3
    // NOTE: 2.4 TMOO (mood) is discarded for 2.3
    // NOTE: 2.4 TPRO (produced notice) is discarded for 2.3
    // NOTE: 2.4 TSOA (album sort order) is discarded for 2.3
    // NOTE: 2.4 TSOP (performer sort order) is discarded for 2.3
    // NOTE: 2.4 TSOT (title sort order) is discarded for 2.3
    // NOTE: 2.4 TSST (set subtitle) is discarded for 2.3

    for (f24 = tag24->frames; f24; f24 = f24->next) {
        DB_id3v2_frame_t *f23 = NULL;
        // we are altering the tag, so check for tag alter preservation
        if (tag24->flags & (1<<6)) {
            continue; // discard the frame
        }

        int simplecopy = 0; // means format is the same in 2.3 and 2.4
        int text = 0; // means this is a text frame

        int i;

        if (f24->id[0] == 'W') { // covers all W000..WZZZ tags
            simplecopy = 1;
        }

        if (!simplecopy) {
            for (i = 0; copy_frames[i]; i++) {
                if (!strcmp (f24->id, copy_frames[i])) {
                    simplecopy = 1;
                    break;
                }
            }
        }

        if (!simplecopy) {
            // check if this is a text frame
            for (i = 0; text_frames[i]; i++) {
                if (!strcmp (f24->id, text_frames[i])) {
                    text = 1;
                    break;
                }
            }
        }


        if (!simplecopy && !text) {
            continue; // unknown frame
        }

        // convert flags
        uint8_t flags[2];
        // 1st byte (status flags) is the same, but shifted by 1 bit to the left
        flags[0] = f24->flags[0] << 1;
        
        // 2nd byte (format flags) is quite different
        // 2.4 format is %0h00kmnp (grouping, compression, encryption, unsync)
        // 2.3 format is %ijk00000 (compression, encryption, grouping)
        flags[1] = 0;
        if (f24->flags[1] & (1 << 6)) {
            flags[1] |= (1 << 4);
        }
        if (f24->flags[1] & (1 << 3)) {
            flags[1] |= (1 << 7);
        }
        if (f24->flags[1] & (1 << 2)) {
            flags[1] |= (1 << 6);
        }
        if (f24->flags[1] & (1 << 1)) {
            flags[1] |= (1 << 5);
        }
        if (f24->flags[1] & 1) {
            // 2.3 doesn't support per-frame unsyncronyzation
            // let's ignore it
        }

        if (simplecopy) {
            f23 = malloc (sizeof (DB_id3v2_frame_t) + f24->size);
            memset (f23, 0, sizeof (DB_id3v2_frame_t) + f24->size);
            strcpy (f23->id, f24->id);
            f23->size = f24->size;
            memcpy (f23->data, f24->data, f24->size);
            f23->flags[0] = flags[0];
            f23->flags[1] = flags[1];
        }
        else if (text) {
            char *decoded = convstr_id3v2 (4, f24->data[0], f24->data+1, f24->size-1);
            if (!decoded) {
                trace ("junk_id3v2_convert_24_to_23: failed to decode text frame %s\n", f24->id);
                continue; // failed, discard it
            }
            if (!strcmp (f24->id, "TDRC")) {
                trace ("junk_id3v2_convert_24_to_23: TDRC text: %s\n", decoded);
                int year, month, day;
                int c = sscanf (decoded, "%4d-%2d-%2d", &year, &month, &day);
                if (c >= 1) {
                    char s[5];
                    snprintf (s, sizeof (s), "%04d", year);
                    f23 = junk_id3v2_add_text_frame_23 (tag23, "TYER", s);
                    if (f23) {
                        tail = f23;
                        f23 = NULL;
                    }
                }
                if (c == 3) {
                    char s[5];
                    snprintf (s, sizeof (s), "%02d%02d", month, day);
                    f23 = junk_id3v2_add_text_frame_23 (tag23, "TDAT", s);
                    if (f23) {
                        tail = f23;
                        f23 = NULL;
                    }
                }
                else {
                    trace ("junk_id3v2_add_text_frame_23: 2.4 TDRC doesn't have month/day info; discarded\n");
                }
            }
            else if (!strcmp (f24->id, "TDOR")) {
                trace ("junk_id3v2_convert_24_to_23: TDOR text: %s\n", decoded);
                int year;
                int c = sscanf (decoded, "%4d", &year);
                if (c == 1) {
                    char s[5];
                    snprintf (s, sizeof (s), "%04d", &year);
                    f23 = junk_id3v2_add_text_frame_23 (tag23, "TORY", s);
                    if (f23) {
                        tail = f23;
                        f23 = NULL;
                    }
                }
                else {
                    trace ("junk_id3v2_add_text_frame_23: 2.4 TDOR doesn't have month/day info; discarded\n");
                }
            }
            else {
                // encode for 2.3
                f23 = junk_id3v2_add_text_frame_23 (tag23, f24->id, decoded);
                if (f23) {
                    tail = f23;
                    f23 = NULL;
                }
            }
            free (decoded);
        }
        if (f23) {
            if (tail) {
                tail->next = f23;
            }
            else {
                tag23->frames = f23;
            }
            tail = f23;
        }
    }

    // convert tag header
    tag23->version[0] = 3;
    tag23->version[1] = 0;
    tag23->flags = tag24->flags;
    tag23->flags &= ~(1<<4); // no footer (unsupported in 2.3)
    tag23->flags &= ~(1<<7); // no unsync

    return 0;
}

int
junk_id3v2_convert_23_to_24 (DB_id3v2_tag_t *tag23, DB_id3v2_tag_t *tag24) {
    DB_id3v2_frame_t *f23;
    DB_id3v2_frame_t *tail = tag24->frames;

    while (tail && tail->next) {
        tail = tail->next;
    }

    const char *copy_frames[] = {
        "AENC", "APIC",
        "COMM", "COMR", "ENCR",
        "ETCO", "GEOB", "GRID",
        "LINK", "MCDI", "MLLT", "OWNE", "PRIV",
        "POPM", "POSS", "RBUF",
        "RVRB",
        "SYLT", "SYTC",
        "UFID", "USER", "USLT",
        NULL
    };

    // NOTE: all Wxxx frames are copy_frames, handled as special case

    // "TDRC" TDAT with conversion from ID3v2-strct timestamp to DDMM format
    // "TDOR" TORY with conversion from ID3v2-strct timestamp to year
    // TODO: "TIPL" IPLS with conversion to non-text format

    const char *text_frames[] = {
        "TALB", "TBPM", "TCOM", "TCON", "TCOP", "TDLY", "TENC", "TEXT", "TFLT", "TIT1", "TIT2", "TIT3", "TKEY", "TLAN", "TLEN", "TMED", "TOAL", "TOFN", "TOLY", "TOPE", "TOWN", "TPE1", "TPE2", "TPE3", "TPE4", "TPOS", "TPUB", "TRCK", "TRSN", "TRSO", "TSRC", "TSSE", "TXXX", "TDRC", NULL
    };

    for (f23 = tag23->frames; f23; f23 = f23->next) {
        // we are altering the tag, so check for tag alter preservation
        if (tag23->flags & (1<<7)) {
            continue; // discard the frame
        }

        int simplecopy = 0; // means format is the same in 2.3 and 2.4
        int text = 0; // means this is a text frame

        int i;

        if (f23->id[0] == 'W') { // covers all W000..WZZZ tags
            simplecopy = 1;
        }

        if (!simplecopy) {
            for (i = 0; copy_frames[i]; i++) {
                if (!strcmp (f23->id, copy_frames[i])) {
                    simplecopy = 1;
                    break;
                }
            }
        }

        if (!simplecopy) {
            // check if this is a text frame
            for (i = 0; text_frames[i]; i++) {
                if (!strcmp (f23->id, text_frames[i])) {
                    text = 1;
                    break;
                }
            }
        }


        if (!simplecopy && !text) {
            continue; // unknown frame
        }

        // convert flags
        uint8_t flags[2];
        // 1st byte (status flags) is the same, but shifted by 1 bit to the
        // right
        flags[0] = f23->flags[0] >> 1;
        
        // 2nd byte (format flags) is quite different
        // 2.4 format is %0h00kmnp (grouping, compression, encryption, unsync)
        // 2.3 format is %ijk00000 (compression, encryption, grouping)
        flags[1] = 0;
        if (f23->flags[1] & (1 << 4)) {
            flags[1] |= (1 << 6);
        }
        if (f23->flags[1] & (1 << 7)) {
            flags[1] |= (1 << 3);
        }
        if (f23->flags[1] & (1 << 6)) {
            flags[1] |= (1 << 2);
        }
        if (f23->flags[1] & (1 << 5)) {
            flags[1] |= (1 << 1);
        }

        DB_id3v2_frame_t *f24 = NULL;
        if (simplecopy) {
            f24 = malloc (sizeof (DB_id3v2_frame_t) + f23->size);
            memset (f24, 0, sizeof (DB_id3v2_frame_t) + f23->size);
            strcpy (f24->id, f23->id);
            f24->size = f23->size;
            memcpy (f24->data, f23->data, f23->size);
            f24->flags[0] = flags[0];
            f24->flags[1] = flags[1];
        }
        else if (text) {
            // decode text into utf8
            char *decoded = convstr_id3v2 (3, f23->data[0], f23->data+1, f23->size-1);
            if (!decoded) {
                trace ("junk_id3v2_convert_23_to_24: failed to decode text frame %s\n", f23->id);
                continue; // failed, discard it
            }
            if (!strcmp (f23->id, "TDRC")) {
                trace ("junk_id3v2_convert_23_to_24: TDRC text: %s\n", decoded);
                int year, month, day;
                int c = sscanf (decoded, "%4d-%2d-%2d", &year, &month, &day);
                if (c >= 1) {
                    char s[5];
                    snprintf (s, sizeof (s), "%04d", year);
                    f24 = junk_id3v2_add_text_frame_24 (tag24, "TYER", s);
                    if (f24) {
                        tail = f24;
                        f24 = NULL;
                    }
                }
                if (c == 3) {
                    char s[5];
                    snprintf (s, sizeof (s), "%02d%02d", month, day);
                    f24 = junk_id3v2_add_text_frame_24 (tag24, "TDAT", s);
                    if (f24) {
                        tail = f24;
                        f24 = NULL;
                    }
                }
                else {
                    trace ("junk_id3v2_add_text_frame_24: 2.4 TDRC doesn't have month/day info; discarded\n");
                }
            }
            else if (!strcmp (f23->id, "TORY")) {
                trace ("junk_id3v2_convert_23_to_24: TDOR text: %s\n", decoded);
                int year;
                int c = sscanf (decoded, "%4d", &year);
                if (c == 1) {
                    char s[5];
                    snprintf (s, sizeof (s), "%04d", &year);
                    f24 = junk_id3v2_add_text_frame_24 (tag24, "TDOR", s);
                    if (f24) {
                        tail = f24;
                        f24 = NULL;
                    }
                }
                else {
                    trace ("junk_id3v2_add_text_frame_24: 2.4 TDOR doesn't have month/day info; discarded\n");
                }
            }
            else {
                // encode for 2.3
                f24 = junk_id3v2_add_text_frame_24 (tag24, f23->id, decoded);
                if (f24) {
                    tail = f24;
                    f24 = NULL;
                }
            }
            free (decoded);
        }
        if (f24) {
            if (tail) {
                tail->next = f24;
            }
            else {
                tag24->frames = f24;
            }
            tail = f24;
        }
    }

    // convert tag header
    tag24->version[0] = 4;
    tag24->version[1] = 0;
    tag24->flags = tag23->flags;
    tag24->flags &= ~(1<<4); // no footer (unsupported in 2.3)
    tag24->flags &= ~(1<<7); // no unsync

    return 0;
}

int
junk_id3v2_convert_22_to_24 (DB_id3v2_tag_t *tag22, DB_id3v2_tag_t *tag24) {
    DB_id3v2_frame_t *f22;
    DB_id3v2_frame_t *tail = tag24->frames;

    while (tail && tail->next) {
        tail = tail->next;
    }

    const char *copy_frames[] = {
        "BUF", "COM", "CRA", "ETC", "GEO", "IPL", "MCI", "MLL", "POP", "REV", "SLT", "STC", "UFI", "ULT",
        NULL
    };

    const char *copy_frames_24[] = {
        "RBUF", "COMM", "AENC", "ETCO", "GEOB", "TIPL", "MCDI", "MLLT", "POPM", "RVRB", "SYLT", "SYTC", "UFID", "USLT",
        NULL
    };

    // NOTE: BUF is discarded (no match in 2.4)
    // NOTE: CNT is discarded (useless)
    // NOTE: CRM is discarded (no match in 2.4)
    // NOTE: EQU is discarded (difficult to convert to EQU2)
    // NOTE: LNK is discarded (maybe later)
    // NOTE: PIC is discarded (needs conversion from custom image-format field to mime-type)
    // NOTE: RVA is discarded (subjective, and difficult to convert to RVA2)

    const char *text_frames[] = {
        "TAL", "TBP", "TCM", "TCO", "TCR", "TDY", "TEN", "TFT", "TKE", "TLA", "TLE", "TMT", "TOA", "TOF", "TOL", "TOT", "TP1", "TP2", "TP3", "TP4", "TPA", "TPB", "TRC", "TRK", "TSS", "TT1", "TT2", "TT3", "TXT", "TXX", "TOR", NULL
    };

    const char *text_frames_24[] = {
        "TALB", "TBPM", "TCOM", "TCON", "TCOP", "TDLY", "TENC", "TFLT", "TKEY", "TLAN", "TLEN", "TMED", "TOPE", "TOFN", "TOLY", "TOAL", "TPE1", "TPE2", "TPE3", "TPE4", "TPOS", "TPUB", "TSRC", "TRCK", "TSSE", "TIT1", "TIT2", "TIT3", "TEXT", "TXXX", "TDOR"
    };

    // NOTE: TRD is discarded (no match in 2.4)
    // NOTE: TSI is discarded (no match in 2.4)

    int year = 0;
    int month = 0;
    int day = 0;
    int hour = 0;
    int minute = 0;

    for (f22 = tag22->frames; f22; f22 = f22->next) {
        int simplecopy = -1; // means format is the same in 2.2 and 2.4
        int text = -1; // means this is a text frame

        int i;

        if (f22->id[0] == 'W') { // covers all W00..WZZ tags
            simplecopy = 0;
        }

        if (simplecopy == -1) {
            for (i = 0; copy_frames[i]; i++) {
                if (!strcmp (f22->id, copy_frames[i])) {
                    simplecopy = i;
                    break;
                }
            }
        }

        if (simplecopy == -1) {
            // check if this is a text frame
            for (i = 0; text_frames[i]; i++) {
                if (!strcmp (f22->id, text_frames[i])) {
                    text = i;
                    break;
                }
            }
        }


        if (simplecopy == -1 && text == -1) {
            continue; // unknown frame
        }

        // convert flags
        uint8_t flags[2];
        // 1st byte (status flags) is the same, but shifted by 1 bit to the
        // right
        flags[0] = f22->flags[0] >> 1;
        
        // 2nd byte (format flags) is quite different
        // 2.4 format is %0h00kmnp (grouping, compression, encryption, unsync)
        // 2.3 format is %ijk00000 (compression, encryption, grouping)
        flags[1] = 0;
        if (f22->flags[1] & (1 << 4)) {
            flags[1] |= (1 << 6);
        }
        if (f22->flags[1] & (1 << 7)) {
            flags[1] |= (1 << 3);
        }
        if (f22->flags[1] & (1 << 6)) {
            flags[1] |= (1 << 2);
        }
        if (f22->flags[1] & (1 << 5)) {
            flags[1] |= (1 << 1);
        }

        DB_id3v2_frame_t *f24 = NULL;
        if (simplecopy != -1) {
            f24 = malloc (sizeof (DB_id3v2_frame_t) + f22->size);
            memset (f24, 0, sizeof (DB_id3v2_frame_t) + f22->size);
            if (f22->id[0] == 'W') { // duplicate last letter of W00-WZZ frames
                strcpy (f24->id, f22->id);
                f24->id[3] = f24->id[2];
                f24->id[4] = 0;
            }
            else {
                strcpy (f24->id, copy_frames_24[simplecopy]);
            }
            f24->size = f22->size;
            memcpy (f24->data, f22->data, f22->size);
            f24->flags[0] = flags[0];
            f24->flags[1] = flags[1];
        }
        else if (text != -1) {
            // decode text into utf8
            char *decoded = convstr_id3v2 (2, f22->data[0], f22->data+1, f22->size-1);
            if (!decoded) {
                trace ("junk_id3v2_convert_23_to_24: failed to decode text frame %s\n", f22->id);
                continue; // failed, discard it
            }
            // encode for 2.4
            f24 = junk_id3v2_add_text_frame_24 (tag24, text_frames_24[text], decoded);
            if (f24) {
                tail = f24;
                f24 = NULL;
            }
            free (decoded);
        }
        else if (!strcmp (f22->id, "TYE")) {
            char *decoded = convstr_id3v2 (2, f22->data[0], f22->data+1, f22->size-1);
            if (!decoded) {
                year = atoi (decoded);
                free (decoded);
            }
        }
        else if (!strcmp (f22->id, "TDA")) {
            char *decoded = convstr_id3v2 (2, f22->data[0], f22->data+1, f22->size-1);
            if (!decoded) {
                sscanf (decoded, "%02d02d", &month, &day);
                free (decoded);
            }
        }
        else if (!strcmp (f22->id, "TIM")) {
            char *decoded = convstr_id3v2 (2, f22->data[0], f22->data+1, f22->size-1);
            if (!decoded) {
                sscanf (decoded, "%02d02d", &hour, &minute);
                free (decoded);
            }
        }
        if (f24) {
            if (tail) {
                tail->next = f24;
            }
            else {
                tag24->frames = f24;
            }
            tail = f24;
        }
    }

    char tdrc[100];
    char *p = tdrc;
    if (year > 0) {
        int n = sprintf (p, "%04d", year);
        p += n;
        if (month) {
            n = sprintf (p, "-%02d", month);
            p += n;
            if (day) {
                n = sprintf (p, "-%02d", day);
                p += n;
                if (hour && minute) {
                    n = sprintf (p, "-T%02d:02d", hour, minute);
                    p += n;
                }
            }
        }
        DB_id3v2_frame_t *f24 = junk_id3v2_add_text_frame_24 (tag24, "TDRC", tdrc);
        if (f24) {
            tail = f24;
        }
    }

    // convert tag header
    tag24->version[0] = 4;
    tag24->version[1] = 0;
    tag24->flags = tag22->flags;
    tag24->flags &= ~(1<<4); // no footer (unsupported in 2.3)
    tag24->flags &= ~(1<<7); // no unsync

    return 0;
}

int
junk_apev2_remove_frames (DB_apev2_tag_t *tag, const char *frame_id) {
    DB_apev2_frame_t *prev = NULL;
    for (DB_apev2_frame_t *f = tag->frames; f; ) {
        DB_apev2_frame_t *next = f->next;
        if (!strcmp (f->key, frame_id)) {
            if (prev) {
                prev->next = f->next;
            }
            else {
                tag->frames = f->next;
            }
            free (f);
        }
        else {
            prev = f;
        }
        f = next;
    }
    return 0;
}

DB_apev2_frame_t *
junk_apev2_add_text_frame (DB_apev2_tag_t *tag, const char *frame_id, const char *value) {
    if (!*value) {
        return NULL;
    }
    DB_apev2_frame_t *tail = tag->frames;
    while (tail && tail->next) {
        tail = tail->next;
    }

    int size = strlen (value);
    DB_apev2_frame_t *f = malloc (sizeof (DB_apev2_frame_t) + size);
    if (!f) {
        trace ("junk_apev2_add_text_frame: failed to allocate %d bytes\n", size);
        return NULL;
    }
    memset (f, 0, sizeof (DB_apev2_frame_t));
    f->flags = 0;
    strcpy (f->key, frame_id);
    f->size = size;
    memcpy (f->data, value, size);

    if (tail) {
        tail->next = f;
    }
    else {
        tag->frames = f;
    }
    return f;
}

int
junk_id3v2_convert_apev2_to_24 (DB_apev2_tag_t *ape, DB_id3v2_tag_t *tag24) {
    DB_apev2_frame_t *f_ape;
    DB_id3v2_frame_t *tail = tag24->frames;

    while (tail && tail->next) {
        tail = tail->next;
    }

    const char *text_keys[] = {
        "Title", "Subtitle", "Artist", "Album", "Publisher", "Conductor", "Track", "Composer", "Copyright", "Genre", "Media", "ISRC", "Language", "Year", NULL
    };

    const char *text_keys_24[] = {
        "TIT2", "TIT3", "TPE1", "TALB", "TPUB", "TPE3", "TRCK", "TCOM", "TCOP", "TCON", "TPOS", "TSRC", "TLAN", "TDRC"
    };

    const char *comm_frames[] = {
        "Comment", "EAN/UPC", "ISBN", "Catalog", "LC", "Publicationright", "Record Location", "Related", "Abstract", "Bibliography", NULL
    };

    // FIXME: additional frames: File->WOAF 
    // converted to COMM: Comment, EAN/UPC, ISBN, Catalog, LC, Publicationright, Record Location, Related, Abstract, Bibliography
    // "Debut album" is discarded
    // "Index" is discarded
    // "Introplay" is discarded

    for (f_ape = ape->frames; f_ape; f_ape = f_ape->next) {
        int i;

        for (i = 0; text_keys[i]; i++) {
            if (!strcasecmp (text_keys[i], f_ape->key)) {
                break;
            }
        }

        DB_id3v2_frame_t *f24 = NULL;

        if (text_keys[i]) {
            char str[f_ape->size+1];
            memcpy (str, f_ape->data, f_ape->size);
            str[f_ape->size] = 0;
            f24 = junk_id3v2_add_text_frame_24 (tag24, text_keys_24[i], str);
            if (f24) {
                tail = f24;
                f24 = NULL;
            }
        }
        else {
            for (i = 0; comm_frames[i]; i++) {
                if (!strcasecmp (f_ape->key, comm_frames[i])) {
                    char str[f_ape->size+1];
                    memcpy (str, f_ape->data, f_ape->size);
                    str[f_ape->size] = 0;
                    if (!strcasecmp (f_ape->key, "Comment")) {
                        junk_id3v2_add_comment_frame_24 (tag24, "eng", "", str);
                    }
                    else {
                        junk_id3v2_add_comment_frame_24 (tag24, "eng", comm_frames[i], str);
                    }
                    break;
                }
            }
        }

        if (f24) {
            if (tail) {
                tail->next = f24;
            }
            else {
                tag24->frames = f24;
            }
            tail = f24;
        }
    }

    // convert tag header
    tag24->version[0] = 4;
    tag24->version[1] = 0;
    tag24->flags = 0;

    return 0;
}

int
junk_apev2_write_i32_le (FILE *fp, uint32_t data) {
    int shift = 0;
    for (int i = 0; i < 4; i++) {
        uint8_t d = (data >> shift) & 0xff;
        if (fwrite (&d, 1, 1, fp) != 1) {
            return -1;
        }
        shift += 8;
    }

    return 0;
}

int
junk_apev2_write (FILE *fp, DB_apev2_tag_t *tag, int write_header, int write_footer) {
    // calc size and numitems
    uint32_t numframes = 0;
    uint32_t size = 0;
    DB_apev2_frame_t *f = tag->frames;
    while (f) {
        size += 8 + strlen (f->key) + 1 + f->size;
        numframes++;
        f = f->next;
    }
    size += 32;

    trace ("junk_apev2_write: writing apev2 tag, size=%d, numframes=%d\n", size, numframes);


    if (write_header) {
        if (fwrite ("APETAGEX", 1, 8, fp) != 8) {
            trace ("junk_apev2_write: failed to write apev2 header signature\n");
            goto error;
        }
        uint32_t flags = (1 << 31) | (1 << 29); // contains header, this is header
        if (!write_footer) {
            flags |= 1 << 30; // contains no footer
        }
        uint32_t header[4] = {
            2000, // version
            size,
            numframes,
            flags
        };
        for (int i = 0; i < 4; i++) {
            if (junk_apev2_write_i32_le (fp, header[i]) != 0) {
                trace ("junk_apev2_write_i32_le: failed to write apev2 header\n");
                goto error;
            }
        }
        // write 8 bytes of padding
        header[0] = header[1] = 0;
        if (fwrite (header, 1, 8, fp) != 8) {
            trace ("junk_apev2_write_i32_le: failed to write apev2 header padding\n");
            goto error;
        }
    }

    // write items
    f = tag->frames;
    while (f) {
        if (junk_apev2_write_i32_le (fp, f->size) != 0) {
            trace ("junk_apev2_write_i32_le: failed to write apev2 item size\n");
            goto error;
        }
        if (junk_apev2_write_i32_le (fp, f->flags) != 0) {
            trace ("junk_apev2_write_i32_le: failed to write apev2 item flags\n");
            goto error;
        }
        int l = strlen (f->key) + 1;
        if (fwrite (f->key, 1, l, fp) != l) {
            trace ("junk_apev2_write_i32_le: failed to write apev2 item key\n");
            goto error;
        }
        if (fwrite (f->data, 1, f->size, fp) != f->size) {
            trace ("junk_apev2_write_i32_le: failed to write apev2 item value\n");
            goto error;
        }
        f = f->next;
    }

    if (write_footer) {
        if (fwrite ("APETAGEX", 1, 8, fp) != 8) {
            trace ("junk_apev2_write: failed to write apev2 footer signature\n");
            goto error;
        }
        uint32_t flags = 0;
        if (write_header) {
            flags |= 1 << 31;
        }
        uint32_t header[4] = {
            2000, // version
            size,
            numframes,
            flags
        };
        for (int i = 0; i < 4; i++) {
            if (junk_apev2_write_i32_le (fp, header[i]) != 0) {
                trace ("junk_apev2_write_i32_le: failed to write apev2 footer\n");
                goto error;
            }
        }
        // write 8 bytes of padding
        header[0] = header[1] = 0;
        if (fwrite (header, 1, 8, fp) != 8) {
            trace ("junk_apev2_write_i32_le: failed to write apev2 footer padding\n");
            goto error;
        }
    }
    return 0;
error:
    return -1;
}

int
junk_id3v2_write (FILE *out, DB_id3v2_tag_t *tag) {
    if (tag->version[0] < 3) {
        fprintf (stderr, "junk_write_id3v2: writing id3v2.2 is not supported\n");
        return -1;
    }

    FILE *fp = NULL;
    char *buffer = NULL;
    int err = -1;

    // write tag header
    if (fwrite ("ID3", 1, 3, out) != 3) {
        fprintf (stderr, "junk_write_id3v2: failed to write ID3 signature\n");
        goto error;
    }

    if (fwrite (tag->version, 1, 2, out) != 2) {
        fprintf (stderr, "junk_write_id3v2: failed to write tag version\n");
        goto error;
    }
    uint8_t flags = tag->flags;
    flags &= ~(1<<6); // we don't (yet?) write ext header
    flags &= ~(1<<4); // we don't write footer

    if (fwrite (&flags, 1, 1, out) != 1) {
        fprintf (stderr, "junk_write_id3v2: failed to write tag flags\n");
        goto error;
    }
    // run through list of frames, and calculate size
    uint32_t sz = 0;
    int frm = 0;
    for (DB_id3v2_frame_t *f = tag->frames; f; f = f->next) {
        // each tag has 10 bytes header
        if (tag->version[0] > 2) {
            sz += 10;
        }
        else {
            sz += 6;
        }
        sz += f->size;
    }

    trace ("calculated tag size: %d bytes\n", sz);
    uint8_t tagsize[4];
    tagsize[0] = (sz >> 21) & 0x7f;
    tagsize[1] = (sz >> 14) & 0x7f;
    tagsize[2] = (sz >> 7) & 0x7f;
    tagsize[3] = sz & 0x7f;
    if (fwrite (tagsize, 1, 4, out) != 4) {
        fprintf (stderr, "junk_write_id3v2: failed to write tag size\n");
        goto error;
    }

    trace ("writing frames\n");
    // write frames
    for (DB_id3v2_frame_t *f = tag->frames; f; f = f->next) {
        trace ("writing frame %s size %d\n", f->id, f->size);
        int id_size = 3;
        uint8_t frame_size[4];
        if (tag->version[0] > 2) {
            id_size = 4;
        }
        if (tag->version[0] == 3) {
            frame_size[0] = (f->size >> 24) & 0xff;
            frame_size[1] = (f->size >> 16) & 0xff;
            frame_size[2] = (f->size >> 8) & 0xff;
            frame_size[3] = f->size & 0xff;
        }
        else if (tag->version[0] == 4) {
            frame_size[0] = (f->size >> 21) & 0x7f;
            frame_size[1] = (f->size >> 14) & 0x7f;
            frame_size[2] = (f->size >> 7) & 0x7f;
            frame_size[3] = f->size & 0x7f;
        }
        if (fwrite (f->id, 1, 4, out) != 4) {
            fprintf (stderr, "junk_write_id3v2: failed to write frame id %s\n", f->id);
            goto error;
        }
        if (fwrite (frame_size, 1, 4, out) != 4) {
            fprintf (stderr, "junk_write_id3v2: failed to write frame size, id %s, size %d\n", f->id, f->size);
            goto error;
        }
        if (fwrite (f->flags, 1, 2, out) != 2) {
            fprintf (stderr, "junk_write_id3v2: failed to write frame header flags, id %s, size %s\n", f->id, f->size);
            goto error;
        }
        if (fwrite (f->data, 1, f->size, out) != f->size) {
            fprintf (stderr, "junk_write_id3v2: failed to write frame data, id %s, size %s\n", f->id, f->size);
            goto error;
        }
        sz += f->size;
    }

    return 0;

error:
    if (buffer) {
        free (buffer);
    }
    return err;
}

void
junk_id3v2_free (DB_id3v2_tag_t *tag) {
    while (tag->frames) {
        DB_id3v2_frame_t *next = tag->frames->next;
        free (tag->frames);
        tag->frames = next;
    }
}

void
junk_apev2_free (DB_apev2_tag_t *tag) {
    while (tag->frames) {
        DB_apev2_frame_t *next = tag->frames->next;
        free (tag->frames);
        tag->frames = next;
    }
}

int
junklib_id3v2_sync_frame (uint8_t *data, int size) {
    char *writeptr = data;
    int skipnext = 0;
    int written = 0;
    while (size > 0) {
        *writeptr = *data;
        if (data[0] == 0xff && size >= 2 && data[1] == 0) {
            data++;
            size--;
        }
        writeptr++;
        data++;
        size--;
        written++;
    }
    return written;
}

char *
junk_append_meta (const char *old, const char *new) {
    int sz = strlen (old) + strlen (new) + 2;
    char *appended = malloc (sz);
    if (!appended) {
        trace ("junk_append_meta: failed to allocate %d bytes\n");
        return NULL;
    }
    snprintf (appended, sz, "%s\n%s", old, new);
    return appended;
}

int
junk_load_comm_frame (int version_major, playItem_t *it, uint8_t *readptr, int synched_size, char **pcomment) {
    char *comment = *pcomment;
    uint8_t enc = readptr[0];
    char lang[4] = {readptr[1], readptr[2], readptr[3], 0};
    trace ("COMM enc is: %d\n", (int)enc);
    trace ("COMM language is: %s\n", lang);

    uint8_t *pdescr = readptr+4;
    // find value field
    uint8_t *pvalue = pdescr;
    if ((enc == 3 && version_major == 4) || enc == 0) {
        while (pvalue - pdescr < synched_size-4 && *pvalue) {
            pvalue++;
        }
        if (pvalue - pdescr >= synched_size-4) {
            pvalue = NULL;
        }
        else {
            pvalue++;
        }
    }
    else {
        // unicode, find 0x00 0x00 0xXX
        while (pvalue - pdescr + 2 < synched_size-4 && (pvalue[0] || pvalue[1] || !pvalue[2])) {
            trace ("byte: %X\n", *pvalue);
            pvalue++;
        }
        if (pvalue - pdescr + 2 >= synched_size-4) {
            trace ("out of bounds: size=%d, framesize=%d\n", pvalue - pdescr + 1, synched_size-4);
            pvalue = NULL;
        }
        else {
            pvalue += 2;
        }
    }

    if (!pvalue) {
        trace ("failed to parse COMM frame\n");
        return -1;
    }

    trace ("COMM descr length: %d\n", pvalue-pdescr);
    char *descr = convstr_id3v2 (version_major, enc, pdescr, pvalue - pdescr);
    trace ("COMM descr: %s\n", descr);
    if (!descr) {
        trace ("failed to decode COMM descr\n");
        return -1;
    }

    int valsize = synched_size - 4 - (pvalue - pdescr);
    trace ("valsize=%d\n", valsize);
    char *text = convstr_id3v2 (version_major, enc, pvalue, valsize);
    trace ("COMM text: %s\n", text);
    if (!text) {
        trace ("failed to decode COMM text\n");
        return -1;
    }

    int len = (comment ? (strlen (comment) + 1) : 0) + strlen (descr) + strlen (text) + 3;
    char *newcomment = malloc (len);

    if (*descr) {
        if (comment) {
            snprintf (newcomment, len, "%s\n%s: %s", comment, descr, text);
        }
        else {
            snprintf (newcomment, len, "%s: %s", descr, text);
        }
    }
    else {
        if (comment) {
            snprintf (newcomment, len, "%s\n%s", comment, text);
        }
        else {
            snprintf (newcomment, len, "%s", text);
        }
    }
    if (comment) {
        free (comment);
    }
    comment = newcomment;
    trace ("COMM combined: %s\n", comment);
    *pcomment = comment;
    return 0;
}

int
junk_id3v2_load_txx (int version_major, playItem_t *it, uint8_t *readptr, int synched_size) {
    uint8_t *p = readptr;
    uint8_t encoding = *p;
    p++;
    uint8_t *desc = p;
    int desc_sz = 0;
    while (*p && p - readptr < synched_size) {
        p++;
        desc_sz++;
    }
    p++;
    if (p - readptr >= synched_size) {
        trace ("bad TXXX frame, skipped\n");
        return -1;
    }
    // FIXME: decode properly using frame encoding
    char *desc_s = desc;
    char *value_s = p;
    //trace ("value=%s\n", value_s);
    if (!strcasecmp (desc_s, "replaygain_album_gain")) {
        it->replaygain_album_gain = atof (value_s);
        trace ("%s=%s (%f)\n", desc_s, value_s, it->replaygain_album_gain);
    }
    else if (!strcasecmp (desc_s, "replaygain_album_peak")) {
        it->replaygain_album_peak = atof (value_s);
        trace ("%s=%s (%f)\n", desc_s, value_s, it->replaygain_album_peak);
    }
    else if (!strcasecmp (desc_s, "replaygain_track_gain")) {
        it->replaygain_track_gain = atof (value_s);
        trace ("%s=%s (%f)\n", desc_s, value_s, it->replaygain_track_gain);
    }
    else if (!strcasecmp (desc_s, "replaygain_track_peak")) {
        it->replaygain_track_peak = atof (value_s);
        trace ("%s=%s (%f)\n", desc_s, value_s, it->replaygain_track_peak);
    }
}

int
junk_id3v2_read_full (playItem_t *it, DB_id3v2_tag_t *tag_store, DB_FILE *fp) {
    DB_id3v2_frame_t *tail = NULL;
    int title_added = 0;
    if (!fp) {
        trace ("bad call to junk_id3v2_read!\n");
        return -1;
    }
    deadbeef->rewind (fp);
    uint8_t header[10];
    if (deadbeef->fread (header, 1, 10, fp) != 10) {
        return -1; // too short
    }
    if (strncmp (header, "ID3", 3)) {
        return -1; // no tag
    }
    uint8_t version_major = header[3];
    uint8_t version_minor = header[4];
    if (version_major > 4 || version_major < 2) {
        trace ("id3v2.%d.%d is unsupported\n", version_major, version_minor);
        return -1; // unsupported
    }
    uint8_t flags = header[5];
    if (flags & 15) {
        trace ("unrecognized flags: one of low 15 bits is set, value=0x%x\n", (int)flags);
        return -1; // unsupported
    }
    int unsync = (flags & (1<<7)) ? 1 : 0;
    int extheader = (flags & (1<<6)) ? 1 : 0;
    int expindicator = (flags & (1<<5)) ? 1 : 0;
    int footerpresent = (flags & (1<<4)) ? 1 : 0;
    // check for bad size
    if ((header[9] & 0x80) || (header[8] & 0x80) || (header[7] & 0x80) || (header[6] & 0x80)) {
        trace ("bad header size\n");
        return -1; // bad header
    }
    uint32_t size = (header[9] << 0) | (header[8] << 7) | (header[7] << 14) | (header[6] << 21);

    trace ("tag size: %d\n", size);
    if (tag_store) {
        tag_store->version[0] = version_major;
        tag_store->version[1] = version_minor;
        tag_store->flags = flags;
        // remove unsync flag
        tag_store->flags &= ~ (1<<7);
    }

    uint8_t *tag = malloc (size);
    if (!tag) {
        fprintf (stderr, "junklib: out of memory while reading id3v2, tried to alloc %d bytes\n", size);
    }
    if (deadbeef->fread (tag, 1, size, fp) != size) {
        goto error; // bad size
    }
    uint8_t *readptr = tag;
    int crcpresent = 0;
    trace ("version: 2.%d.%d, unsync: %d, extheader: %d, experimental: %d\n", version_major, version_minor, unsync, extheader, expindicator);
    
    if (extheader) {
        uint32_t sz = (readptr[3] << 0) | (header[2] << 8) | (header[1] << 16) | (header[0] << 24);
        //if (size < 6) {
        //    goto error; // bad size
        //}

        uint32_t padding = (readptr[9] << 0) | (header[8] << 8) | (header[7] << 16) | (header[6] << 24);
        size -= padding;

        if (size < sz) {
            return -1; // bad size
        }
        readptr += sz;
#if 0
        uint16_t extflags = (readptr[1] << 0) | (readptr[0] << 8);
        readptr += 2;
        uint32_t pad = (readptr[3] << 0) | (header[2] << 8) | (header[1] << 16) | (header[0] << 24);
        readptr += 4;
        if (extflags & 0x8000) {
            crcpresent = 1;
        }
        if (crcpresent && sz != 10) {
            return -1; // bad header
        }
        readptr += 4; // skip crc
#endif
    }
    char *artist = NULL;
    char *album = NULL;
    char *band = NULL;
    char *track = NULL;
    char *title = NULL;
    char *year = NULL;
    char *vendor = NULL;
    char *comment = NULL;
    char *copyright = NULL;
    char *genre = NULL;
    char *performer = NULL;
    char *composer = NULL;
    char *disc = NULL;
    int err = 0;
    while (readptr - tag <= size - 4) {
        if (version_major == 3 || version_major == 4) {
            char frameid[5];
            memcpy (frameid, readptr, 4);
            frameid[4] = 0;
            readptr += 4;
            if (readptr - tag >= size - 4) {
                break;
            }
            uint32_t sz;
            if (version_major == 4) {
                sz = (readptr[3] << 0) | (readptr[2] << 7) | (readptr[1] << 14) | (readptr[0] << 21);
            }
            else if (version_major == 3) {
                sz = (readptr[3] << 0) | (readptr[2] << 8) | (readptr[1] << 16) | (readptr[0] << 24);
            }
            else {
                trace ("unknown id3v2 version (2.%d.%d)\n", version_major, version_minor);
                return -1;
            }
            readptr += 4;
            trace ("got frame %s, size %d, pos %d, tagsize %d\n", frameid, sz, readptr-tag, size);
            if (readptr - tag >= size - sz) {
                trace ("frame is out of tag bounds\n");
                err = 1;
                break; // size of frame is more than size of tag
            }
            if (sz < 1) {
//                err = 1;
                break; // frame must be at least 1 byte long
            }
            uint8_t flags1 = readptr[0];
            uint8_t flags2 = readptr[1];
            readptr += 2;

            int synched_size = sz;
            if (unsync) {
                synched_size = junklib_id3v2_sync_frame (readptr, sz);
            }

            if (sz > MAX_ID3V2_FRAME_SIZE) {
                trace ("junk_id3v2_read_full: frame %s size is too big, discarded\n", frameid);
                readptr += sz;
                continue;
            }
            if (tag_store) {
                DB_id3v2_frame_t *frm = malloc (sizeof (DB_id3v2_frame_t) + sz);
                if (!frm) {
                    fprintf (stderr, "junklib: failed to alloc %d bytes for id3v2 frame %s\n", sizeof (DB_id3v2_frame_t) + sz, frameid);
                    goto error;
                }
                memset (frm, 0, sizeof (DB_id3v2_frame_t));
                if (tail) {
                    tail->next = frm;
                }
                tail = frm;
                if (!tag_store->frames) {
                    tag_store->frames = frm;
                }
                strcpy (frm->id, frameid);
                memcpy (frm->data, readptr, sz);
                frm->size = synched_size;

                frm->flags[0] = flags1;
                frm->flags[1] = flags2;
            }
            if (version_major == 4) {
                if (flags1 & 0x8f) {
                    // unknown flags
                    trace ("unknown status flags: %02x\n", flags1);
                    readptr += sz;
                    continue;
                }
                if (flags2 & 0xb0) {
                    // unknown flags
                    trace ("unknown format flags: %02x\n", flags2);
                    readptr += sz;
                    continue;
                }

                if (flags2 & 0x40) { // group id
                    trace ("frame has group id\n");
                    readptr++; // skip id
                    sz--;
                }
                if (flags2 & 0x08) { // compressed frame, ignore
                    trace ("frame is compressed, skipping\n");
                    readptr += sz;
                    continue;
                }
                if (flags2 & 0x04) { // encrypted frame, skip
                    trace ("frame is encrypted, skipping\n");
                    readptr += sz;
                    continue;
                }
                if (flags2 & 0x02) { // unsync, just do nothing
                }
                if (flags2 & 0x01) { // data size
                    uint32_t size = extract_i32 (readptr);
                    trace ("frame has extra size field = %d\n", size);
                    readptr += 4;
                    sz -= 4;
                }
            }
            else if (version_major == 3) {
                if (flags1 & 0x1F) {
                    trace ("unknown status flags: %02x\n", flags1);
                    readptr += sz;
                    continue;
                }
                if (flags2 & 0x1F) {
                    trace ("unknown format flags: %02x\n", flags2);
                    readptr += sz;
                    continue;
                }
                if (flags2 & 0x80) {
                    trace ("frame is compressed, skipping\n");
                    readptr += sz;
                    continue;
                }
                if (flags2 & 0x40) {
                    trace ("frame is encrypted, skipping\n");
                    readptr += sz;
                    continue;
                }
                if (flags2 & 0x20) {
                    trace ("frame has group id\n");
                    readptr++; // skip id
                    sz--;
                }
            }

            // parse basic 2.3/2.4 text frames
            const char *text_frames[] = { "TPE1", "TPE2", "TPOS", "TIT2", "TALB", "TCOP", "TCON", "TENC", "TPE3", "TCOM", "TRCK", "TYER", "TDRC", NULL };
            char **text_holders[] = { &artist, &band, &disc, &title, &album, &copyright, &genre, &vendor, &performer, &composer, &track, version_major == 3 ? &year : NULL,  version_major == 4 ? &year : NULL, };
            int f = 0;
            for (f = 0; text_frames[f]; f++) {
                if (!strcmp (frameid, text_frames[f])) {
                    if (synched_size > MAX_TEXT_FRAME_SIZE) {
                        trace ("frame %s is too big, discard\n", frameid);
                        break;
                    }
                    char *text = convstr_id3v2 (version_major, readptr[0], readptr+1, synched_size-1);
                    if (text && text_holders[f]) {
                        if (*text_holders[f]) {
                            // append
                            char *new = junk_append_meta (*text_holders[f], text);
                            if (new) {
                                free (*text_holders[f]);
                                *text_holders[f] = new;
                            }
                            free (text);
                        }
                        else {
                            *text_holders[f] = text;
                        }
                    }
                    if (text) {
                        trace ("%s = %s\n", frameid, text);
                    }
                    break;
                }
            }

            if (!strcmp (frameid, "COMM")) {
                if (sz < 4) {
                    trace ("COMM frame is too short, skipped\n");
                    readptr += sz; // bad tag
                    continue;
                }

                /*int res = */junk_load_comm_frame (version_major, it, readptr, synched_size, &comment);
            }
            else if (it && !strcmp (frameid, "TXXX")) {
                if (synched_size < 2) {
                    trace ("TXXX frame is too short, skipped\n");
                    readptr += sz; // bad tag
                    continue;
                }
                int res = junk_id3v2_load_txx (version_major, it, readptr, synched_size);
            }
            readptr += sz;
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
            if (sz > MAX_ID3V2_FRAME_SIZE) {
                trace ("junk_id3v2_read_full: frame %s size is too big, discarded\n", frameid);
                readptr += sz;
                continue;
            }
            int synched_size = sz;
            if (unsync) {
                synched_size = junklib_id3v2_sync_frame (readptr, sz);
            }

            if (tag_store) {
                DB_id3v2_frame_t *frm = malloc (sizeof (DB_id3v2_frame_t) + sz);
                if (!frm) {
                    fprintf (stderr, "junklib: failed to alloc %d bytes for id3v2.2 frame %s\n", sizeof (DB_id3v2_frame_t) + sz, frameid);
                    goto error;
                }
                memset (frm, 0, sizeof (DB_id3v2_frame_t));
                if (tail) {
                    tail->next = frm;
                }
                tail = frm;
                if (!tag_store->frames) {
                    tag_store->frames = frm;
                }
                strcpy (frm->id, frameid);
                memcpy (frm->data, readptr, synched_size);
                frm->size = sz;
            }
//            trace ("found id3v2.2 frame: %s, size=%d\n", frameid, sz);

            // parse basic 2.2 text frames
            const char *text_frames[] = { "TEN", "TT2", "TAL", "TP1", "TP2", "TP3", "TCM", "TPA", "TRK", "TYE", "TCR", "TCO", NULL };
            char **text_holders[] = { &vendor, &title, &album, &artist, &band, &performer, &composer, &disc, &track, &year, &copyright, &genre };
            int f = 0;
            for (f = 0; text_frames[f]; f++) {
                if (!strcmp (frameid, text_frames[f])) {
                    if (synched_size > MAX_TEXT_FRAME_SIZE) {
                        trace ("frame %s is too big, discard\n", frameid);
                        break;
                    }
                    char *text = convstr_id3v2 (version_major, readptr[0], readptr+1, synched_size-1);
                    if (text && text_holders[f]) {
                        if (*text_holders[f]) {
                            // append
                            char *new = junk_append_meta (*text_holders[f], text);
                            if (new) {
                                free (*text_holders[f]);
                                *text_holders[f] = new;
                            }
                            free (text);
                        }
                        else {
                            *text_holders[f] = text;
                        }
                    }
                    break;
                }
            }

            if (!strcmp (frameid, "COM")) {
                if (synched_size < 6) {
                    readptr += sz;
                    continue;
                }
                /*int res = */junk_load_comm_frame (version_major, it, readptr, synched_size, &comment);
            }
            else if (it && !strcmp (frameid, "TXX")) {
                if (synched_size < 2) {
                    trace ("TXX frame is too short, skipped\n");
                    readptr += sz; // bad tag
                    continue;
                }
                int res = junk_id3v2_load_txx (version_major, it, readptr, synched_size);
            }
            readptr += sz;
        }
        else {
            trace ("id3v2.%d (unsupported!)\n", version_minor);
        }
    }
    if (!err && it) {
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
        if (performer) {
            pl_add_meta (it, "performer", performer);
            free (performer);
        }
        if (composer) {
            pl_add_meta (it, "composer", composer);
            free (composer);
        }
        if (track) {
            char *slash = strchr (track, '/');
            if (slash) {
                // split into track/number
                *slash = 0;
                slash++;
                pl_add_meta (it, "numtracks", slash);
            }
            pl_add_meta (it, "track", track);
            free (track);
        }
        if (title) {
            pl_add_meta (it, "title", title);
            free (title);
        }
        if (genre) {
            if (genre[0] == '(') {
                // find matching parenthesis
                char *p = &genre[1];
                while (*p && *p != ')') {
                    if (!isdigit (*p)) {
                        break;
                    }
                    p++;
                }
                if (*p == ')' && p[1] == 0) {
                    *p = 0;
                    memmove (genre, genre+1, p-genre);
                }
            }
            // check if it is numeric
            if (genre) {
                const char *p = genre;
                while (*p) {
                    if (!isdigit (*p)) {
                        break;
                    }
                    p++;
                }
                if (*p == 0 && p > genre) {
                    int genre_id = atoi (genre);
                    if (genre_id >= 0) {
                        const char *genre_str = NULL;
                        if (genre_id <= 147) {
                            genre_str = junk_genretbl[genre_id];
                        }
                        else if (genre_id == 0xff) {
                            genre_str = "None";
                        }
                        if (genre_str) {
                            free (genre);
                            genre = strdup (genre_str);
                        }
                    }
                }
                else if (!strcmp (genre, "CR")) {
                    free (genre);
                    genre = strdup ("Cover");
                }
                else if (!strcmp (genre, "RX")) {
                    free (genre);
                    genre = strdup ("Remix");
                }
            }

            pl_add_meta (it, "genre", genre);
            free (genre);
        }
        if (year) {
            pl_add_meta (it, "year", year);
            free (year);
        }
        if (copyright) {
            pl_add_meta (it, "copyright", copyright);
            free (copyright);
        }
        if (vendor) {
            pl_add_meta (it, "vendor", vendor);
            free (vendor);
        }
        if (comment) {
            pl_add_meta (it, "comment", comment);
            free (comment);
        }
        if (disc) {
            pl_add_meta (it, "disc", disc);
            free (disc);
        }

        if (version_major == 2) {
            pl_append_meta (it, "tags", "ID3v2.2");
        }
        else if (version_major == 3) {
            pl_append_meta (it, "tags", "ID3v2.3");
        }
        else if (version_major == 4) {
            pl_append_meta (it, "tags", "ID3v2.4");
        }

        if (!title) {
            pl_add_meta (it, "title", NULL);
        }
        return 0;
    }
    else if (err) {
        trace ("error parsing id3v2\n");
    }

    return 0;

error:
    if (tag) {
        free (tag);
    }
    if (tag_store) {
        while (tag_store->frames) {
            DB_id3v2_frame_t *next = tag_store->frames->next;
            free (tag_store->frames);
            tag_store->frames = next;
        }
    }
    return -1;
}

int
junk_id3v2_read (playItem_t *it, DB_FILE *fp) {
    return junk_id3v2_read_full (it, NULL, fp);
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
    iconv_t cd = iconv_open (UTF8, cs);
    if (cd == (iconv_t)-1) {
        trace ("iconv can't recode from %s to utf8\n", cs);
        memcpy (out, in, min(inlen, outlen));
        return;
    }
    else {
        size_t inbytesleft = inlen;
        size_t outbytesleft = outlen;
#ifdef __linux__
        char *pin = (char*)in;
#else
        const char *pin = in;
#endif
        char *pout = out;
        memset (out, 0, outlen);
        size_t res = iconv (cd, &pin, &inbytesleft, &pout, &outbytesleft);
        iconv_close (cd);
    }
}

