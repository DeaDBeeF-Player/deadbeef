/* Ogginfo
 *
 * A tool to describe ogg file contents and metadata.
 *
 * Copyright 2002-2005 Michael Smith <msmith@xiph.org>
 * Licensed under the GNU GPL, distributed with this program.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <getopt.h>
#include <math.h>

#include <ogg/ogg.h>
#include <vorbis/codec.h>

#ifdef HAVE_KATE
#include <kate/oggkate.h>
#endif

#include <locale.h>
#include "utf8.h"
#include "i18n.h"

#include "theora.h"

#define CHUNK 4500

#ifdef _WIN32
#define I64FORMAT "I64d"
#else
#define I64FORMAT "lld"
#endif

struct vorbis_release {
    char *vendor_string;
    char *desc;
} releases[] = {
        {"Xiphophorus libVorbis I 20000508", "1.0 beta 1 or beta 2"},
        {"Xiphophorus libVorbis I 20001031", "1.0 beta 3"},
        {"Xiphophorus libVorbis I 20010225", "1.0 beta 4"},
        {"Xiphophorus libVorbis I 20010615", "1.0 rc1"},
        {"Xiphophorus libVorbis I 20010813", "1.0 rc2"},
        {"Xiphophorus libVorbis I 20011217", "1.0 rc3"},
        {"Xiphophorus libVorbis I 20011231", "1.0 rc3"},
        {"Xiph.Org libVorbis I 20020717", "1.0"},
        {"Xiph.Org libVorbis I 20030909", "1.0.1"},
        {"Xiph.Org libVorbis I 20040629", "1.1.0"},
	{"Xiph.Org libVorbis I 20050304", "1.1.1"},
	{"Xiph.Org libVorbis I 20050304", "1.1.2"},
	{"Xiph.Org libVorbis I 20070622", "1.2.0"},
	{"Xiph.Org libVorbis I 20080501", "1.2.1"},
        {NULL, NULL},
    };


/* TODO:
 *
 * - detect violations of muxing constraints
 * - detect granulepos 'gaps' (possibly vorbis-specific). (seperate from
 *   serial-number gaps)
 */

typedef struct _stream_processor {
    void (*process_page)(struct _stream_processor *, ogg_page *);
    void (*process_end)(struct _stream_processor *);
    int isillegal;
    int constraint_violated;
    int shownillegal;
    int isnew;
    long seqno;
    int lostseq;

    int start;
    int end;

    int num;
    char *type;

    ogg_uint32_t serial; /* must be 32 bit unsigned */
    ogg_stream_state os;
    void *data;
} stream_processor;

typedef struct {
    stream_processor *streams;
    int allocated;
    int used;

    int in_headers;
} stream_set;

typedef struct {
    vorbis_info vi;
    vorbis_comment vc;

    ogg_int64_t bytes;
    ogg_int64_t lastgranulepos;
    ogg_int64_t firstgranulepos;

    int doneheaders;
} misc_vorbis_info;

typedef struct {
    theora_info ti;
    theora_comment tc;

    ogg_int64_t bytes;
    ogg_int64_t lastgranulepos;
    ogg_int64_t firstgranulepos;

    int doneheaders;

    ogg_int64_t framenum_expected;
} misc_theora_info;

typedef struct {
#ifdef HAVE_KATE
    kate_info ki;
    kate_comment kc;
#else
    int num_headers;
#endif

    int major;
    int minor;
    char language[16];
    char category[16];

    ogg_int64_t bytes;
    ogg_int64_t lastgranulepos;
    ogg_int64_t firstgranulepos;

    int doneheaders;
} misc_kate_info;

static int printlots = 0;
static int printinfo = 1;
static int printwarn = 1;
static int verbose = 1;

static int flawed;

#define CONSTRAINT_PAGE_AFTER_EOS   1
#define CONSTRAINT_MUXING_VIOLATED  2

static stream_set *create_stream_set(void) {
    stream_set *set = calloc(1, sizeof(stream_set));

    set->streams = calloc(5, sizeof(stream_processor));
    set->allocated = 5;
    set->used = 0;

    return set;
}

static void info(char *format, ...) 
{
    va_list ap;

    if(!printinfo)
        return;

    va_start(ap, format);
    vfprintf(stdout, format, ap);
    va_end(ap);
}

static void warn(char *format, ...)
{
    va_list ap;

    flawed = 1;
    if(!printwarn)
        return;

    va_start(ap, format);
    vfprintf(stdout, format, ap);
    va_end(ap);
}

static void error(char *format, ...)
{
    va_list ap;

    flawed = 1;

    va_start(ap, format);
    vfprintf(stdout, format, ap);
    va_end(ap);
}

