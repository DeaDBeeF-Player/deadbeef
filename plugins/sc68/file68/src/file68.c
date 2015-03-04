/*
 * @file    file68.c
 * @brief   load and save sc68 stream
 * @author  http://sourceforge.net/users/benjihan
 *
 * Copyright (c) 1998-2015 Benjamin Gerard
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.
 *
 * If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "file68_api.h"
#include "file68.h"
#include "file68_chk.h"
#include "file68_err.h"
#include "file68_msg.h"
#include "file68_str.h"
#include "file68_rsc.h"
#include "file68_opt.h"

#include "file68_vfs_def.h"
#include "file68_vfs.h"
/* #include "file68_vfs_file.h" */
/* #include "file68_vfs_fd.h" */
/* #include "file68_vfs_curl.h" */
/* #include "file68_vfs_mem.h" */
#include "file68_vfs_z.h"
/* #include "file68_vfs_null.h" */
#include "file68_ice.h"
#include "file68_zip.h"
#include "file68_uri.h"
#include "file68_tdb.h"

#ifndef u64
# ifdef HAVE_STDINT_H
#  include <stdint.h>
#  define u64 uint_least64_t
# elif defined(_MSC_VER)
#  define u64 unsigned __int64
# elif defined(__GNUC__)
#  define u64 unsigned long long
# endif
#endif

#ifndef u64
# error "u64 must be defined as an integer of at least 64 bit"
#endif

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <assert.h>

#define FOURCC(A,B,C,D) ((int)( ((A)<<24) | ((B)<<16) | ((C)<<8) | (D) ))
#define gzip_cc FOURCC('g','z','i','p')
#define ice_cc  FOURCC('i','c','e','!')
#define sndh_cc FOURCC('S','N','D','H')
#define sc68_cc FOURCC('S','C','6','8')

/* SC68 file identifier strings */
const char file68_idstr_v1[] = SC68_IDSTR;
const char file68_idstr_v2[] = SC68_IDSTR_V2;
const char file68_mimestr[]  = SC68_MIMETYPE;
/* maximum number of guessed loop allowed */
#define MAX_TRACK_LP 4
/* below this value, force a few loops */
#define MIN_TRACK_MS (45*1000u)

#ifndef DEBUG_FILE68_O
# define DEBUG_FILE68_O 0
#endif
int file68_cat = msg68_DEFAULT;

#define ISCHK(A,B) ( (A)[0] == (B)[0] && (A)[1] == (B)[1] )

/* Array of static strings, mainly used by metatags. */
static /* const */ struct strings_table {
  char n_a[4];
  char aka[4];
  char sc68[5];
  char sndh[5];
  char rate[5];
  char year[5];
  char album[6];
  char genre[6];
  char image[6];
  char title[6];
  char replay[7];
  char artist[7];
  char author[7];
  char ripper[7];
  char format[7];
  char comment[8];
  char composer[9];
  char converter[10];
  char copyright[10];
  char amiga_chiptune[15];
  char atari_st_chiptune[18];

  char _reserved;
} tagstr = {
  SC68_NOFILENAME,
  TAG68_AKA,
  "sc68",
  "sndh",
  TAG68_RATE,
  TAG68_YEAR,
  TAG68_ALBUM,
  TAG68_GENRE,
  TAG68_IMAGE,
  TAG68_TITLE,
  TAG68_REPLAY,
  TAG68_ARTIST,
  TAG68_AUTHOR,
  TAG68_RIPPER,
  TAG68_FORMAT,
  TAG68_COMMENT,
  TAG68_COMPOSER,
  TAG68_CONVERTER,
  TAG68_COPYRIGHT,
  "Amiga chiptune",
  "Atari-ST chiptune",
  -1,
};


/***********************************************************************
 * Check functions
 **********************************************************************/

/* valid year ? */
static int is_year(const char * const s) {
  int year = 0;
  if ( ( (*s=='1' && s[1]=='9') || (*s=='2' && s[1] =='0') ) &&
       isdigit((int)s[2]) && isdigit((int)s[3]) )
    year = (*s-'0')*1000 + (s[1]-'0')*100 + (s[2]-'0')*10 + (s[3]-'0');
  return year;
}

/* valid disk ? */
static inline int is_disk(const disk68_t * const mb) {
  return mb && mb->magic == SC68_DISK_ID;
}

/* null or valid disk ? */
static inline int null_or_disk(const disk68_t * const mb) {
  return !mb || mb->magic == SC68_DISK_ID;
}

/* Track value in range ? */
static inline int in_range(const disk68_t * mb, const int track) {
  return track > 0 && track <= mb->nb_mus;
}

/* Track in disk ? */
static inline int in_disk(const disk68_t * mb, const int track) {
  return is_disk(mb) && in_range(mb, track);
}

/* Does this memory belongs to the disk data ? */
static inline int is_disk_data(const disk68_t * const mb, const void * const _s)
{
  const char * const s = (const char *) _s;
  return is_disk(mb) && s >= mb->data && s < mb->data+mb->datasz;
}

/* Does this memory belongs to the static string array ? */
static inline int is_static_str(const char * const s)
{
  const char * const tagstr0 = (const char *) &tagstr;
  const char * const tagstr1 = tagstr0+sizeof(tagstr);
  return (s >= tagstr0 && s < tagstr1);
}

/* Free buffer unless it is allocated inside disk data or inside
 * static tag string array */
static void free_string(const disk68_t * const mb, void * const s)
{
  if (s && !is_static_str(s) && !is_disk_data(mb,s))
    free(s);
}

static inline void safe_free_string(const disk68_t * const mb, void * const _s)
{
  void ** s = _s;
  free_string(mb, *s); *s = 0;
}

static char *strdup_not_static(const disk68_t * const mb, const char * s)
{
  return (char *) ( (s && !is_static_str(s) && !is_disk_data(mb,s))
                    ? strdup68(s)
                    : s );
}


/* Peek Little Endian Unaligned 32 bit value */
static inline int LPeek(const void * const a)
{
  int r;
  const unsigned char * const c = (const unsigned char *) a;
  r = c[0] + (c[1] << 8) + (c[2] << 16) + ((int)(signed char)c[3] << 24);
  return r;
}

/* Peek big Endian Unaligned 16 bit value */
static inline int WPeekBE(const void * const a)
{
  int r;
  const unsigned char * const c = (const unsigned char *) a;
  r = c[1] + ((int)(signed char)c[0] << 8);
  return r;
}

/* Peek Big Endian Unaligned 32 bit value */
static inline int LPeekBE(const void * const a)
{
  int r;
  const unsigned char * const c = (const unsigned char *) a;
  r = ((int)(signed char)c[0] << 24) + (c[1] << 16) + (c[2] << 8) + c[3];
  return r;
}

/* Poke Little Endian Unaligned 32 bit value */
static inline void LPoke(void * const a, int r)
{
  unsigned char * const c = (unsigned char *) a;
  c[0] = r;
  c[1] = r >> 8;
  c[2] = r >> 16;
  c[3] = r >> 24;
}

static int myatoi(const char *s, int i, int max, int * pv)
{
  int v = 0;
  for (; i<max; ++i) {
    int c = s[i] & 255;
    if (c>='0' && c<='9') {
      v = v * 10 + c - '0';
    } else {
      break;
    }
  }
  if (pv) *pv = v;
  return i;
}

/* Match regexpr : [[:alpha:]][-_[:alnum:]]* */
static int is_valid_key(const char * key)
{
  int c;
  if (!key || (c = *key++, !isalpha(c)))
    return 0;
  do {
    if (!isalnum(c) || c == '-' || c == '_')
      return 0;
  } while (c = *key++, c);
  return 1;
}

/* Searching a key (key can be 0 to search an empty slot) */
static int get_customtag(const tagset68_t * tags, const char * key)
{
  int i;
  if (!strcmp68(key, tagstr.title))
    return TAG68_ID_TITLE;
  if (!strcmp68(key, tagstr.album))
    return TAG68_ID_ALBUM;
  if (!strcmp68(key, tagstr.artist))
    return TAG68_ID_ARTIST;
  if (!strcmp68(key, tagstr.author))
    return TAG68_ID_AUTHOR;
  if (!strcmp68(key, tagstr.genre))
    return TAG68_ID_GENRE;
  if (!strcmp68(key, tagstr.format))
    return TAG68_ID_FORMAT;
  for (i=TAG68_ID_CUSTOM; i<TAG68_ID_MAX; ++i) {
    if (!strcmp68(key, tags->array[i].key))
      return i;
  }
  return -1;
}

