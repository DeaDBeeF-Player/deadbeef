/* OggEnc
 **
 ** This program is distributed under the GNU General Public License, version 2.
 ** A copy of this license is included with this source.
 **
 ** This particular file may also be distributed under (at your option) any
 ** later version of the GNU General Public License.
 **
 ** Copyright 2008, ogg.k.ogg.k <ogg.k.ogg.k@googlemail.com>
 **
 ** Portions from ffmpeg2theora, (c) j <j@v2v.cc>
 **/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <errno.h>

#ifdef HAVE_KATE
#include <kate/oggkate.h>
#endif

#include "lyrics.h"
#include "utf8.h"
#include "i18n.h"

typedef enum {
  lf_unknown,
  lf_srt,
  lf_lrc,
} lyrics_format;

#ifdef HAVE_KATE

static char *fgets2(char *s,size_t sz,FILE *f)
{
    char *ret = fgets(s, sz, f);
    if (ret) {
      /* fixup DOS newline character */
      char *ptr=strchr(ret, '\r');
      if (ptr) {
        *ptr='\n';
        *(ptr+1)=0;
      }
    }
    return ret;
}

static double hmsms2s(int h,int m,int s,int ms)
{
    return h*3600+m*60+s+ms/1000.0;
}

static int add_lyrics(oe_lyrics *lyrics, char *text, kate_motion *km, double t0,double t1)
{
  size_t len;
  int ret;
  char *utf8;

  ret=utf8_encode(text,&utf8);
  if (ret<0) {
    fprintf(stderr,_("Failed to convert to UTF-8: %s\n"),text);
    return ret;
  }

  lyrics->lyrics = (oe_lyrics_item*)realloc(lyrics->lyrics, (lyrics->count+1)*sizeof(oe_lyrics_item));
  if (!lyrics->lyrics) {
    free(utf8);
    fprintf(stderr, _("Out of memory\n"));
    return -1;
  }
  len = strlen(utf8);
  ret=kate_text_validate(kate_utf8,utf8,len+1);
  if (ret<0) {
    fprintf(stderr,_("WARNING: subtitle %s is not valid UTF-8\n"),utf8);
    free(utf8);
  }
  else {
    /* kill off trailing \n characters */
    while (len>0) {
      if (utf8[len-1]=='\n') utf8[--len]=0; else break;
    }
    lyrics->lyrics[lyrics->count].text = utf8;
    lyrics->lyrics[lyrics->count].len = len;
    lyrics->lyrics[lyrics->count].t0 = t0;
    lyrics->lyrics[lyrics->count].t1 = t1;
    lyrics->lyrics[lyrics->count].km = km;
    lyrics->count++;
  }
  return 0;
}

static int is_line_empty(const char *s)
{
  /* will work fine with UTF-8 despite the appearance */
  if (s) while (*s) {
    if (!strchr(" \t\r\n",*s)) return 0;
    ++s;
  }
  return 1;
}

static oe_lyrics *load_srt_lyrics(FILE *f)
{
    enum { need_id, need_timing, need_text };
    int need = need_id;
    int last_seen_id=0;
    int ret;
    int id;
    static char text[4096];
    static char str[4096];
    int h0,m0,s0,ms0,h1,m1,s1,ms1;
    double t0=0.0;
    double t1=0.0;
    oe_lyrics *lyrics;
    unsigned int line=0;

    if (!f) return NULL;

    lyrics=(oe_lyrics*)malloc(sizeof(oe_lyrics));
    if (!lyrics) return NULL;
    lyrics->count = 0;
    lyrics->lyrics = NULL;
    lyrics->karaoke = 0;

    fgets2(str,sizeof(str),f);
    ++line;
    while (!feof(f)) {
      switch (need) {
        case need_id:
          if (is_line_empty(str)) {
            /* be nice and ignore extra empty lines between records */
          }
          else {
            ret=sscanf(str,"%d\n",&id);
            if (ret!=1 || id<0) {
              fprintf(stderr,_("ERROR - line %u: Syntax error: %s\n"),line,str);
              free_lyrics(lyrics);
              return NULL;
            }
            if (id!=last_seen_id+1) {
              fprintf(stderr,_("WARNING - line %u: non consecutive ids: %s - pretending not to have noticed\n"),line,str);
            }
            last_seen_id=id;
            need=need_timing;
            strcpy(text,"");
          }
          break;
        case need_timing:
          /* we could use %u, but glibc accepts minus signs for %u for some reason */
          ret=sscanf(str,"%d:%d:%d%*[.,]%d --> %d:%d:%d%*[.,]%d\n",&h0,&m0,&s0,&ms0,&h1,&m1,&s1,&ms1);
          if (ret!=8 || (h0|m0|s0|ms0)<0 || (h1|m1|s1|ms1)<0) {
            fprintf(stderr,_("ERROR - line %u: Syntax error: %s\n"),line,str);
            free_lyrics(lyrics);
            return NULL;
          }
          else if (t1<t0) {
            fprintf(stderr,_("ERROR - line %u: end time must not be less than start time: %s\n"),line,str);
           free_lyrics(lyrics);
            return NULL;
          }
          else {
            t0=hmsms2s(h0,m0,s0,ms0);
            t1=hmsms2s(h1,m1,s1,ms1);
          }
          need=need_text;
          break;
        case need_text:
          if (str[0]=='\n') {
            if (add_lyrics(lyrics,text,NULL,t0,t1) < 0) {
              free_lyrics(lyrics);
              return NULL;
            }
            need=need_id;
          }
          else {
            /* in case of very long lines */
            size_t len=strlen(text);
            if (len+strlen(str) >= sizeof(text)) {
              fprintf(stderr, _("WARNING - line %u: text is too long - truncated\n"),line);
            }
            strncpy(text+len,str,sizeof(text)-len);
            text[sizeof(text)-1]=0;
          }
          break;
      }
      fgets2(str,sizeof(str),f);
      ++line;
    }

    if (need!=need_id) {
      /* shouldn't be a problem though, but warn */
      fprintf(stderr, _("WARNING - line %u: missing data - truncated file?\n"),line);
    }

    return lyrics;
}

