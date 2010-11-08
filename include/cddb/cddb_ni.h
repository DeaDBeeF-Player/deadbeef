/*
    $Id: cddb_ni.h,v 1.32 2009/03/01 03:28:07 jcaratzas Exp $

    Copyright (C) 2003, 2004, 2005 Kris Verbeeck <airborne@advalvas.be>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the
    Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA  02111-1307, USA.
*/

#ifndef CDDB_NI_H
#define CDDB_NI_H 1

#ifdef __cplusplus
    extern "C" {
#endif


#if HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_ICONV_H
#  include <iconv.h>
#else
   typedef void *iconv_t;       /* for code uniformity */
#endif

#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif

#ifdef HAVE_WINSOCK2_H
#include <winsock2.h>
#ifndef ETIMEDOUT
#define ETIMEDOUT   WSAETIMEDOUT
#endif
#ifndef EWOULDBLOCK
#define EWOULDBLOCK WSAEWOULDBLOCK
#endif
#ifndef EINPROGRESS
#define EINPROGRESS WSAEINPROGRESS
#endif
#endif

#include "cddb/cddb_regex.h"
#include "cddb/cddb.h"
#include "cddb/cddb_conn_ni.h"
#include "cddb/cddb_net.h"
#include "cddb/cddb_cmd_ni.h"
#include "cddb/cddb_log_ni.h"


#define FALSE          0
#define TRUE           1

#define CHR_CR         '\r'
#define CHR_LF         '\n'
#define CHR_EOS        '\0'
#define CHR_SPACE      ' '
#define CHR_DOT        '.'

#define DEFAULT_BUF_SIZE 1024

#define CLIENT_NAME    PACKAGE
#define CLIENT_VERSION VERSION

#define DEFAULT_USER        "anonymous"
#define DEFAULT_HOST        "localhost"
#define DEFAULT_SERVER      "freedb.org"
#define DEFAULT_PORT        888
#define DEFAULT_TIMEOUT     10
#define DEFAULT_PATH_QUERY  "/~cddb/cddb.cgi"
#define DEFAULT_PATH_SUBMIT "/~cddb/submit.cgi"
#define DEFAULT_CACHE       ".cddbslave"
#define DEFAULT_PROXY_PORT  8080

#define DEFAULT_PROTOCOL_VERSION 6
#define SERVER_CHARSET           "UTF8"


#define FREE_NOT_NULL(p) if (p) { free(p); p = NULL; }
#define CONNECTION_OK(c) (c->socket != -1)
#define STR_OR_NULL(s) ((s) ? s : "NULL")
#define STR_OR_EMPTY(s) ((s) ? s : "")

#define RETURN_STR_OR_EMPTY(s) \
            return (!s && (libcddb_flags() & CDDB_F_EMPTY_STR)) ? "" : s

#define ASSERT(cond, error) \
            if (!(cond)) { return error; }
#define ASSERT_NOT_NULL(ptr) \
            ASSERT(ptr!=NULL, CDDB_ERR_INVALID)
#define ASSERT_RANGE(num,lo,hi) \
            ASSERT((num>=lo)&&(num<=hi), CDDB_ERR_INVALID)


/* --- type definitions */


/** Actual definition of track structure. */
struct cddb_track_s
{
    int num;                    /**< track number on the disc */
    int frame_offset;           /**< frame offset of the track on the disc */
    int length;                 /**< track length in seconds */
    char *title;                /**< track title */
    char *artist;               /**< (optional) track artist */
    char *ext_data;             /**< (optional) extended disc data */
    struct cddb_track_s *prev;  /**< pointer to previous track, or NULL */
    struct cddb_track_s *next;  /**< pointer to next track, or NULL */
    struct cddb_disc_s *disc;   /**< disc of which this is a track */
};

/** Actual definition of disc structure. */
struct cddb_disc_s
{
    unsigned int revision;      /**< revision number */
    unsigned int discid;        /**< four byte disc ID */
    cddb_cat_t category;        /**< CDDB category */
    char *genre;                /**< disc genre */
    char *title;                /**< disc title */
    char *artist;               /**< disc artist */
    unsigned int length;        /**< disc length in seconds */
    unsigned int year;          /**< (optional) disc year YYYY */
    char *ext_data;             /**< (optional) extended disc data  */
    int track_cnt;              /**< number of tracks on the disc */
    cddb_track_t *tracks;       /**< pointer to the first track */
    cddb_track_t *iterator;     /**< track iterator */
};


/* --- global variables */


/** Server connection used especially for text searches. */
extern cddb_conn_t *cddb_search_conn;


/* --- non-exported function prototypes */


unsigned int libcddb_flags(void);

/**
 * Convert a string to a new character encoding according to the given
 * conversion descriptor.
 */
int cddb_str_iconv(iconv_t cd, ICONV_CONST char *in, char **out);

/**
 * Converts all disc and track strings to user character encoding.
 */
int cddb_disc_iconv(iconv_t cd, cddb_disc_t *disc);

/**
 * Converts all track strings to user character encoding.
 */
int cddb_track_iconv(iconv_t cd, cddb_track_t *track);

/**
 * Converts all site strings to user character encoding.
 */
int cddb_site_iconv(iconv_t cd, cddb_site_t *site);

/**
 * Base64 encode the source string and write it to the destination
 * buffer.  The destination buffer should be large enough (= 4/3 of
 * src string length).
 */
void cddb_b64_encode(char *dst, const char *src);


#ifdef __cplusplus
    }
#endif

#endif /* CDDB_NI_H */