/* Set/Replace a custom key (val can be 0 to remove the tag). */
static int set_customtag(disk68_t * mb, tagset68_t * tags,
                         const char * key, const char * val)
{
  int i
    = get_customtag(tags, key);
  if (!val) {
    if (i >= 0) {
      safe_free_string(mb,&tags->array[i].val);
      if (i >= TAG68_ID_CUSTOM)
        safe_free_string(mb,&tags->array[i].key);
    }
  } else {
    if (i < 0)
      i = get_customtag(tags, 0);
    if (i >= 0) {
      safe_free_string(mb,&tags->array[i].val);
      if (!tags->array[i].key &&
          !(tags->array[i].key = strdup_not_static(mb, key)))
        i = -1;
      else if (!(tags->array[i].val = strdup_not_static(mb, val)))
        i = -1;
    }
  }
  return i;
}

#define ok_int(V)  strok68(V)
#define strnull(S) strnevernull68(S)

static int gzip_is_magic(const char * buffer)
{
  return 1
    && buffer[0] == 0x1f
    && buffer[1] == (char)0x8b
    && buffer[2] == 8;
}

static int ice_is_magic(const char * buffer)
{
  return 1
    && buffer[0] == 'I'
    && (buffer[1]|0x20) == 'c'
    && (buffer[2]|0x20) == 'e'
    && buffer[3] == '!';
}

struct sndh_boot { int init, kill, play, data; };

/* Decode 68k instruction at the beginning of sndh files.
 * @return  offset of the jump or org if it's a rts.
 * @retval -1 on error
 */
static int sndh_decode(const char * buf, int org, int off)
{
  if (off <= 10) {
    unsigned int w0, w1;
    w0 = WPeekBE(buf + off + 0);

    /* Skip nop */
    if (w0 == 0x4e71)
      return sndh_decode(buf,org,off+2);
    w1 = WPeekBE(buf + off + 2);

    /* Decode 68k instructions allowed instructions: jmp(pc)/bra/bra.s/rts
     */
    if (w0 == 0x6000 || w0 == 0x4efa)
      return off + 2 + w1;              /* bra and jmp(pc) */
    else if ( (w0 & 0xff00) == 0x6000 )
      return off + (int8_t) w0;         /* bra.s */
    else if ( w0 == 0x4e75 || (w0 == 0x4e00 && off == 4))
      return org;                       /* rts or illegal-fix */
  }
  return -1;
}

/* @retval 0   on error
 * @retval 12  nornal 'SNDH' tag position.
 */
static int sndh_is_magic(const char *buffer, int max, struct sndh_boot * sb)
{
  struct sndh_boot tmp, * boot = sb ? sb : &tmp;
  const int start = 10;
  int i=0, v = 0;

  boot->init = boot->kill = boot->play = -1;
  boot->data = 0x8000;

  if (max >= 12
      && (boot->init = sndh_decode(buffer,0,0)) >= 0
      && (boot->kill = sndh_decode(buffer,4,4)) >= 0
      && (boot->play = sndh_decode(buffer,8,8)) >= 0 ) {

    if (boot->init >= 16 && boot->init < boot->data)
      boot->data = boot->init;
    if (boot->kill >= 16 && boot->kill < boot->data)
      boot->data = boot->kill;
    if (boot->play >= 16 && boot->play < boot->data)
      boot->data = boot->play;
    if (boot->data == 0x1000)
      return 0;
    max = boot->data;

    for (i=start, v = LPeekBE(buffer+i); i < max && v != sndh_cc;
         v = ((v<<8) | (buffer[i++]&255)) & 0xFFFFFFFF)
      ;
  }
  i = (v == sndh_cc) ? i-4: 0;

  assert(i==0 || i == 12);

  return i;
}

static
int isread(vfs68_t * const is, void * data, int len, unsigned int * hptr)
{
  int read = vfs68_read(is, data, len);
  if (read > 0 && hptr) {
    uint32_t h = *hptr;
    int n = read;
    uint8_t * k = (uint8_t *) data;
    do {
      h += *k++;
      h += h << 10;
      h ^= h >> 6;
    } while (--n);
    *hptr = h;
  }
  return read;
}


/* Ensure enough data in id buffer;
 *  retval  0      on error
 *  retval  count  on success
 */
static inline
int ensure_header(vfs68_t * const is, char *id,
                  int have, int need, unsigned int * hptr)
{
  if (have < need) {
    int miss = need - have;
    int read = isread(is, id+have, miss, hptr);
    if (read != miss) {
      /* msg68_error("not a sc68 file (%s)\n", read==-1?"read error":"eof"); */
      have = 0;
    } else {
      have = need;
    }
  }
  return have;
}

/* Verify header; returns # bytes to alloc & read
 * or -1 if error
 * or -gzip_cc if may be gzipped
 * or -ice_cc if may be iced
 * or -sndh_cc if may be sndh
 */
static int read_header(vfs68_t * const is, unsigned int * hptr)
{
  char id[256];
  const int idv1_req = sizeof(file68_idstr_v1);
  const int idv2_req = sizeof(file68_idstr_v2);
  const int sndh_req = 32;
  int have = 0;
  const char * missing_id = "not a sc68 file (no magic)";

  assert( idv1_req == 56 );
  assert( idv2_req ==  8 );

  /* Read ID v2 string */
  if(have = ensure_header(is, id, have, idv2_req, hptr), !have) {
    return -1;
  }

  if (!memcmp(id, file68_idstr_v1, idv2_req)) {
    /* looks like idv1; need more bytes to confirm */
    if (have = ensure_header(is, id, have, idv1_req, hptr), !have) {
      return -1;
    }
    if (memcmp(id, file68_idstr_v1, idv1_req)) {
      return error68(missing_id);
    }
    TRACE68(file68_cat,"file68: found %s signature\n", "SC68_v1");
  } else if (!memcmp(id, file68_idstr_v2, idv2_req)) {
    TRACE68(file68_cat,"file68: found %s signature\n","SC68_v2");
  } else {
    if (have = ensure_header(is, id, have, sndh_req, hptr), !have) {
      return -1;
    }
    if (gzip_is_magic(id)) {
      TRACE68(file68_cat,"file68: found %s signature\n","GZIP");
      return -gzip_cc;
    } else if (ice_is_magic(id)) {
      TRACE68(file68_cat,"file68: found %s signature\n","ICE!");
      return -ice_cc;
    } else {
      /* Must be done after gzip or ice becausez id-string may appear
       * in compressed buffer too.
       */
      if (sndh_is_magic(id,sndh_req,0)) {
        TRACE68(file68_cat,"file68: found %s signature\n","SNDH");
        return -sndh_cc;
      }
    }
    return error68(missing_id);
  }

  /* Check 1st chunk */
  if (isread(is, id, 4, hptr) != 4
      || memcmp(id, CH68_CHUNK CH68_BASE, 4)) {
    return error68("file68: not sc68 file -- missing base chunk");
  }

  /* Get base chunk : file total size */
  if (isread(is, id, 4, hptr) != 4
      || (have = LPeek(id), have <= 8)) {
    return error68("file68: not sc68 file -- weird base chunk size");
  }
  /* TRACE68(file68_cat,"file68: header have %d bytes\n",have-8); */
  return have-8;
}

static const char * not_noname(const char * s)
{
  return (s && strcmp68(s,tagstr.n_a)) ? s : 0;
}

/* FR  = SEC * HZ
 * FR  = MS*HZ/1000
 *
 * SEC = FR / HZ
 * MS  = FR*1000/HZ
 */

static unsigned int fr_to_ms(unsigned int fr, unsigned int hz)
{
  u64 ms;
  ms  = fr;
  ms *= 1000u;
  ms /= hz;
  return (unsigned int) ms;
}

static unsigned int ms_to_fr(unsigned int ms, unsigned int hz)
{
  u64 fr;
  fr  = ms;
  fr *= hz;
  fr /= 1000u;
  return (unsigned int) fr;
}

/* match ".+ ([^)]+)$" */
static int has_parenthesis(char * name, char ** sptr, char ** eptr)
{
  int i, l = strlen(name);

  if (l < 5 || name[l-1] != ')')
    return 0;
  for (i = l-2; i > 1 && name[i] != '('; --i)
    if (name[i] == ')') return 0;
  if (i <= 1 || i == l-2 || name[i-1] != ' ')
    return 0;
  *sptr = name+i;
  *eptr = name+l-1;

  return 1;
}