static void add_kate_karaoke_tag(kate_motion *km,kate_float dt,const char *str,size_t len,int line)
{
  kate_curve *kc;
  kate_float ptr=(kate_float)-0.5;
  int ret;

  if (dt<0) {
    fprintf(stderr, _("WARNING - line %d: lyrics times must not be decreasing\n"), line);
    return;
  }

  /* work out how many glyphs we have */
  while (len>0) {
    ret=kate_text_get_character(kate_utf8,&str,&len);
    if (ret<0) {
      fprintf(stderr, _("WARNING - line %d: failed to get UTF-8 glyph from string\n"), line);
      return;
    }
    ptr+=(kate_float)1.0;
  }
  /* ptr now points to the middle of the glyph we're at */

  kc=(kate_curve*)malloc(sizeof(kate_curve));
  kate_curve_init(kc);
  kc->type=kate_curve_static;
  kc->npts=1;
  kc->pts=(kate_float*)malloc(2*sizeof(kate_float));
  kc->pts[0]=ptr;
  kc->pts[1]=(kate_float)0;

  km->ncurves++;
  km->curves=(kate_curve**)realloc(km->curves,km->ncurves*sizeof(kate_curve*));
  km->durations=(kate_float*)realloc(km->durations,km->ncurves*sizeof(kate_float));
  km->curves[km->ncurves-1]=kc;
  km->durations[km->ncurves-1]=dt;
}

static int fraction_to_milliseconds(int fraction,int digits)
{
  while (digits<3) {
    fraction*=10;
    ++digits;
  }
  while (digits>3) {
    fraction/=10;
    --digits;
  }
  return fraction;
}

static kate_motion *process_enhanced_lrc_tags(char *str,kate_float start_time,kate_float end_time,int line)
{
  char *start,*end;
  int ret;
  int m,s,fs;
  kate_motion *km=NULL;
  kate_float current_time = start_time;
  int f0,f1;

  if (!str) return NULL;

  start=str;
  while (1) {
    start=strchr(start,'<');
    if (!start) break;
    end=strchr(start+1,'>');
    if (!end) break;

    /* we found a <> pair, parse it */
    f0=f1=-1;
    ret=sscanf(start,"<%d:%d.%n%d%n>",&m,&s,&f0,&fs,&f1);

    /* remove the <> tag from input to get raw text */
    memmove(start,end+1,strlen(end+1)+1);

    if (ret<3 || (f0|f1)<0 || f0>=f1 || (m|s|fs)<0) {
      fprintf(stderr, _("WARNING - line %d: failed to process enhanced LRC tag (%*.*s) - ignored\n"),line,(int)(end-start+1),(int)(end-start+1),start);
    }
    else {
      kate_float tag_time=hmsms2s(0,m,s,fraction_to_milliseconds(fs,f1-f0));

      /* if this is the first tag in this line, create a kate motion */
      if (!km) {
        km=(kate_motion*)malloc(sizeof(kate_motion));
        if (!km) {
          fprintf(stderr, _("WARNING: failed to allocate memory - enhanced LRC tag will be ignored\n"));
        }
        else {
          kate_motion_init(km);
          km->semantics=kate_motion_semantics_glyph_pointer_1;
        }
      }
      /* add to the kate motion */
      if (km) {
        add_kate_karaoke_tag(km,tag_time-current_time,str,start-str,line);
        current_time = tag_time;
      }
    }
  }

  /* if we've found karaoke info, extend the motion to the end time */
  if (km) {
    add_kate_karaoke_tag(km,end_time-current_time,str,strlen(str),line);
  }

  return km;
}