static void check_xiph_comment(stream_processor *stream, int i, const char *comment,
    int comment_length)
{
    char *sep = strchr(comment, '=');
    char *decoded;
    int j;
    int broken = 0;
    unsigned char *val;
    int bytes;
    int remaining;

    if(sep == NULL) {
        warn(_("WARNING: Comment %d in stream %d has invalid "
              "format, does not contain '=': \"%s\"\n"), 
              i, stream->num, comment);
             return;
    }

    for(j=0; j < sep-comment; j++) {
        if(comment[j] < 0x20 || comment[j] > 0x7D) {
            warn(_("WARNING: Invalid comment fieldname in "
                   "comment %d (stream %d): \"%s\"\n"),
                   i, stream->num, comment);
            broken = 1;
            break;
        }
    }

    if(broken)
	return;

    val = (unsigned char *)comment;

    j = sep-comment+1;
    while(j < comment_length)
    {
        remaining = comment_length - j;
        if((val[j] & 0x80) == 0)
            bytes = 1;
        else if((val[j] & 0x40) == 0x40) {
            if((val[j] & 0x20) == 0)
                bytes = 2;
            else if((val[j] & 0x10) == 0)
                bytes = 3;
            else if((val[j] & 0x08) == 0)
                bytes = 4;
            else if((val[j] & 0x04) == 0)
                bytes = 5;
            else if((val[j] & 0x02) == 0)
                bytes = 6;
            else {
                warn(_("WARNING: Illegal UTF-8 sequence in "
                    "comment %d (stream %d): length marker wrong\n"),
                    i, stream->num);
                broken = 1;
                break;
            }
        }
        else {
            warn(_("WARNING: Illegal UTF-8 sequence in comment "
                "%d (stream %d): length marker wrong\n"), i, stream->num);
            broken = 1;
            break;
        }

        if(bytes > remaining) {
            warn(_("WARNING: Illegal UTF-8 sequence in comment "
                "%d (stream %d): too few bytes\n"), i, stream->num);
            broken = 1;
            break;
        }

        switch(bytes) {
            case 1:
                /* No more checks needed */
                break;
            case 2:
                if((val[j+1] & 0xC0) != 0x80)
                    broken = 1;
                if((val[j] & 0xFE) == 0xC0)
                    broken = 1;
                break;
            case 3:
                if(!((val[j] == 0xE0 && val[j+1] >= 0xA0 && val[j+1] <= 0xBF && 
                         (val[j+2] & 0xC0) == 0x80) ||
                     (val[j] >= 0xE1 && val[j] <= 0xEC && 
                         (val[j+1] & 0xC0) == 0x80 &&
                         (val[j+2] & 0xC0) == 0x80) ||
                     (val[j] == 0xED && val[j+1] >= 0x80 &&
                         val[j+1] <= 0x9F &&
                         (val[j+2] & 0xC0) == 0x80) ||
                     (val[j] >= 0xEE && val[j] <= 0xEF &&
                         (val[j+1] & 0xC0) == 0x80 &&
                         (val[j+2] & 0xC0) == 0x80)))
                     broken = 1;
                 if(val[j] == 0xE0 && (val[j+1] & 0xE0) == 0x80)
                     broken = 1;
                 break;
            case 4:
                 if(!((val[j] == 0xF0 && val[j+1] >= 0x90 &&
                         val[j+1] <= 0xBF &&
                         (val[j+2] & 0xC0) == 0x80 &&
                         (val[j+3] & 0xC0) == 0x80) ||
                     (val[j] >= 0xF1 && val[j] <= 0xF3 &&
                         (val[j+1] & 0xC0) == 0x80 &&
                         (val[j+2] & 0xC0) == 0x80 &&
                         (val[j+3] & 0xC0) == 0x80) ||
                     (val[j] == 0xF4 && val[j+1] >= 0x80 &&
                         val[j+1] <= 0x8F &&
                         (val[j+2] & 0xC0) == 0x80 &&
                         (val[j+3] & 0xC0) == 0x80)))
                     broken = 1;
                 if(val[j] == 0xF0 && (val[j+1] & 0xF0) == 0x80)
                     broken = 1;
                 break;
             /* 5 and 6 aren't actually allowed at this point */
             case 5:
                 broken = 1;
                 break;
             case 6:
                 broken = 1;
                 break;
         }

         if(broken) {
             char *simple = malloc (comment_length + 1);
             char *seq = malloc (comment_length * 3 + 1);
             static char hex[] = {'0', '1', '2', '3', '4', '5', '6', '7', 
                                  '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
             int i, c1 = 0, c2 = 0;
             for (i = 0; i < comment_length; i++) {
               seq[c1++] = hex[((unsigned char)comment[i]) >> 4];
               seq[c1++] = hex[((unsigned char)comment[i]) & 0xf];
               seq[c1++] = ' ';

               if(comment[i] < 0x20 || comment[i] > 0x7D)
                 simple[c2++] = '?';
               else
                 simple[c2++] = comment[i];
             }
             seq[c1] = 0;
             simple[c2] = 0;
             warn(_("WARNING: Illegal UTF-8 sequence in comment "
                   "%d (stream %d): invalid sequence \"%s\": %s\n"), i, 
                   stream->num, simple, seq);
             broken = 1;
             free (simple);
             free (seq);
             break;
         }

         j += bytes;
     }

     if(!broken) {
         if(utf8_decode(sep+1, &decoded) < 0) {
             warn(_("WARNING: Failure in UTF-8 decoder. This should not be possible\n"));
             return;
	 }
         *sep = 0;
         if(!broken) {
           info("\t%s=%s\n", comment, decoded);
           free(decoded);
         }
     }
}

static void theora_process(stream_processor *stream, ogg_page *page)
{
    ogg_packet packet;
    misc_theora_info *inf = stream->data;
    int i, header=0;
    int res;

    ogg_stream_pagein(&stream->os, page);
    if(inf->doneheaders < 3)
        header = 1;

    while(1) {
        res = ogg_stream_packetout(&stream->os, &packet);
        if(res < 0) {
           warn(_("WARNING: discontinuity in stream (%d)\n"), stream->num);
           continue;
        }
        else if (res == 0)
            break;

        if(inf->doneheaders < 3) {
            if(theora_decode_header(&inf->ti, &inf->tc, &packet) < 0) {
                warn(_("WARNING: Could not decode Theora header "
                       "packet - invalid Theora stream (%d)\n"), stream->num);
                continue;
            }
            inf->doneheaders++;
            if(inf->doneheaders == 3) {
                if(ogg_page_granulepos(page) != 0 || ogg_stream_packetpeek(&stream->os, NULL) == 1)
                    warn(_("WARNING: Theora stream %d does not have headers "
                           "correctly framed. Terminal header page contains "
                           "additional packets or has non-zero granulepos\n"),
                            stream->num);
                info(_("Theora headers parsed for stream %d, "
                       "information follows...\n"), stream->num);

                info(_("Version: %d.%d.%d\n"), inf->ti.version_major, inf->ti.version_minor, inf->ti.version_subminor);

                info(_("Vendor: %s\n"), inf->tc.vendor);
                info(_("Width: %d\n"), inf->ti.frame_width);
                info(_("Height: %d\n"), inf->ti.frame_height);
		info(_("Total image: %d by %d, crop offset (%d, %d)\n"),
		    inf->ti.width, inf->ti.height, inf->ti.offset_x, inf->ti.offset_y);
		if(inf->ti.offset_x + inf->ti.frame_width > inf->ti.width)
		    warn(_("Frame offset/size invalid: width incorrect\n"));
		if(inf->ti.offset_y + inf->ti.frame_height > inf->ti.height)
		    warn(_("Frame offset/size invalid: height incorrect\n"));

		if(inf->ti.fps_numerator == 0 || inf->ti.fps_denominator == 0) 
		   warn(_("Invalid zero framerate\n"));
		else
		   info(_("Framerate %d/%d (%.02f fps)\n"), inf->ti.fps_numerator, inf->ti.fps_denominator, (float)inf->ti.fps_numerator/(float)inf->ti.fps_denominator);
		
		if(inf->ti.aspect_numerator == 0 || inf->ti.aspect_denominator == 0) 
		{
		    info(_("Aspect ratio undefined\n"));
		}	
		else
		{
		    float frameaspect = (float)inf->ti.frame_width/(float)inf->ti.frame_height * (float)inf->ti.aspect_numerator/(float)inf->ti.aspect_denominator; 
		    info(_("Pixel aspect ratio %d:%d (%f:1)\n"), inf->ti.aspect_numerator, inf->ti.aspect_denominator, (float)inf->ti.aspect_numerator/(float)inf->ti.aspect_denominator);
                    if(fabs(frameaspect - 4.0/3.0) < 0.02)
			info(_("Frame aspect 4:3\n"));
                    else if(fabs(frameaspect - 16.0/9.0) < 0.02)
			info(_("Frame aspect 16:9\n"));
		    else
			info(_("Frame aspect %f:1\n"), frameaspect);
		}

		if(inf->ti.colorspace == OC_CS_ITU_REC_470M)
		    info(_("Colourspace: Rec. ITU-R BT.470-6 System M (NTSC)\n")); 
		else if(inf->ti.colorspace == OC_CS_ITU_REC_470BG)
		    info(_("Colourspace: Rec. ITU-R BT.470-6 Systems B and G (PAL)\n")); 
		else
		    info(_("Colourspace unspecified\n"));

		if(inf->ti.pixelformat == OC_PF_420)
		    info(_("Pixel format 4:2:0\n"));
		else if(inf->ti.pixelformat == OC_PF_422)
		    info(_("Pixel format 4:2:2\n"));
		else if(inf->ti.pixelformat == OC_PF_444)
		    info(_("Pixel format 4:4:4\n"));
		else
		    warn(_("Pixel format invalid\n"));

		info(_("Target bitrate: %d kbps\n"), inf->ti.target_bitrate/1000);
		info(_("Nominal quality setting (0-63): %d\n"), inf->ti.quality);

                if(inf->tc.comments > 0)
                    info(_("User comments section follows...\n"));

                for(i=0; i < inf->tc.comments; i++) {
                    char *comment = inf->tc.user_comments[i];
		    check_xiph_comment(stream, i, comment, 
		            inf->tc.comment_lengths[i]);
		}
	    }
	}
        else {
            ogg_int64_t framenum;
            ogg_int64_t iframe,pframe;
            ogg_int64_t gp = packet.granulepos;

            if(gp > 0) {
                iframe=gp>>inf->ti.granule_shift;
                pframe=gp-(iframe<<inf->ti.granule_shift);
                framenum = iframe+pframe;
                if(inf->framenum_expected >= 0 && 
                    inf->framenum_expected != framenum)
                {
                    warn(_("WARNING: Expected frame %" I64FORMAT 
                           ", got %" I64FORMAT "\n"), 
                           inf->framenum_expected, framenum);
                }
                inf->framenum_expected = framenum + 1;
            }
            else if (inf->framenum_expected >= 0) {
                inf->framenum_expected++;
            }
        }
    }

    if(!header) {
        ogg_int64_t gp = ogg_page_granulepos(page);
        if(gp > 0) {
            if(gp < inf->lastgranulepos)
                warn(_("WARNING: granulepos in stream %d decreases from %" 
                        I64FORMAT " to %" I64FORMAT "\n"),
                        stream->num, inf->lastgranulepos, gp);
            inf->lastgranulepos = gp;
        }
        if(inf->firstgranulepos < 0) { /* Not set yet */
        }
        inf->bytes += page->header_len + page->body_len;
    }
}

static void theora_end(stream_processor *stream) 
{
    misc_theora_info *inf = stream->data;
    long minutes, seconds, milliseconds;
    double bitrate, time;

    /* This should be lastgranulepos - startgranulepos, or something like that*/
    ogg_int64_t iframe=inf->lastgranulepos>>inf->ti.granule_shift;
    ogg_int64_t pframe=inf->lastgranulepos-(iframe<<inf->ti.granule_shift);
    time = (double)(iframe+pframe) /
	((float)inf->ti.fps_numerator/(float)inf->ti.fps_denominator);
    minutes = (long)time / 60;
    seconds = (long)time - minutes*60;
    milliseconds = (long)((time - minutes*60 - seconds)*1000);
    bitrate = inf->bytes*8 / time / 1000.0;

    info(_("Theora stream %d:\n"
           "\tTotal data length: %" I64FORMAT " bytes\n"
           "\tPlayback length: %ldm:%02ld.%03lds\n"
           "\tAverage bitrate: %f kb/s\n"), 
            stream->num,inf->bytes, minutes, seconds, milliseconds, bitrate);

    theora_comment_clear(&inf->tc);
    theora_info_clear(&inf->ti);

    free(stream->data);
}


static void vorbis_process(stream_processor *stream, ogg_page *page )
{
    ogg_packet packet;
    misc_vorbis_info *inf = stream->data;
    int i, header=0, packets=0;
    int k;
    int res;

    ogg_stream_pagein(&stream->os, page);
    if(inf->doneheaders < 3)
        header = 1;

    while(1) {
        res = ogg_stream_packetout(&stream->os, &packet);
        if(res < 0) {
           warn(_("WARNING: discontinuity in stream (%d)\n"), stream->num);
           continue;
        }
        else if (res == 0)
            break;

        packets++;
        if(inf->doneheaders < 3) {
            if(vorbis_synthesis_headerin(&inf->vi, &inf->vc, &packet) < 0) {
                warn(_("WARNING: Could not decode Vorbis header "
                       "packet %d - invalid Vorbis stream (%d)\n"), 
                        inf->doneheaders, stream->num);
                continue;
            }
            inf->doneheaders++;
            if(inf->doneheaders == 3) {
                if(ogg_page_granulepos(page) != 0 || ogg_stream_packetpeek(&stream->os, NULL) == 1)
                    warn(_("WARNING: Vorbis stream %d does not have headers "
                           "correctly framed. Terminal header page contains "
                           "additional packets or has non-zero granulepos\n"),
                            stream->num);
                info(_("Vorbis headers parsed for stream %d, "
                       "information follows...\n"), stream->num);

                info(_("Version: %d\n"), inf->vi.version);
                k = 0;
                while(releases[k].vendor_string) {
                    if(!strcmp(inf->vc.vendor, releases[k].vendor_string)) {
                        info(_("Vendor: %s (%s)\n"), inf->vc.vendor, 
                                    releases[k].desc);
                        break;
                    }
                    k++;
                }
                if(!releases[k].vendor_string)
                    info(_("Vendor: %s\n"), inf->vc.vendor);
                info(_("Channels: %d\n"), inf->vi.channels);
                info(_("Rate: %ld\n\n"), inf->vi.rate);

                if(inf->vi.bitrate_nominal > 0)
                    info(_("Nominal bitrate: %f kb/s\n"), 
                            (double)inf->vi.bitrate_nominal / 1000.0);
                else
                    info(_("Nominal bitrate not set\n"));

                if(inf->vi.bitrate_upper > 0)
                    info(_("Upper bitrate: %f kb/s\n"), 
                            (double)inf->vi.bitrate_upper / 1000.0);
                else
                    info(_("Upper bitrate not set\n"));

                if(inf->vi.bitrate_lower > 0)
                    info(_("Lower bitrate: %f kb/s\n"), 
                            (double)inf->vi.bitrate_lower / 1000.0);
                else
                    info(_("Lower bitrate not set\n"));

                if(inf->vc.comments > 0)
                    info(_("User comments section follows...\n"));

                for(i=0; i < inf->vc.comments; i++) {
                    char *comment = inf->vc.user_comments[i];
		    check_xiph_comment(stream, i, comment, 
		            inf->vc.comment_lengths[i]);
		}
            }
        }
    }

    if(!header) {
        ogg_int64_t gp = ogg_page_granulepos(page);
        if(gp > 0) {
            if(gp < inf->lastgranulepos)
                warn(_("WARNING: granulepos in stream %d decreases from %" 
                        I64FORMAT " to %" I64FORMAT "\n" ),
                        stream->num, inf->lastgranulepos, gp);
            inf->lastgranulepos = gp;
        }
        else if(packets) {
            /* Only do this if we saw at least one packet ending on this page.
             * It's legal (though very unusual) to have no packets in a page at
             * all - this is occasionally used to have an empty EOS page */
            warn(_("Negative or zero granulepos (%" I64FORMAT ") on Vorbis stream outside of headers. This file was created by a buggy encoder\n"), gp);
        }
        if(inf->firstgranulepos < 0) { /* Not set yet */
        }
        inf->bytes += page->header_len + page->body_len;
    }
}

static void vorbis_end(stream_processor *stream) 
{
    misc_vorbis_info *inf = stream->data;
    long minutes, seconds, milliseconds;
    double bitrate, time;

    /* This should be lastgranulepos - startgranulepos, or something like that*/
    time = (double)inf->lastgranulepos / inf->vi.rate;
    minutes = (long)time / 60;
    seconds = (long)time - minutes*60;
    milliseconds = (long)((time - minutes*60 - seconds)*1000);
    bitrate = inf->bytes*8 / time / 1000.0;

    info(_("Vorbis stream %d:\n"
           "\tTotal data length: %" I64FORMAT " bytes\n"
           "\tPlayback length: %ldm:%02ld.%03lds\n"
           "\tAverage bitrate: %f kb/s\n"), 
            stream->num,inf->bytes, minutes, seconds, milliseconds, bitrate);

    vorbis_comment_clear(&inf->vc);
    vorbis_info_clear(&inf->vi);

    free(stream->data);
}

static void kate_process(stream_processor *stream, ogg_page *page )
{
    ogg_packet packet;
    misc_kate_info *inf = stream->data;
    int header=0, packets=0;
    int res;
#ifdef HAVE_KATE
    int i;
    const char *encoding = NULL, *directionality = NULL;
#endif

    ogg_stream_pagein(&stream->os, page);
    if(!inf->doneheaders)
        header = 1;

    while(1) {
        res = ogg_stream_packetout(&stream->os, &packet);
        if(res < 0) {
           warn(_("WARNING: discontinuity in stream (%d)\n"), stream->num);
           continue;
        }
        else if (res == 0)
            break;

        packets++;
        if(!inf->doneheaders) {
#ifdef HAVE_KATE
            int ret = kate_ogg_decode_headerin(&inf->ki, &inf->kc, &packet);
            if(ret < 0) {
                warn(_("WARNING: Could not decode Kate header "
                       "packet %d - invalid Kate stream (%d)\n"), 
                        packet.packetno, stream->num);
                continue;
            }
            else if (ret > 0) {
                inf->doneheaders=1;
            }
#else
            /* if we're not building against libkate, do some limited checks */
            if (packet.bytes<64 || memcmp(packet.packet+1, "kate\0\0\0", 7)) {
                warn(_("WARNING: packet %d does not seem to be a Kate header - "
                       "invalid Kate stream (%d)\n"), 
                        packet.packetno, stream->num);
                continue;
            }
            if (packet.packetno==inf->num_headers) {
                inf->doneheaders=1;
            }
#endif

            if (packet.packetno==0) {
#ifdef HAVE_KATE
                inf->major = inf->ki.bitstream_version_major;
                inf->minor = inf->ki.bitstream_version_minor;
                memcpy(inf->language, inf->ki.language, 16);
                inf->language[15] = 0;
                memcpy(inf->category, inf->ki.category, 16);
                inf->category[15] = 0;
#else
                inf->major = packet.packet[9];
                inf->minor = packet.packet[10];
                inf->num_headers = packet.packet[11];
                memcpy(inf->language, packet.packet+32, 16);
                inf->language[15] = 0;
                memcpy(inf->category, packet.packet+48, 16);
                inf->category[15] = 0;
#endif
            }

            if(inf->doneheaders) {
                if(ogg_page_granulepos(page) != 0 || ogg_stream_packetpeek(&stream->os, NULL) == 1)
                    warn(_("WARNING: Kate stream %d does not have headers "
                           "correctly framed. Terminal header page contains "
                           "additional packets or has non-zero granulepos\n"),
                            stream->num);
                info(_("Kate headers parsed for stream %d, "
                       "information follows...\n"), stream->num);

                info(_("Version: %d.%d\n"), inf->major, inf->minor);
#ifdef HAVE_KATE
                info(_("Vendor: %s\n"), inf->kc.vendor);
#endif

                if (*inf->language) {
                    info(_("Language: %s\n"), inf->language);
                }
                else {
                    info(_("No language set\n"));
                }
                if (*inf->category) {
                    info(_("Category: %s\n"), inf->category);
                }
                else {
                    info(_("No category set\n"));
                }

#ifdef HAVE_KATE
                switch (inf->ki.text_encoding) {
                  case kate_utf8: encoding=_("utf-8"); break;
                  default: encoding=NULL; break;
                }
                if (encoding) {
                    info(_("Character encoding: %s\n"),encoding);
                }
                else {
                    info(_("Unknown character encoding\n"));
                }

                if (printlots) {
                    switch (inf->ki.text_directionality) {
                      case kate_l2r_t2b: directionality=_("left to right, top to bottom"); break;
                      case kate_r2l_t2b: directionality=_("right to left, top to bottom"); break;
                      case kate_t2b_r2l: directionality=_("top to bottom, right to left"); break;
                      case kate_t2b_l2r: directionality=_("top to bottom, left to right"); break;
                      default: directionality=NULL; break;
                    }
                    if (directionality) {
                        info(_("Text directionality: %s\n"),directionality);
                    }
                    else {
                        info(_("Unknown text directionality\n"));
                    }

                    info("%u regions, %u styles, %u curves, %u motions, %u palettes,\n"
                         "%u bitmaps, %u font ranges, %u font mappings\n",
                         inf->ki.nregions, inf->ki.nstyles,
                         inf->ki.ncurves, inf->ki.nmotions,
                         inf->ki.npalettes, inf->ki.nbitmaps,
                         inf->ki.nfont_ranges, inf->ki.nfont_mappings);
                }

		if(inf->ki.gps_numerator == 0 || inf->ki.gps_denominator == 0) 
		   warn(_("Invalid zero granulepos rate\n"));
		else
		   info(_("Granulepos rate %d/%d (%.02f gps)\n"),
                       inf->ki.gps_numerator, inf->ki.gps_denominator,
                       (float)inf->ki.gps_numerator/(float)inf->ki.gps_denominator);
		
                if(inf->kc.comments > 0)
                    info(_("User comments section follows...\n"));

                for(i=0; i < inf->kc.comments; i++) {
                    const char *comment = inf->kc.user_comments[i];
		    check_xiph_comment(stream, i, comment, 
		            inf->kc.comment_lengths[i]);
		}
#endif
                info(_("\n"));
            }
        }
    }

    if(!header) {
        ogg_int64_t gp = ogg_page_granulepos(page);
        if(gp > 0) {
            if(gp < inf->lastgranulepos)
                warn(_("WARNING: granulepos in stream %d decreases from %" 
                        I64FORMAT " to %" I64FORMAT "\n" ),
                        stream->num, inf->lastgranulepos, gp);
            inf->lastgranulepos = gp;
        }
        else if(packets && gp<0) { /* zero granpos on data is valid for kate */
            /* Only do this if we saw at least one packet ending on this page.
             * It's legal (though very unusual) to have no packets in a page at
             * all - this is occasionally used to have an empty EOS page */
            warn(_("Negative granulepos (%" I64FORMAT ") on Kate stream outside of headers. This file was created by a buggy encoder\n"), gp);
        }
        if(inf->firstgranulepos < 0) { /* Not set yet */
        }
        inf->bytes += page->header_len + page->body_len;
    }
}

#ifdef HAVE_KATE
static void kate_end(stream_processor *stream) 
{
    misc_kate_info *inf = stream->data;
    long minutes, seconds, milliseconds;
    double bitrate, time;

    /* This should be lastgranulepos - startgranulepos, or something like that*/
    //time = (double)(inf->lastgranulepos>>inf->ki.granule_shift) * inf->ki.gps_denominator / inf->ki.gps_numerator;
    ogg_int64_t gbase=inf->lastgranulepos>>inf->ki.granule_shift;
    ogg_int64_t goffset=inf->lastgranulepos-(gbase<<inf->ki.granule_shift);
    time = (double)(gbase+goffset) / ((float)inf->ki.gps_numerator/(float)inf->ki.gps_denominator);
    minutes = (long)time / 60;
    seconds = (long)time - minutes*60;
    milliseconds = (long)((time - minutes*60 - seconds)*1000);
    bitrate = inf->bytes*8 / time / 1000.0;

    info(_("Kate stream %d:\n"
           "\tTotal data length: %" I64FORMAT " bytes\n"
           "\tPlayback length: %ldm:%02ld.%03lds\n"
           "\tAverage bitrate: %f kb/s\n"), 
            stream->num,inf->bytes, minutes, seconds, milliseconds, bitrate);

    kate_comment_clear(&inf->kc);
    kate_info_clear(&inf->ki);

    free(stream->data);
}
#else
static void kate_end(stream_processor *stream) 
{
}
#endif


static void process_null(stream_processor *stream, ogg_page *page)
{
    /* This is for invalid streams. */
}

static void process_other(stream_processor *stream, ogg_page *page )
{
    ogg_packet packet;

    ogg_stream_pagein(&stream->os, page);

    while(ogg_stream_packetout(&stream->os, &packet) > 0) {
        /* Should we do anything here? Currently, we don't */
    }
}


static void free_stream_set(stream_set *set)
{
    int i;
    for(i=0; i < set->used; i++) {
        if(!set->streams[i].end) {
            warn(_("WARNING: EOS not set on stream %d\n"), 
                    set->streams[i].num);
            if(set->streams[i].process_end)
                set->streams[i].process_end(&set->streams[i]);
        }
        ogg_stream_clear(&set->streams[i].os);
    }

    free(set->streams);
    free(set);
}

static int streams_open(stream_set *set)
{
    int i;
    int res=0;
    for(i=0; i < set->used; i++) {
        if(!set->streams[i].end)
            res++;
    }

    return res;
}

static void null_start(stream_processor *stream)
{
    stream->process_end = NULL;
    stream->type = "invalid";
    stream->process_page = process_null;
}

static void other_start(stream_processor *stream, char *type)
{
    if(type)
        stream->type = type;
    else
        stream->type = "unknown";
    stream->process_page = process_other;
    stream->process_end = NULL;
}

static void theora_start(stream_processor *stream)
{
    misc_theora_info *info;

    stream->type = "theora";
    stream->process_page = theora_process;
    stream->process_end = theora_end;

    stream->data = calloc(1, sizeof(misc_theora_info));
    info = stream->data;
    info->framenum_expected = -1;
}

static void vorbis_start(stream_processor *stream)
{
    misc_vorbis_info *info;

    stream->type = "vorbis";
    stream->process_page = vorbis_process;
    stream->process_end = vorbis_end;

    stream->data = calloc(1, sizeof(misc_vorbis_info));

    info = stream->data;

    vorbis_comment_init(&info->vc);
    vorbis_info_init(&info->vi);

}

static void kate_start(stream_processor *stream)
{
    misc_kate_info *info;

    stream->type = "kate";
    stream->process_page = kate_process;
    stream->process_end = kate_end;

    stream->data = calloc(1, sizeof(misc_kate_info));

    info = stream->data;

#ifdef HAVE_KATE
    kate_comment_init(&info->kc);
    kate_info_init(&info->ki);
#endif
}

static stream_processor *find_stream_processor(stream_set *set, ogg_page *page)
{
    ogg_uint32_t serial = ogg_page_serialno(page);
    int i;
    int invalid = 0;
    int constraint = 0;
    stream_processor *stream;

    for(i=0; i < set->used; i++) {
        if(serial == set->streams[i].serial) {
            /* We have a match! */
            stream = &(set->streams[i]);

            set->in_headers = 0;
            /* if we have detected EOS, then this can't occur here. */
            if(stream->end) {
                stream->isillegal = 1;
                stream->constraint_violated = CONSTRAINT_PAGE_AFTER_EOS;
                return stream;
            }

            stream->isnew = 0;
            stream->start = ogg_page_bos(page);
            stream->end = ogg_page_eos(page);
            stream->serial = serial;
            return stream;
        }
    }

    /* If there are streams open, and we've reached the end of the
     * headers, then we can't be starting a new stream.
     * XXX: might this sometimes catch ok streams if EOS flag is missing,
     * but the stream is otherwise ok?
     */
    if(streams_open(set) && !set->in_headers) {
        constraint = CONSTRAINT_MUXING_VIOLATED;
        invalid = 1;
    }

    set->in_headers = 1;

    if(set->allocated < set->used)
        stream = &set->streams[set->used];
    else {
        set->allocated += 5;
        set->streams = realloc(set->streams, sizeof(stream_processor)*
                set->allocated);
        stream = &set->streams[set->used];
    }
    set->used++;
    stream->num = set->used; /* We count from 1 */

    stream->isnew = 1;
    stream->isillegal = invalid;
    stream->constraint_violated = constraint;

    {
        int res;
        ogg_packet packet;

        /* We end up processing the header page twice, but that's ok. */
        ogg_stream_init(&stream->os, serial);
        ogg_stream_pagein(&stream->os, page);
        res = ogg_stream_packetout(&stream->os, &packet);
        if(res <= 0) {
            warn(_("WARNING: Invalid header page, no packet found\n"));
            null_start(stream);
        }
        else if(packet.bytes >= 7 && memcmp(packet.packet, "\x01vorbis", 7)==0)
            vorbis_start(stream);
        else if(packet.bytes >= 7 && memcmp(packet.packet, "\x80theora", 7)==0)
            theora_start(stream);
        else if(packet.bytes >= 8 && memcmp(packet.packet, "OggMIDI\0", 8)==0)
            other_start(stream, "MIDI");
        else if(packet.bytes >= 5 && memcmp(packet.packet, "\177FLAC", 5)==0)
            other_start(stream, "FLAC");
        else if(packet.bytes == 4 && memcmp(packet.packet, "fLaC", 4)==0)
            other_start(stream, "FLAC (legacy)");
        else if(packet.bytes >= 8 && memcmp(packet.packet, "Speex   ", 8)==0)
            other_start(stream, "speex");
        else if(packet.bytes >= 8 && memcmp(packet.packet, "fishead\0", 8)==0)
            other_start(stream, "skeleton");
        else if(packet.bytes >= 5 && memcmp(packet.packet, "BBCD\0", 5)==0)
            other_start(stream, "dirac");
        else if(packet.bytes >= 8 && memcmp(packet.packet, "KW-DIRAC", 8)==0)
            other_start(stream, "dirac (legacy)");
        else if(packet.bytes >= 8 && memcmp(packet.packet, "\x80kate\0\0\0", 8)==0)
            kate_start(stream);
        else
            other_start(stream, NULL);

        res = ogg_stream_packetout(&stream->os, &packet);
        if(res > 0) {
            warn(_("WARNING: Invalid header page in stream %d, "
                              "contains multiple packets\n"), stream->num);
        }

        /* re-init, ready for processing */
        ogg_stream_clear(&stream->os);
        ogg_stream_init(&stream->os, serial);
   }

   stream->start = ogg_page_bos(page);
   stream->end = ogg_page_eos(page);
   stream->serial = serial;

   if(stream->serial == 0 || stream->serial == -1) {
       info(_("Note: Stream %d has serial number %d, which is legal but may "
              "cause problems with some tools.\n"), stream->num, 
               stream->serial);
   }

   return stream;
}

static int get_next_page(FILE *f, ogg_sync_state *sync, ogg_page *page, 
        ogg_int64_t *written)
{
    int ret;
    char *buffer;
    int bytes;

    while((ret = ogg_sync_pageseek(sync, page)) <= 0) {
        if(ret < 0) {
            /* unsynced, we jump over bytes to a possible capture - we don't need to read more just yet */
            warn(_("WARNING: Hole in data (%d bytes) found at approximate offset %" I64FORMAT " bytes. Corrupted Ogg.\n"), -ret, *written);
            continue;
        }

        /* zero return, we didn't have enough data to find a whole page, read */
        buffer = ogg_sync_buffer(sync, CHUNK);
        bytes = fread(buffer, 1, CHUNK, f);
        if(bytes <= 0) {
            ogg_sync_wrote(sync, 0);
            return 0;
        }
        ogg_sync_wrote(sync, bytes);
        *written += bytes;
    }

    return 1;
}

static void process_file(char *filename) {
    FILE *file = fopen(filename, "rb");
    ogg_sync_state sync;
    ogg_page page;
    stream_set *processors = create_stream_set();
    int gotpage = 0;
    ogg_int64_t written = 0;

    if(!file) {
        error(_("Error opening input file \"%s\": %s\n"), filename,
                    strerror(errno));
        return;
    }

    printf(_("Processing file \"%s\"...\n\n"), filename);

    ogg_sync_init(&sync);

    while(get_next_page(file, &sync, &page, &written)) {
        stream_processor *p = find_stream_processor(processors, &page);
        gotpage = 1;

        if(!p) {
            error(_("Could not find a processor for stream, bailing\n"));
            return;
        }

        if(p->isillegal && !p->shownillegal) {
            char *constraint;
            switch(p->constraint_violated) {
                case CONSTRAINT_PAGE_AFTER_EOS:
                    constraint = _("Page found for stream after EOS flag");
                    break;
                case CONSTRAINT_MUXING_VIOLATED:
                    constraint = _("Ogg muxing constraints violated, new "
                                   "stream before EOS of all previous streams");
                    break;
                default:
                    constraint = _("Error unknown.");
            }

            warn(_("WARNING: illegally placed page(s) for logical stream %d\n"
                   "This indicates a corrupt Ogg file: %s.\n"), 
                    p->num, constraint);
            p->shownillegal = 1;
            /* If it's a new stream, we want to continue processing this page
             * anyway to suppress additional spurious errors
             */
            if(!p->isnew)
                continue;
        }

        if(p->isnew) {
            info(_("New logical stream (#%d, serial: %08x): type %s\n"), 
                    p->num, p->serial, p->type);
            if(!p->start)
                warn(_("WARNING: stream start flag not set on stream %d\n"),
                        p->num);
        }
        else if(p->start)
            warn(_("WARNING: stream start flag found in mid-stream "
                      "on stream %d\n"), p->num);

        if(p->seqno++ != ogg_page_pageno(&page)) {
            if(!p->lostseq) 
                warn(_("WARNING: sequence number gap in stream %d. Got page "
                       "%ld when expecting page %ld. Indicates missing data.\n"
                       ), p->num, ogg_page_pageno(&page), p->seqno - 1);
            p->seqno = ogg_page_pageno(&page);
            p->lostseq = 1;
        }
        else
            p->lostseq = 0;

        if(!p->isillegal) {
            p->process_page(p, &page);

            if(p->end) {
                if(p->process_end)
                    p->process_end(p);
                info(_("Logical stream %d ended\n"), p->num);
                p->isillegal = 1;
                p->constraint_violated = CONSTRAINT_PAGE_AFTER_EOS;
            }
        }
    }

    if(!gotpage)
        error(_("ERROR: No Ogg data found in file \"%s\".\n"
                "Input probably not Ogg.\n"), filename);

    free_stream_set(processors);

    ogg_sync_clear(&sync);

    fclose(file);
}

static void version (void) {
    printf (_("ogginfo from %s %s\n"), PACKAGE, VERSION);
}

static void usage(void) {
    version ();
    printf (_(" by the Xiph.Org Foundation (http://www.xiph.org/)\n\n"));
    printf(_("(c) 2003-2005 Michael Smith <msmith@xiph.org>\n"
             "\n"
             "Usage: ogginfo [flags] file1.ogg [file2.ogx ... fileN.ogv]\n"
             "Flags supported:\n"
             "\t-h Show this help message\n"
             "\t-q Make less verbose. Once will remove detailed informative\n"
             "\t   messages, two will remove warnings\n"
             "\t-v Make more verbose. This may enable more detailed checks\n"
             "\t   for some stream types.\n"));
    printf (_("\t-V Output version information and exit\n"));
}

int main(int argc, char **argv) {
    int f, ret;

    setlocale(LC_ALL, "");
    bindtextdomain(PACKAGE, LOCALEDIR);
    textdomain(PACKAGE);

    if(argc < 2) {
        fprintf(stdout, 
                _("Usage: ogginfo [flags] file1.ogg [file2.ogx ... fileN.ogv]\n"
                  "\n"
                  "ogginfo is a tool for printing information about Ogg files\n"
                  "and for diagnosing problems with them.\n"
                  "Full help shown with \"ogginfo -h\".\n"));
        exit(1);
    }

    while((ret = getopt(argc, argv, "hqvV")) >= 0) {
        switch(ret) {
            case 'h':
                usage();
                return 0;
            case 'V':
                version();
                return 0;
            case 'v':
                verbose++;
                break;
            case 'q':
                verbose--;
                break;
        }
    }

    if(verbose > 1)
        printlots = 0;
    if(verbose < 1)
        printinfo = 0;
    if(verbose < 0) 
        printwarn = 0;

    if(optind >= argc) {
        fprintf(stderr, 
                _("No input files specified. \"ogginfo -h\" for help\n"));
        return 1;
    }

    ret = 0;

    for(f=optind; f < argc; f++) {
        flawed = 0;
        process_file(argv[f]);
        if(flawed != 0)
            ret = flawed;
    }

    return ret;
}