/* A bit of cooking here
 *
 * For sc68 file: extract AKA from legacy sc68 artist nomenclature
 *                `ARTIST (AKA)' excepted for unknown (replay) ones.
 *
 * For sndh file: extract YEAR from title or artist.
 *
 * always returns AKA tag id or -1
 */
static int decode_artist(disk68_t * mb, tagset68_t * tags)
{
  char *s, *e;
  const int is_sc68 = mb->tags.tag.genre.val == tagstr.sc68;
  int id_aka
    = get_customtag(tags, tagstr.aka);

  if (is_sc68) {
    /* sc68 file format */
    if (id_aka >= 0)
      return id_aka;
    if ( !tags->tag.artist.val ||
         !has_parenthesis(tags->tag.artist.val, &s, &e) ||
         !strncmp68(tags->tag.artist.val,"unknown",7) )
      return id_aka;
    s[-1] = e[0] = 0;                   /* remove `(' and `)' */
    id_aka = set_customtag(mb, tags, tagstr.aka, s+1);
    TRACE68(file68_cat, "file68 guessed %s='%s'\n", tagstr.aka, s+1);
  } else {
    /* not sc68 file format */
    if (get_customtag(&mb->tags, tagstr.year) < 0 &&
         (
           (
             ( tags->tag.title.val &&
               has_parenthesis(tags->tag.title.val, &s, &e) &&
               is_year(s+1) >= 1990 )
             ||
             ( tags->tag.artist.val &&
               has_parenthesis(tags->tag.artist.val, &s, &e) &&
               is_year(s+1) >= 1990 )
             )
           )
      ) {
      s[-1] = e[0] = 0;                   /* remove `(' and `)' */
      set_customtag(mb, &mb->tags, tagstr.year, s+1);
      TRACE68(file68_cat, "file68 guessed %s='%s'\n",
              tagstr.year, s+1);

    }
  }
  return id_aka;
}

/* This function inits all pointers for this music files. It setup non
 * initialized data to defaut value. It verifies most values are in
 * good range.
 */
static int valid(disk68_t * mb)
{
  music68_t *m;
  int i, pdatasz = 0, has_infinite = 0;
  void * pdata   = 0;
  char * title, * artist, * aalias;
  int is_sndh = mb->tags.tag.genre.val == tagstr.sndh;

  if (mb->nb_mus <= 0)
    return error68("file68: disk has no track");

  /* Ensure default music in valid range */
  if (mb->def_mus < 0 || mb->def_mus >= mb->nb_mus)
    mb->def_mus = 0;

  /* default album is 'noname' */
  if (!mb->tags.tag.title.val)
    mb->tags.tag.title.val = tagstr.n_a;
  /* default title is album */
  title  = mb->tags.tag.title.val;
  /* default album artist is default track artist. Postpone ... */
  /* default artist is album artist or 'n/a' */
  artist = mb->tags.tag.artist.val ? mb->tags.tag.artist.val : tagstr.n_a;
  /* default alias */
  aalias = (i = decode_artist(mb, &mb->tags), i<0)
    ? 0 : mb->tags.array[i].val;

  /* Disk total time : 00:00 */
  mb->time_ms = 0;

  /* Clear flags */
  mb->hwflags.all = 0;

  /* Init all music in this file */
  for (m = mb->mus, i = 0; m < mb->mus + mb->nb_mus; m++, i++) {

    /* $$$ TEMP $$$ hack tao_tsd, to remove ym from hardware flags */
    if (!strcmp68(m->replay,"tao_tsd")) {
      m->hwflags.bit.ym     = 0;
      m->hwflags.bit.ste    = 1;
    }

    /* Default load address */
    if ( (m->has.pic = !m->a0) )
      m->a0 = SC68_LOADADDR;

    /* Default replay frequency is 50Hz */
    if (!m->frq)
      m->frq = 50u;

    /* Compute ms from frames prior to frames from ms. */
    if ( (m->has.time = m->first_fr > 0u) ) {
      m->first_ms = fr_to_ms(m->first_fr, m->frq);
    } else {
      if ( !(m->has.time = m->first_ms > 0) )
        /* m->first_ms = DEF_TRACK_MS */;
      m->first_fr = ms_to_fr(m->first_ms, m->frq);
    }

    if (is_sndh && !m->has.time) {
      unsigned int frames;
      int flags;

      /* TODO: check sndh time */
      msg68_notice("searching track #%02d:%08x in sndh timedb\n",
                   i+1, mb->hash);

      if (-1 != timedb68_get(mb->hash, i, &frames, &flags)) {
        m->has.time = 1;
        m->first_fr = frames;
        m->first_ms = fr_to_ms(m->first_fr, m->frq);

        m->hwflags.bit.ste    = !!(flags & TDB_STE);
        /* timers set means we have the timer information, not that
         * any timer is actually used ! Previous assumption (at least
         * 1 timer have to be unused) was wrong.
         */
        m->hwflags.bit.timers = 1;
        if (m->hwflags.bit.timers) {
          /* Invert the flag as timerdb knows about unused timers. */
          m->hwflags.bit.timera = ! (flags & TDB_TA);
          m->hwflags.bit.timerb = ! (flags & TDB_TB);
          m->hwflags.bit.timerc = ! (flags & TDB_TC);
          m->hwflags.bit.timerd = ! (flags & TDB_TD);
        }
        msg68_notice("found track #%02d:%08x in sndh timedb"
                     " -- %d ms, %d frames, %c%c%c%c%s\n",
                     i+1, mb->hash, m->first_ms, m->first_fr,
                     m->hwflags.bit.timera ? 'A' : '.',
                     m->hwflags.bit.timerb ? 'B' : '.',
                     m->hwflags.bit.timerc ? 'C' : '.',
                     m->hwflags.bit.timerd ? 'D' : '.',
                     m->hwflags.bit.ste ? ",STE" : "");
      }
    }

    if (m->has.loop) {
      /* Compute ms from frames. */
      m->loops_ms = fr_to_ms(m->loops_fr, m->frq);
    } else {
      /* loop time not set, default to track time. */
      m->loops_fr = m->first_fr;
      m->loops_ms = m->first_ms;
    }

    if (!m->loops_fr) {
      /* Track does not loop, force number of loops to 1. */
      m->loops = 1;
    } else if (!m->loops) {
      /* Number of loop unspeficied: calcultate it so that the total
       * time never be less than MIN_TRACK_MS, unless it requires more
       * loop than MAX_TRACK_LP.
       */
      m->loops = 1;
      if ( m->first_ms < MIN_TRACK_MS)
        m->loops += (MIN_TRACK_MS - m->first_ms + (m->loops_ms>>1)) / m->loops_ms;
      if (m->loops > MAX_TRACK_LP)
        m->loops = MAX_TRACK_LP;
    }

    if (m->loops > 0)
      mb->time_ms += fr_to_ms(m->first_fr+m->loops_fr*(m->loops-1), m->frq);
    else {
      m->loops = 0;
      has_infinite = 1;
    }

    /* default mode is YM2149 (Atari ST) */
    if (!m->hwflags.all) {
      m->hwflags.bit.ym = 1;
    }
    mb->hwflags.all |= m->hwflags.all;

    /* default genre */
    if (!m->tags.tag.genre.val)
      m->tags.tag.genre.val = (m->hwflags.bit.amiga)
        ? tagstr.amiga_chiptune
        : tagstr.atari_st_chiptune
        ;

    /* default music name is album name */
    if (!m->tags.tag.title.val)
      m->tags.tag.title.val = title;    /* inherits title  */
    else
      title = m->tags.tag.title.val;    /* new inheririted title */

    /* default artist */
    if (!m->tags.tag.artist.val) {
      m->tags.tag.artist.val = artist;  /* inherit artist */
      if (aalias)
        set_customtag(mb, &m->tags, tagstr.aka, aalias);
    } else {
      int id;
      artist = m->tags.tag.artist.val;  /* new inherited artist */
      aalias = (id = decode_artist(mb, &m->tags), id < 0)
        ? 0 : m->tags.array[id].val;
    }

    /* use data from previous music */
    if (!m->data) {
      m->data   = (char *) pdata;       /* inherit music data */
      m->datasz = pdatasz;
    }
    if (!m->data)
      return error68("file68: track #%d has no data", i+1);
    pdata   = m->data;                  /* new inherited music data */
    pdatasz = m->datasz;
  }

  /* album artist inherits default track artist */
  if (!mb->tags.tag.artist.val) {
    mb->tags.tag.artist.val = mb->mus[mb->def_mus].tags.tag.artist.val;
    if (i = get_customtag(&mb->mus[mb->def_mus].tags, tagstr.aka), i>=0)
      set_customtag(mb, &mb->tags, tagstr.aka,
                    mb->mus[mb->def_mus].tags.array[i].val);
  }

  if (has_infinite)
    mb->time_ms = 0;

  return 0;
}