static oe_lyrics *load_lrc_lyrics(FILE *f)
{
  oe_lyrics *lyrics;
  static char str[4096];
  static char lyrics_line[4096]="";
  int m,s,fs;
  double t,start_time = -1.0;
  int offset;
  int ret;
  unsigned line=0;
  kate_motion *km;
  int f0,f1;

  if (!f) return NULL;

  /* skip headers */
  fgets2(str,sizeof(str),f);
  ++line;
  while (!feof(f)) {
    ret = sscanf(str, "[%d:%d.%d]%n\n",&m,&s,&fs,&offset);
    if (ret >= 3)
      break;
    fgets2(str,sizeof(str),f);
    ++line;
  }
  if (feof(f)) {
    fprintf(stderr,_("ERROR - line %u: Syntax error: %s\n"),line,str);
    return NULL;
  }

  lyrics=(oe_lyrics*)malloc(sizeof(oe_lyrics));
  if (!lyrics) return NULL;
  lyrics->count = 0;
  lyrics->lyrics = NULL;
  lyrics->karaoke = 0;

  while (!feof(f)) {
    /* ignore empty lines */
    if (!is_line_empty(str)) {
      f0=f1=-1;
      ret=sscanf(str, "[%d:%d.%n%d%n]%n\n",&m,&s,&f0,&fs,&f1,&offset);
      if (ret<3 || (f0|f1)<0 || f1<=f0 || (m|s|fs)<0) {
        fprintf(stderr,_("ERROR - line %u: Syntax error: %s\n"),line,str);
        free_lyrics(lyrics);
        return NULL;
      }
      t=hmsms2s(0,m,s,fraction_to_milliseconds(fs,f1-f0));

      if (start_time>=0.0 && !is_line_empty(lyrics_line)) {
        km=process_enhanced_lrc_tags(lyrics_line,start_time,t,line);
        if (km) {
          lyrics->karaoke = 1;
        }
        if (add_lyrics(lyrics,lyrics_line,km,start_time,t) < 0) {
          free_lyrics(lyrics);
          return NULL;
        }
      }

      strncpy(lyrics_line,str+offset,sizeof(lyrics_line));
      lyrics_line[sizeof(lyrics_line)-1]=0;
      start_time=t;
    }

    fgets2(str,sizeof(str),f);
    ++line;
  }

  return lyrics;
}

/* very weak checks, but we only support two formats, so it's ok */
lyrics_format probe_lyrics_format(FILE *f)
{
  int dummy_int;
  static char str[4096];
  lyrics_format format=lf_unknown;
  long pos;

  if (!f) return lf_unknown;

  pos=ftell(f);
  fgets2(str,sizeof(str),f);

  /* srt */
  if (sscanf(str, "%d\n", &dummy_int) == 1 && dummy_int>=0)
    format=lf_srt;

  /* lrc */
  if (str[0] == '[')
    format=lf_lrc;

  fseek(f,pos,SEEK_SET);

  return format;
}

#endif

oe_lyrics *load_lyrics(const char *filename)
{
#ifdef HAVE_KATE
  static char str[4096];
  int ret;
  oe_lyrics *lyrics=NULL;
  FILE *f;

  if (!filename) {
      fprintf(stderr,_("ERROR: No lyrics filename to load from\n"));
      return NULL;
  }

  f = fopen(filename, "r");
  if (!f) {
      fprintf(stderr,_("ERROR: Failed to open lyrics file %s (%s)\n"), filename, strerror(errno));
      return NULL;
  }

  /* first, check for a BOM */
  ret=fread(str,1,3,f);
  if (ret<3 || memcmp(str,"\xef\xbb\xbf",3)) {
    /* No BOM, rewind */
    fseek(f,0,SEEK_SET);
  }

  switch (probe_lyrics_format(f)) {
    case lf_srt:
      lyrics = load_srt_lyrics(f);
      break;
    case lf_lrc:
      lyrics = load_lrc_lyrics(f);
      break;
    default:
      fprintf(stderr, _("ERROR: Failed to load %s - can't determine format\n"), filename);
      break;
  }

  fclose(f);

  return lyrics;
#else
  return NULL;
#endif
}

void free_lyrics(oe_lyrics *lyrics)
{
#ifdef HAVE_KATE
    size_t n,c;
    if (lyrics) {
        for (n=0; n<lyrics->count; ++n) {
          oe_lyrics_item *li=&lyrics->lyrics[n];
          free(li->text);
          if (li->km) {
            for (c=0; c<li->km->ncurves; ++c) {
              free(li->km->curves[c]->pts);
              free(li->km->curves[c]);
            }
            free(li->km->curves);
            free(li->km->durations);
            free(li->km);
          }
        }
        free(lyrics->lyrics);
        free(lyrics);
    }
#endif
}

const oe_lyrics_item *get_lyrics(const oe_lyrics *lyrics, double t, size_t *idx)
{
#ifdef HAVE_KATE
    if (!lyrics || *idx>=lyrics->count) return NULL;
    if (lyrics->lyrics[*idx].t0 > t) return NULL;
    return &lyrics->lyrics[(*idx)++];
#else
    return NULL;
#endif
}