int file68_is_our(const char * uri, const char * exts, int * is_remote)
{
  assert(!"do not use, needs a refresh");
  return 0;

#if 0
  const char * uri_end, *u;
  char protocol[16], *p;
  int has_protocol, remote, is_our;

  TRACE68(file68_cat,"file68: check uri --\n",uri);
  is_our = remote = 0;
  if (!uri || !*uri) {
    goto exit;
  }

  /* Default supported extensions */
  if (!exts) {
    exts = ".sc68\0.sndh\0.snd\0";
  }

  uri_end = uri + strlen(uri);
  has_protocol = !uri68_get_protocol(protocol, sizeof(protocol), uri);

  if (has_protocol) {
    is_our = !strcmp68(protocol,"SC68");
    if (!is_our && !strcmp68(protocol,"RSC68") && uri+14<uri_end) {
      is_our = strncmp(uri+8, "music/", 6);
    }

    if (is_our)  {
      /* $$$ Not really sure; may be remote or not. The only way to
         know is to check for the corresponding local file.
      */
      remote = 0;
      goto exit;
    }
  }

  /* Check remote for other protocol */
  remote = !uri68_local_protocol(protocol);

  /* Check extension ... */
  p = protocol+sizeof(protocol);
  *--p = 0;
  for (u=uri_end; u > uri && p > protocol; ) {
    int c = *--u & 255;
    if (c == '/') {
      break;
    }
    *--p = c;
    if (c == '.') {
      if (!strcmp68(p,".GZ")) {
        p = protocol+sizeof(protocol)-1;
      } else {
        break;
      }
    }
  }

  while (*exts) {
    is_our = !strcmp68(p,exts);
    if (is_our) {
      break;
    }
    exts += strlen(exts)+1;
  }

exit:
  if (is_remote) *is_remote = remote;
  TRACE68(file68_cat, "file68: check uri -- [%s]\n", ok_int(!is_our));
  return is_our;
#endif
}

static vfs68_t * uri_or_file_create(const char * uri, int mode,
                                    rsc68_info_t * info)
{
  vfs68_t *vfs = 0;

  TRACE68(file68_cat,"file68: create -- %s -- mode:%d -- with%s info\n",
          strnull(uri),mode,info?"":"out");

  if (info && !strncmp68(uri,"sc68://music/", 13)) {
    info->type = rsc68_last;
    vfs = uri68_vfs(uri, mode, 1, &info);
  } else {
    vfs = uri68_vfs(uri, mode, 0);
  }

  if (vfs68_open(vfs) < 0) {
    vfs68_destroy(vfs);
    vfs = 0;
  }

  TRACE68(file68_cat,"file68: create -- [%s,%s]\n",
          ok_int(!vfs),
          strnull(vfs68_filename(vfs)));
  return vfs;
}

disk68_t * file68_load_uri(const char * fname)
{
  disk68_t    * d;
  vfs68_t * is;
  rsc68_info_t  info;

  TRACE68(file68_cat,"file68: load -- %s\n", strnull(fname));

  is = uri_or_file_create(fname, 1, &info);
  d = file68_load(is);
  vfs68_destroy(is);

  if (d && info.type == rsc68_music) {
    // int i;

    TRACE68(file68_cat,
            "file68: load -- on the fly patch -- #%d/%d/%d\n",
            info.data.music.track,
            info.data.music.loops,
            info.data.music.time_ms);

    d->force_track   = info.data.music.track;
    d->force_loops   = info.data.music.loops;
    d->force_ms      = info.data.music.time_ms;
  }
  TRACE68(file68_cat,"file68: load -- [%s]\n", ok_int(!d));
  return d;
}

disk68_t * file68_load_mem(const void * buffer, int len)
{
  disk68_t * d;
  vfs68_t * is;

  is = uri68_vfs("mem:", 1, 2, buffer, len);
  d = vfs68_open(is) ? 0 : file68_load(is);
  vfs68_destroy(is);

  return d;
}


static int st_isupper( int c )
{
  return (c >= 'A' && c <= 'Z');
}

static int st_isdigit( int c )
{
  return c >= '0' && c <= '9';
}

static int st_istag( int c )
{
  return c == '!' || c == '#' || c == '*';
}

static int st_isgraph( int c )
{
  return st_isupper(c) || st_isdigit(c) || st_istag(c);
}

/* @see http://sndh.atari.org/fileformat.php
 */
static int sndh_info(disk68_t * mb, int len)
{
  const int unknowns_max = 8;
  int i, vbl = 0, frq = 0/* , steonly = 0 */,
    unknowns = 0, fail = 0;
  int dtag = 0, ttag = 0;
  char * b = mb->data;
  char empty_tag[4] = { 0, 0, 0, 0 };
  struct sndh_boot boot;

  /* Default */
  mb->mus[0].data   = b;
  mb->mus[0].datasz = len;

#if 1
  mb->nb_mus = -1; /* Make validate failed */
#else
  mb->nb_mus = 1; /* Assume default of 1 track */
#endif

  mb->mus[0].replay = 0;

  i = sndh_is_magic(b, len, &boot);
  if (!i) {
    /* should not happen since we already have tested it. */
    msg68_critical("file68: sndh -- info mising magic!\n");
    return -1;
  }
  if (boot.data >= len) {
    msg68_error("file68: sndh -- unable to locate music data\n");
    return -1;
  }
  len = boot.data;


  TRACE68(file68_cat,
          "file68: sndh -- init:%04x kill:%04x play:%04x data:%04x\n",
          boot.init, boot.kill, boot.play, boot.data);

  /* HAXXX:
     Some music have 0 after values. I don't know what are
     sndh rules. May be 0 must be skipped or may be tag must be word
     aligned.
     Anyway the current parser allows a given number of successive
     unknown tags. May be this number should be increase in order to prevent
     some "large" unknown tag to break the parser.
  */

  while (i+4 < len) {
    char ** p;
    int j, t, s, ctypes;

    /* check char types for the next 4 chars */
    for (ctypes = 0, j=0; j<4; ++j) {
      ctypes |= (st_isgraph( b[i+j] ) << j );
      ctypes |= (st_isdigit( b[i+j] ) << (j + 8) );
    }

    TRACE68(file68_cat,
            "file68: sndh -- pos:%d/%d ctypes:%04X -- '%c%c%c%c'\n",
            i, len, ctypes, b[i+0], b[i+1], b[i+2], b[i+3]);

    t       = -1;                       /* offset on tag */
    s       = -1;                       /* offset on arg */
    p       = 0;                        /* store arg     */

    if (  (ctypes & 0x000F) != 0x000F ) {
      /* Not graphical ... should not be a valid tag */
    } else if (!memcmp(b+i,"SNDH",4)) {
      /* Header */
      t = i; i += 4;
    } else if (!memcmp(b+i,"COMM",4)) {
      /* Artist */
      t = i; s = i += 4;
      p = &mb->mus[0].tags.tag.artist.val;
    } else if (!memcmp(b+i,"TITL",4)) { /* title    */
      /* Title */
      t = i; s = i += 4;
      p = &mb->tags.tag.title.val;
    } else if (!memcmp(b+i,"RIPP",4)) {
      /* Ripper */
      if (ttag < TAG68_ID_CUSTOM_MAX) {
        t = i; s = i += 4;
        mb->mus[0].tags.tag.custom[ttag].key = tagstr.ripper;
        p = &mb->mus[0].tags.tag.custom[ttag++].val;
      }
    } else if (!memcmp(b+i,"CONV",4)) {
      /* Converter */
      if (ttag < TAG68_ID_CUSTOM_MAX) {
        t = i; s = i += 4;
        mb->mus[0].tags.tag.custom[ttag].key = tagstr.converter;
        p = &mb->mus[0].tags.tag.custom[ttag++].val;
      }
    } else if (!memcmp(b+i, "YEAR", 4)) {
      /* year */
      if (dtag < TAG68_ID_CUSTOM_MAX) {
        t = i; s = i += 4;
        mb->tags.tag.custom[dtag].key = tagstr.year;
        p = &mb->tags.tag.custom[dtag++].val;
      }
#if 0 /* Does not happen probably just a typo in the sndh format doc. */
    } else if ( (ctypes & 0x0F00) == 0x0F00 && is_year(b+i) && !b[4] ) {
      assert(0);
      /* match direct a YEAR */
      if (dtag < TAG68_ID_CUSTOM_MAX) {
        t = i; s = i += 4;
        mb->tags.tag.custom[dtag].key = tagstr.year;
        mb->tags.tag.custom[dtag++].val = b;
      }
#endif
    } else if (!memcmp(b+i,"MuMo",4)) {
      /* MusicMon ???  */
      msg68_warning("file68: sndh -- %s\n","what to do with 'MuMo' tag ?");
      /* musicmon = 1; */
      t = i; i += 4;
    } else if (!memcmp(b+i,"TIME",4)) {
      /* Time in second */
      int j, tracks = mb->nb_mus <= 0 ? 1 : mb->nb_mus;
      t = i; i += 4;
      for ( j = 0; j < tracks; ++j ) {
        if (i < len-2 && j < SC68_MAX_TRACK)
          mb->mus[j].first_ms = 1000u *
            ( ( ( (unsigned char) b[i]) << 8 ) | (unsigned char) b[i+1] );
        TRACE68(file68_cat,
                "file68: sndh -- TIME #%02d -- 0x%02X%02X (%c%c) -- %u ms\n",
                j+1, (unsigned char)b[i], (unsigned char)b[i+1],
                isgraph((int)(unsigned char)b[i])?b[i]:'.',
                isgraph((int)(unsigned char)b[i+1])?b[i+1]:'.',
                mb->mus[j].first_ms);
        i += 2;
      }
    } else if (!memcmp(b+i, "FLAG", 4)) {
      /* Track features (hardware,fx...) */
      int j, max=0, tracks = mb->nb_mus <= 0 ? 1 : mb->nb_mus;
      t = i;
      for ( j = 0; j < tracks; ++j ) {
        music68_t * m = mb->mus+j;
        int k, off = WPeekBE(b + i + 4 + j*2);
        m->hwflags.bit.timers = 1;
        TRACE68(file68_cat,
                "file68: sndh -- FLAG #%02d -- %s\n", j+1, b+i+off);
        /* parse the flad */
        for (k=i+off; k<len && b[k]; ++k)
          switch (b[k]) {
          case 'y': m->hwflags.bit.ym     = 1; break;
          case 'e': m->hwflags.bit.ste    = 1; break;
          case 'a': m->hwflags.bit.timera = 1; break;
          case 'b': m->hwflags.bit.timerb = 1; break;
          case 'c': m->hwflags.bit.timerc = 1; break;
          case 'd': m->hwflags.bit.timerd = 1; break;
          case 'p': m->hwflags.bit.amiga  = 1; break;
          }
        if (k > max)
          max = k;
      }
      i = max+1;

    } else if ( !memcmp(b+i,"##",2) && ( (ctypes & 0xC00) == 0xC00 ) ) {
      mb->nb_mus = ( b[i+2] - '0' ) * 10 + ( b[i+3] - '0' );
      /* assert(0); */
      t = i; i += 4;
    } else if (!memcmp(b+i,"!#SN",4)) {
      /* track names */
      int j, max=0, tracks = mb->nb_mus <= 0 ? 1 : mb->nb_mus;

      int faulty = 0;

      t = i;
      /* assert(0); */


      /* FIXME: normally (according to the documentation) the subname
       * table are offset relative the tag itself. However in some
       * files they are obviously relative to the end of the offset
       * table. Those are most probably faulty files, but I'll keep
       * loading them by guessing the offset is wrong since it points
       * in middle of the offset table instead of an actual string. */
      for (j = 0; j < tracks; ++j) {
        int off = WPeekBE(b + i + 4 + j*2); /* string offset */
        if (off < 4+tracks*2) {
          /* assert(!"faulty !#SN"); */
          faulty = 4+tracks*2;
          break;
        }
      }

      for (j = 0; j < tracks; ++j) {
        int off = faulty + WPeekBE(b + i + 4 + j*2); /* string offset */
        if (off > max) max = off;
        mb->mus[j].tags.tag.title.val = b + i + off;
        TRACE68(file68_cat,
                "file68: sndh -- !#SN #%02d pos:%d -- '%s'\n",
                j+1, i+off, mb->mus[j].tags.tag.title.val);
      }
      /* Position on the last sub name and skip it. */
      for (i += max; i < len && b[i] ; i++ )
        ;
      for (; i < len && !b[i] ; i++ )
        ;
    } else if ( !memcmp(b+i,"!#",2) && ( (ctypes & 0xC00) == 0xC00 ) ) {
      mb->def_mus = ( b[i+2] - '0' ) * 10 + ( b[i+3] - '0' ) - 1;
      t = i; i += 4;
    } else if ( !memcmp(b+i,"!V",2) && ( (ctypes & 0xC00) == 0xC00 ) ) {
      vbl = ( b[i+2] - '0' ) * 10 + ( b[i+3] - '0' );
      i += 4;
    } else if (!memcmp(b+i,"**",2)) {
      /* FX + string 2 char ??? */
      msg68_warning("file68: sndh -- what to do with tag ? -- '**%c%c'\n",
                    b[i+2], b[i+3]);
      i += 4;
    } else if ( b[i] == 'T' && b[i+1] >= 'A' && b[i+1] <= 'D') {
      t = i; s = i += 2;
      myatoi(b, i, len, &frq);
    } else if( memcmp( b + i, empty_tag, 4 ) == 0 ||
               memcmp( b + i, "HDNS", 4 ) == 0 ) {
      t = i;
      i = len;
    } else {
      /* skip until next 0 byte, as long as it's inside the tag area */
      i += 4;
      while( *(b + i) != 0 && i < len ) {
        i++;
      }
    }

    if ( t < 0 ) {
      /* Unkwown tag, finish here. */
      ++unknowns;
      TRACE68(file68_cat,
              "file68: sndh -- not a tag at offset %d -- %02X%02X%02X%02X\n",
              i, b[i]&255, b[i+1]&255, b[i+2]&255, b[i+3]&255);
      ++i;

      if (fail || unknowns >= unknowns_max) {
        i = len;
      }

    } else {
      /* Well known tag */

      TRACE68(file68_cat,
              "file68: sndh -- got TAG -- '%c%c%c%c'\n",
              b[t], b[t+1], b[t+2], b[t+3]);
      unknowns = 0; /* Reset successive unkwown. */

      if (s >= 0) {
        int j, k;
        for ( j = s, k = s - 1; j < len && b[j]; ++j) {
          if ( b[j] < 32 ) b[j] = 32;
          else k = j;                   /* k is last non space char */
        }

        if (k+1 < len) {
          b[k+1] = 0;                   /* Strip trailing space */
          i = k+2;
          if (p)
            *p = b+s;                   /* store tag */
          else
            TRACE68(file68_cat,"file68: sndh -- not storing -- '%s'\n",
                    b+s);

          /* HAXXX: using name can help determine STE needs */
          /* if (p == &mb->tags.tag.title.val) */
          /*   steonly = 0 */
          /*     || !!strstr(mb->tags.tag.title.val,"STE only") */
          /*     || !!strstr(mb->tags.tag.title.val,"(STe)") */
          /*     || !!strstr(mb->tags.tag.title.val,"(STE)") */
          /*     ; */

          TRACE68(file68_cat,
                  "file68: sndh -- got ARG -- '%s'\n",
                  b+s);

        }

        /* skip the trailing null chars */
        for ( ; i < len && !b[i] ; i++ )
          ;
      }

    }
  }

  if (mb->nb_mus <= 0) {
    TRACE68(file68_cat,
            "file68: sndh -- %d track; assuming 1 track\n", mb->nb_mus);
    mb->nb_mus = 1;
  }

  if (mb->nb_mus > SC68_MAX_TRACK) {
    mb->nb_mus = SC68_MAX_TRACK;
  }

  for (i=0; i<mb->nb_mus; ++i) {
    mb->mus[i].d0    = i+1;
    mb->mus[i].loops = 0;
    mb->mus[i].frq   = frq ? frq : vbl;
    if (!mb->mus[i].hwflags.bit.timers) {
      /* Did not have the 'FLAG' tag, fallback to YM+STE */
      mb->mus[i].hwflags.bit.ym  = 1;
      mb->mus[i].hwflags.bit.ste = 1;
    }
  }
  return 0;
}

int file68_tag_count(const disk68_t * mb, int track)
{
  int cnt = -1;

  if (mb && track >= 0 && track <= mb->nb_mus) {
    int idx;
    tagset68_t * tags = (tagset68_t *)
      (!track ? &mb->tags : &mb->mus[track-1].tags); /* /!\ discard const */
    for (idx = cnt = TAG68_ID_CUSTOM; idx<TAG68_ID_MAX; ++idx)
      if (tags->array[idx].key && tags->array[idx].val) {
        if (cnt != idx) {
          tags->array[cnt].key = tags->array[idx].key;
          tags->array[cnt].val = tags->array[idx].val;
        }
        ++cnt;
      }
  }
  return cnt;
}

int file68_tag_enum(const disk68_t * mb, int track, int idx,
                    const char ** key, const char ** val)
{
  const char * k = 0, * v = 0;

  if (mb && idx >= 0 && idx < TAG68_ID_MAX) {
    const tagset68_t * tags = 0;
    if (!track)
      tags = &mb->tags;
    else if (track > 0 && track <= mb->nb_mus)
      tags = &mb->mus[track-1].tags;
    if (tags) {
      k = tags->array[idx].key;
      v = tags->array[idx].val;
    }
  }
  if (key) *key = k;
  if (val) *val = v;

  return -!(k && v);
}

static const char * get_tag(const disk68_t * mb, int track, const char * key)
{
  const char * val = 0;
  const tagset68_t * tags;

  assert(is_disk(mb));
  assert(!track || in_range(mb,track));
  assert(key);

  if (!track)
    tags = &mb->tags;
  else if (in_range(mb, track))
    tags = &mb->mus[track-1].tags;
  else
    tags = 0;

  if (tags) {
    int i = get_customtag(tags, key);
    if (i >= 0)
      val = tags->array[i].val;
  }
  return val;
}


const char * file68_tag_get(const disk68_t * mb, int track, const char * key)
{
  return (key && is_disk(mb) && (!track || in_range(mb,track)))
    ? get_tag(mb, track, key)
    : 0
    ;
}

char * file68_tag(const disk68_t * mb, int track, const char * key)
{
  const char * val = 0;

  if (key && is_disk(mb) && (!track || in_range(mb,track))) {
    val = get_tag(mb, track, key);
    if (!val) {
      /* $$$ TODO */
    }
  }

  return strdup68(val);
}

const char * file68_tag_set(disk68_t * mb, int track,
                            const char * key, const char * val)
{
  const char * ret = 0;

  if (mb && is_valid_key(key)) {
    tagset68_t * tags = 0;

    if (!track)
      tags = &mb->tags;
    else if (track <= mb->nb_mus)
      tags = &mb->mus[track-1].tags;
    if (tags) {
      int i = set_customtag(mb, tags, key, val);
      if (i >= 0)
        ret = tags->array[i].val;
      else
        ret = 0;
    }
  }

  return ret;
}

static void free_tags(disk68_t * mb, tagset68_t *tags)
{
  int i;
  for (i=0; i<TAG68_ID_MAX; ++i) {
    free_string(mb, tags->array[i].key); tags->array[i].key = 0;
    free_string(mb, tags->array[i].val); tags->array[i].val = 0;
  }
}

void file68_free(const disk68_t * const_disk)
{
  disk68_t * disk = (disk68_t *)const_disk;

  if (is_disk(disk)) {
    const int max = disk->nb_mus;
    int i;

    free_tags(disk, &disk->tags);

    for (i=0; i<max; ++i) {
      free_string(disk, disk->mus[i].replay);
      free_tags(disk, &disk->mus[i].tags);
      if (disk->mus[i].data) {
        int j;
        free_string(disk, disk->mus[i].data);
        for (j=max-1; j>=i; --j) {
          if (disk->mus[j].replay == disk->mus[i].replay)
            disk->mus[j].replay = 0;
          if ( disk->mus[j].data == disk->mus[i].data )
            disk->mus[j].data   = 0;
          disk->mus[j].datasz = 0;
        }
        disk->mus[i].data = 0;
        disk->mus[i].datasz = 0;
      }
    }
    if (disk->data != disk->buffer) {
      free(disk->data);
      disk->data = 0;
    }
    free(disk);
  }
}

/* Allocate disk buffer */
static disk68_t * alloc_disk(int datasz)
{
  disk68_t * mb;
  int        room = datasz + sizeof(disk68_t);

  if (mb = calloc(room,1), mb) {
    music68_t *cursix;

    /* Set a little bit of magic */
    mb->magic = SC68_DISK_ID;

    /* data points into buffer */
    mb->data = mb->buffer;
    mb->datasz = datasz;

    /* Setup static tags */
    mb->tags.tag.title.key  = tagstr.title;
    mb->tags.tag.artist.key = tagstr.artist;
    mb->tags.tag.genre.key  = tagstr.format;
    for (cursix = mb->mus; cursix < mb->mus+SC68_MAX_TRACK; ++cursix) {
      cursix->tags.tag.title.key  = tagstr.title;
      cursix->tags.tag.artist.key = tagstr.artist;
      cursix->tags.tag.genre.key  = tagstr.genre;
    }
  }
  return mb;
}

disk68_t * file68_new(int extra)
{
  disk68_t * d = 0;
  if (extra < 0 || extra >= 1<<21)
    msg68_error("file68: invalid amount of extra data -- %d\n", extra);
  else
    d = alloc_disk(extra);
  return d;
}


/* Load , allocate memory and valid struct for SC68 music
 */
disk68_t * file68_load(vfs68_t * is)
{
  disk68_t *mb = 0;
  int len;
  unsigned int hash = 0, *h = &hash;
  int chk_size;
  int opened = 0;
  music68_t *cursix;
  tagset68_t * tags;
  char *b;
  const char *fname = vfs68_filename(is);
  const char *errorstr = 0;

  fname = strnevernull68(fname);

  /* Read header and get data length. */
  if (len = read_header(is, h), len < 0) {
    /* Verify tells it is a gzip or unice file, so we may give it a try.
     */
    if (1) {
      void * buffer = 0;
      int l;
      switch (len) {
      case -gzip_cc:
        /* gzipped */
        if (vfs68_seek_to(is,0) == 0) {
          vfs68_t * zis;
          zis=vfs68_z_create(is,VFS68_OPEN_READ,
                                 vfs68_z_default_option);
          if (!vfs68_open(zis)) {
            mb = file68_load(zis);
          }
          vfs68_destroy(zis);
          if (mb) {
            goto already_valid;
          }
        }
        break;

      case -ice_cc:
        if (vfs68_seek_to(is,0) == 0) {
          buffer = file68_ice_load(is, &l);
        }
        break;

      case -sndh_cc:
        if (vfs68_seek_to(is,0) != 0) {
          break;
        }
        len = vfs68_length(is);
        if (len <= 32 || len > 1<<21) {
          break;
        }
        mb = alloc_disk(len);
        if (!mb) {
          errorstr = "memory allocation";
          break;
        }
        mb->tags.tag.genre.val = tagstr.sndh;
        if (isread(is, mb->data, len, h) != len) {
          break;
        }
        if (sndh_info(mb, len)) {
          break;
        }
        goto validate;
      }

      if (buffer) {
        mb = file68_load_mem(buffer, l);
        free(buffer);
        if (mb) {
          return mb;
        }
      }
    }
    if (!errorstr)
      errorstr = "read header";
    goto error;
  }

  mb = alloc_disk(len);
  if (!mb) {
    errorstr = "memory allocation";
    goto error;
  }
  mb->tags.tag.genre.val = tagstr.sc68;

  if (isread(is, mb->data, len, h) != len) {
    errorstr = "read data";
    goto error;
  }

  for (b = mb->data, cursix = 0, tags = &mb->tags;
       len >= 8;
       b += chk_size, len -= chk_size) {
    char chk[8];
    if (b[0] != 'S' || b[1] != 'C') {
      break;
    }

    chk[0] = b[2];
    chk[1] = b[3];
    chk[2] = 0;
    chk_size = LPeek(b + 4);
    b += 8;
    len -= 8;

    if (ISCHK(chk, CH68_BASE)) {
      /* nothing to do. */
    }
    /* Default track */
    else if (ISCHK(chk, CH68_DEFAULT)) {
      mb->def_mus = LPeek(b);
    }
    /* Album or track title */
    else if (ISCHK(chk, CH68_FNAME) || ISCHK(chk, CH68_MNAME)) {
      tags->tag.title.val = b;
    }
    /* Start music session. */
    else if (ISCHK(chk, CH68_MUSIC)) {
      if (mb->nb_mus == SC68_MAX_TRACK) {
        /* Can't have more than SC68_MAX_TRACK tracks */
        len = 0;
        break;
      }
      cursix = mb->mus + mb->nb_mus;
      /* cursix->loops    = 0;             /\* default loop        *\/ */
      /* cursix->loops_fr = ~0;            /\* loop length not set *\/ */
      tags = &cursix->tags;
      mb->nb_mus++;
    }
    /* Author name */
    else if (ISCHK(chk, CH68_ANAME)) {
      tags->tag.artist.val = b;
    }
    /* Composer name */
    else if (ISCHK(chk, CH68_CNAME)) {
      if (strcmp68(b,tags->tag.artist.val))
        set_customtag(mb, tags, tagstr.composer, b);
    }
    /* External replay */
    else if (ISCHK(chk, CH68_REPLAY)) {
      if (!cursix) {
        errorstr = chk;
        goto error;
      }
      cursix->replay = b;
    }
    /* 68000 D0 init value */
    else if (ISCHK(chk, CH68_D0)) {
      if (!cursix) {
        errorstr = chk;
        goto error;
      }
      cursix->d0 = LPeek(b);
    }
    /* 68000 memory load address */
    else if (ISCHK(chk, CH68_AT)) {
      if (!cursix) {
        errorstr = chk;
        goto error;
      }
      cursix->a0 = LPeek(b);
    }
    /* Playing time (ms) */
    else if (ISCHK(chk, CH68_TIME)) {
      int sec;
      if (!cursix) {
        errorstr = chk;
        goto error;
      }
      sec = LPeek(b);
      /* sanity check */
      if (sec < 0 || sec > 60*60*24)
        sec = 0;
      cursix->first_ms = sec * 1000u;
    }
    /* Playing time (frames) */
    else if (ISCHK(chk, CH68_FRAME)) {
      if (!cursix) {
        errorstr = chk;
        goto error;
      }
      cursix->first_fr = LPeek(b);
      /* $$$ Workaround some buggy musics  */
      if (cursix->first_fr >= 0x1000000u)
        cursix->first_fr = 0;
    }
    /* Replay frequency */
    else if (ISCHK(chk, CH68_FRQ)) {
      if (!cursix) {
        errorstr = chk;
        goto error;
      }
      cursix->frq = LPeek(b);
    }
    /* Loop */
    else if (ISCHK(chk, CH68_LOOP)) {
      if (!cursix) {
        errorstr = chk;
        goto error;
      }
      cursix->loops = LPeek(b);
      /* force sanity */
      if (cursix->loops < -1)
        cursix->loops = 0;
    }
    /* Loop length */
    else if (ISCHK(chk, CH68_LOOPFR)) {
      if (!cursix) {
        errorstr = chk;
        goto error;
      }
      cursix->loops_fr = LPeek(b);
      cursix->has.loop = 1;
    }
    /* SFX flag */
    else if (ISCHK(chk, CH68_SFX)) {
      if (!cursix) {
        errorstr = chk;
        goto error;
      }
      cursix->has.sfx = 1;
    }
    /* Replay flags */
    else if (ISCHK(chk, CH68_TYP)) {
      int f;
      if (!cursix) {
        errorstr = chk;
        goto error;
      }
      f = LPeek(b);
      cursix->hwflags.all = 0;
      cursix->hwflags.bit.ym        = !! (f & SC68_YM);
      cursix->hwflags.bit.ste       = !! (f & SC68_STE);
      cursix->hwflags.bit.amiga     = !! (f & SC68_AMIGA);
      cursix->hwflags.bit.stechoice = !! (f & SC68_STECHOICE);
      cursix->hwflags.bit.timers    = !! (f & SC68_TIMERS);
      cursix->hwflags.bit.timera    = !! (f & SC68_TIMERA);
      cursix->hwflags.bit.timerb    = !! (f & SC68_TIMERB);
      cursix->hwflags.bit.timerc    = !! (f & SC68_TIMERC);
      cursix->hwflags.bit.timerd    = !! (f & SC68_TIMERD);
    }
    /* meta data */
    else if (ISCHK(chk, CH68_TAG)) {
      const char * key, * val;
      key = b;
      val = b + strlen(b) + 1;
      TRACE68(file68_cat,"file68: got a tag '%s' '%s'\n", key, val);
      if (set_customtag(mb, tags, key, val) < 0) {
        msg68_warning("file68: unable to set %s tag '%s' '%s'\n",
                      cursix ? "track" : "disk", key, val);
      }
    }
    /* music data */
    else if (ISCHK(chk, CH68_MDATA)) {
      if (!cursix) {
        errorstr = chk;
        goto error;
      }
      cursix->data = b;
      cursix->datasz = chk_size;
    }
    /* EOF */
    else if (ISCHK(chk, CH68_EOF)) {
      len = 0;
      break;
    }
  }

  /* Check it */
  if (len) {
    errorstr = "prematured end of file";
    goto error;
  }

validate:
  mb->hash = hash;
  if (valid(mb)) {
    errorstr = "validation test";
    goto error;
  }

already_valid:
  if (opened) {
    vfs68_close(is);
  }

  return mb;

error:
  if (opened) {
    vfs68_close(is);
  }
  free(mb);
  msg68_error("file68: load '%s' failed [%s]\n",
              fname, errorstr ? errorstr : "no reason");
  return 0;
}



static int get_version(const int version) {
  return version == 2 ? 2 : 1;
}

static void get_header(const int version,
                       const char ** const header, int * const headsz)
{
  switch (get_version(version)) {
  case 1:
    *header = file68_idstr_v1;
    *headsz = sizeof(file68_idstr_v1);
    break;
  case 2:
    *header = file68_idstr_v2;
    *headsz = sizeof(file68_idstr_v2);
    break;
  }
}

#ifndef _FILE68_NO_SAVE_FUNCTION_

/* save CHUNK and data */
/* $$$ NEW: Add auto 16-bit alignement. */
static int save_chunk(vfs68_t * os,
                      const char * chunk, const void * data, int size)
{
  static char zero[4] = {0,0,0,0};
  chunk68_t chk;
  int align;

  memcpy(chk.id, CH68_CHUNK, 2);
  memcpy(chk.id + 2, chunk, 2);
  align = size & 1;
  LPoke(chk.size, size + align);
  if (vfs68_write(os, &chk, (int)sizeof(chunk68_t)) != sizeof(chunk68_t)) {
    goto error;
  }
  /* Special case data is 0 should happen only for SC68 total size
   * chunk.
   */
  if (size && data) {
    if (vfs68_write(os, data, size) != size) {
      goto error;
    }
    if (align && vfs68_write(os, zero, align) != align) {
      goto error;
    }
  }
  return 0;

error:
  return -1;
}

/* save CHUNK and string (only if non-0 & lenght>0) */
static int save_string(vfs68_t * os,
                       const char * chunk, const char * str)
{
  int len;

  if (!str || !(len = strlen(str))) {
    return 0;
  }
  return save_chunk(os, chunk, str, len + 1);
}

/* save CHUNK and string (only if non-0 & lenght>0) */
static int save_noname(vfs68_t * os,
                       const char * chunk, const char * str)
{
  return save_string(os, chunk, not_noname(str));
}

/* save CHUNK & string str ( only if oldstr!=str & lenght>0 ) */
static int save_differstr(vfs68_t * os,
                          const char *chunk, char *str, char *oldstr)
{
  int len;

  if (oldstr == str
      || !str
      || (oldstr && !strcmp(oldstr, str))) {
    return 0;
  }
  len = strlen(str);
  return !len ? 0 :save_chunk(os, chunk, str, len + 1);
}

/* save CHUNK and 4 bytes Big Endian integer */
static int save_number(vfs68_t * os, const char * chunk, int n)
{
  char number[4];

  LPoke(number, n);
  return save_chunk(os, chunk, number, 4);
}

/* save CHUNK and number (only if n!=0) */
static int save_nonzero(vfs68_t * os, const char * chunk, int n)
{
  return !n ? 0 : save_number(os, chunk, n);
}

int file68_save_uri(const char * fname, const disk68_t * mb,
                    int version, int gzip)
{
  vfs68_t * os;
  int err;

  os = uri_or_file_create(fname, 2, 0);
  err = file68_save(os, mb, version, gzip);
  vfs68_destroy(os);

  return err;
}

static int save_tags(vfs68_t *os, const tagset68_t * tags,
                     int start, const char ** skip)
{
  int i, max = 0, err = 0;
  char * tmp = 0;

  for (i=start; i<TAG68_ID_MAX; ++i) {
    int keylen, vallen;

    /* Skip those tags. */
    if (skip) {
      const char ** skip_tag;
      for (skip_tag = skip;
           *skip_tag && strcmp68(*skip_tag, tags->array[i].key); skip_tag++)
        ;
      if (*skip_tag)
        continue;
    }

    if (tags->array[i].key && (keylen = strlen(tags->array[i].key)) &&
        tags->array[i].val && (vallen = strlen(tags->array[i].val))) {
      int len = keylen + vallen + 2;
      if (len > max) {
        char * new = realloc(tmp, len);
        if (!new) continue;
        tmp = new;
        max = len;
      }
      memcpy(tmp         ,tags->array[i].key, keylen+1);
      memcpy(tmp+keylen+1,tags->array[i].val, vallen+1);
      if (err = save_chunk(os, CH68_TAG, tmp, len), err)
        break;
    }
  }
  free(tmp);
  return err;
}

int file68_save_mem(const char * buffer, int len, const disk68_t * mb,
                    int version, int gzip)
{
  vfs68_t * vfs;
  int err;

  vfs = uri68_vfs("mem:", 2, 2, buffer, len);
  err = file68_save(vfs, mb, version, gzip);
  vfs68_destroy(vfs);

  return err;
}

static const char * save_sc68(vfs68_t * os, const disk68_t * mb,
                              int len, int version);

/* Save disk into file. */
int file68_save(vfs68_t * os, const disk68_t * mb, int version, int gzip)
{
  int len;
  const char * fname  = 0;
  const char * errstr = 0;
  vfs68_t * null_os = 0;
  vfs68_t * org_os  = 0;

  const char * header;
  int headsz;

  get_header(version, &header, &headsz);

  /* Get filename (for error message) */
  fname = vfs68_filename(os);

  /* Create a null stream to calculate total size.
     Needed by gzip stream that can't seek back. */
  null_os = uri68_vfs("null:", 3, 0);
  if (vfs68_open(null_os)) {
    errstr = "open";
  } else {
    errstr = save_sc68(null_os, mb, 0, version);
  }
  if (errstr) {
    goto error;
  }
  len = vfs68_length(null_os) - headsz;
  if (len <= 0) {
    errstr = "invalid stream length";
    goto error;
  }

  /* Wrap to gzip stream */
  if (gzip) {
    vfs68_z_option_t gzopt;

    org_os = os;
    gzopt = vfs68_z_default_option;
    gzopt.level = gzip;
    gzopt.name  = 0;
    os = vfs68_z_create(org_os, 2, gzopt);
    if (vfs68_open(os)) {
      errstr = "open";
      goto error;
    }
  }

  errstr = save_sc68(os, mb, len, version);

error:
  if (org_os) {
    /* Was gzipped: clean-up */
    vfs68_destroy(os);
  }
  vfs68_destroy(null_os);

  return errstr
    ? error68("file68: %s error -- %s",errstr,fname)
    : 0;
}

static const char * save_sc68(vfs68_t * os, const disk68_t * mb,
                              int len, int version)
{
  const char * errstr = 0;

  int opened = 0;

  const music68_t * mus;
  char * mname, * aname, /* * cname, */ * data;

  const char * header;
  int headsz;

  get_header(version, &header, &headsz);

  /* Check vfs */
  if (!os) {
    errstr = "null stream";
    goto error;
  }

  /* Check disk */
  if (!is_disk(mb)) {
    errstr = "not a sc68 disk";
    goto error;
  }

  /* Check number of music */
  if (mb->nb_mus <= 0 || mb->nb_mus > SC68_MAX_TRACK) {
    errstr = "invalid number of track";
    goto error;
  }

  /* SC68 file header string */
  if (vfs68_write(os, header, headsz) != headsz) {
    errstr = "header write";
    goto error;
  }
  /* SC68 disk-info chunks */
  if (save_chunk(os, CH68_BASE, 0, len)
      || save_noname  (os, CH68_FNAME,   mb->tags.tag.title.val)
      || save_noname  (os, CH68_ANAME,   mb->tags.tag.artist.val)
      || save_nonzero (os, CH68_DEFAULT, mb->def_mus)
      || save_tags    (os, &mb->tags, TAG68_ID_CUSTOM, 0)
    ) {
    errstr = "chunk write";
    goto error;
  }

  /* Reset previous value for various string */
  mname = mb->tags.tag.title.val;
  aname = mb->tags.tag.artist.val;
  /* cname =  */data = 0;
  for (mus = mb->mus; mus < mb->mus + mb->nb_mus; mus++) {
    int flags
      = 0
      | (mus->hwflags.bit.ym        ? SC68_YM     : 0)
      | (mus->hwflags.bit.ste       ? SC68_STE    : 0)
      | (mus->hwflags.bit.amiga     ? SC68_AMIGA  : 0)
      | (mus->hwflags.bit.stechoice ? SC68_STE    : 0)
      | (mus->hwflags.bit.timers    ? SC68_TIMERS : 0)
      | (mus->hwflags.bit.timera    ? SC68_TIMERA : 0)
      | (mus->hwflags.bit.timerb    ? SC68_TIMERB : 0)
      | (mus->hwflags.bit.timerc    ? SC68_TIMERC : 0)
      | (mus->hwflags.bit.timerd    ? SC68_TIMERD : 0)
      ;

    /* Save track-name, author, composer, replay */
    if (0
        || save_chunk(os, CH68_MUSIC, 0, 0) == -1
        || save_differstr(os, CH68_MNAME, mus->tags.tag.title.val,  mname)
        || save_differstr(os, CH68_ANAME, mus->tags.tag.artist.val, aname)
        || save_tags(os, &mus->tags, TAG68_ID_CUSTOM, 0) /* skip title/artist */
      ) {
      errstr = "chunk write";
      goto error;
    }
    if (mus->tags.tag.title.val) {
      mname = mus->tags.tag.title.val;
    }
    if (mus->tags.tag.artist.val) {
      aname = mus->tags.tag.artist.val;
    }

    /* Save play parms */
    if (0
        || save_string (os, CH68_REPLAY, mus->replay)
        || save_nonzero(os, CH68_D0,     mus->d0)
        || save_nonzero(os, CH68_AT,    !mus->has.pic     * mus->a0)
        || save_nonzero(os, CH68_FRQ,    (mus->frq != 50) * mus->frq)
        || save_nonzero(os, CH68_FRAME,  mus->has.time    * mus->first_fr)
        || save_nonzero(os, CH68_LOOP,   mus->has.loop    * mus->loops)
        || ( mus->has.loop &&
             save_number(os, CH68_LOOPFR,  mus->loops_fr) )
        || save_number (os, CH68_TYP,    flags)
        || ( mus->has.sfx &&
             save_chunk(os, CH68_SFX, 0, 0) )
      ) {
      errstr = "chunk write";
      goto error;
    }

    /* Save music data */
    if (mus->data && mus->data != data) {
      if (save_chunk(os, CH68_MDATA, mus->data, mus->datasz)) {
        errstr = "chunk write";
        goto error;
      }
      data = mus->data;
    }
  }

  /* SC68 last chunk */
  if (save_chunk(os, CH68_EOF, 0, 0)) {
    errstr = "chunk write";
    goto error;
  }

error:
  if (opened) {
    vfs68_close(os);
  }
  return errstr;
}

#endif /* #ifndef _FILE68_NO_SAVE_FUNCTION_ */

const char * file68_identifier(int version)
{
  return version == 1
    ? file68_idstr_v1
    : file68_idstr_v2
    ;
}

const char * file68_mimetype(void)
{
  return file68_mimestr;
}

int file68_loader_init(void)
{
  file68_cat = msg68_cat("loader", "music file loader", DEBUG_FILE68_O);
  return 0;
}

void file68_loader_shutdown(void)
{
  msg68_cat_free(file68_cat);
  file68_cat = msg68_DEFAULT;
}
