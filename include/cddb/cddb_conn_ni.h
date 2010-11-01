/*
    $Id: cddb_conn_ni.h,v 1.14 2005/08/03 18:27:19 airborne Exp $

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

#ifndef CDDB_CONN_NI_H
#define CDDB_CONN_NI_H 1

#ifdef __cplusplus
    extern "C" {
#endif


#include "cddb_ni.h"
#include "ll.h"


/* --- type definitions */


/** Actual definition of iconv structure. */
struct cddb_iconv_s
{
    iconv_t cd_to_freedb;       /**< character set conversion descriptor for
                                     converting from user to FreeDB format */
    iconv_t cd_from_freedb;     /**< character set conversion descriptor for
                                     converting from FreeDB to user format */
};

/** Actual definition of serach parameters structure. */
typedef struct cddb_search_params_s
{
    unsigned int fields;        /**< fields to search (cddb_search_t
                                     bit string) */
    unsigned int cats;          /**< categories to search (cddb_cat_t
                                     bit string) */
} cddb_search_params_t;

/** Actual definition of connection structure. */
struct cddb_conn_s 
{
    unsigned int buf_size;      /**< maximum line/buffer size, defaults to 1024
                                     (see DEFAULT_BUF_SIZE) */
    char *line;                 /**< last line read */

    int is_connected;           /**< are we already connected to the server? */
    struct sockaddr_in sa;      /**< the socket address structure for
                                     connecting to the CDDB server */
    int socket;                 /**< the socket file descriptor */
    char *server_name;          /**< host name of the CDDB server, defaults
                                     to 'freedb.org' (see DEFAULT_SERVER) */
    int server_port;            /**< port of the CDDB server, defaults to 888 
                                     (see DEFAULT_PORT) */
    int timeout;                /**< time out interval (in seconds) used during
                                     network operations, defaults to 10 seconds
                                     (see DEFAULT_TIMEOUT) */

    char *http_path_query;      /**< URL for querying the server through HTTP,
                                     defaults to /~cddb/cddb.cgi'
                                     (see DEFAULT_PATH_QUERY) */
    char *http_path_submit;     /**< URL for submitting to the server through HTTP,
                                     defaults to /~cddb/submit.cgi'
                                     (see DEFAULT_PATH_SUBMIT) */
    int is_http_enabled;        /**< use HTTP, disabled by default */

    int is_http_proxy_enabled;  /**< use HTTP through a proxy server,
                                     disabled by default */
    char *http_proxy_server;    /**< host name of the HTTP proxy server */
    int http_proxy_server_port; /**< port of the HTTP proxy server,
                                     defaults to 8080 (see DEFAULT_PROXY_PORT) */
    char *http_proxy_username;  /**< HTTP proxy user name */
    char *http_proxy_password;  /**< HTTP proxy password */
    char *http_proxy_auth;      /**< Base64 encoded username:password */

    FILE *cache_fp;             /**< a file pointer to a cached CDDB entry or
                                     NULL if no cached version is available */
    cddb_cache_mode_t use_cache;/**< field to specify local CDDB cache behaviour, 
                                     enabled by default (CACHE_ON) */
    char *cache_dir;            /**< CDDB slave cache, defaults to 
                                     '~/.cddbslave' (see DEFAULT_CACHE) */
    int cache_read;             /**< read data from cached file instead of
                                     from the network */

    char *cname;                /**< name of the client program, 'libcddb' by
                                     default */
    char *cversion;             /**< version of the client program, current 
                                     libcddb version by default */
    char *user;                 /**< user name supplied to CDDB server, defaults
                                     to the value of the 'USER' environment 
                                     variable or 'anonymous' if undefined */
    char *hostname;             /**< host name of the local machine, defaults
                                     to the value of the 'HOSTNAME' environment
                                     variable or 'localhost' if undefined */

    cddb_error_t errnum;        /**< error number of last CDDB command */

    list_t *query_data;         /**< list to keep CDDB query results */
    list_t *sites_data;         /**< list to keep FreeDB mirror sites */
    cddb_search_params_t srch;  /**< parameters for text search */

    cddb_iconv_t charset;       /**< character set conversion settings */
};


/* --- getters & setters --- */


#define cddb_cache_file(c) (c)->cache_fp


/* --- connecting / disconnecting --- */


int cddb_connect(cddb_conn_t *c);

void cddb_disconnect(cddb_conn_t *c);


/* --- miscellaneous --- */


/**
 * Clone proxy settings from source connection to destinaton
 * connection.
 */
void cddb_clone_proxy(cddb_conn_t *dst, cddb_conn_t *src);


/* --- error handling --- */


/**
 * Set the error number for the last libcddb command.
 *
 * @param c The CDDB connection structure.
 * @param n The error number
 */
#define cddb_errno_set(c, n) (c)->errnum = n

/**
 * Set the error number for the last libcddb command.  If this number
 * is different from CDDB_ERR_OK, a message is also logged with the
 * level specified.
 *
 * @param c The CDDB connection structure.
 * @param n The error number
 * @param l The log level
 */
#define cddb_errno_log(c, n, l) cddb_errno_set(c, n); cddb_log(l, cddb_error_str(n))

#define cddb_errno_log_debug(c, n) cddb_errno_log(c, n, CDDB_LOG_DEBUG)
#define cddb_errno_log_info(c, n) cddb_errno_log(c, n, CDDB_LOG_INFO)
#define cddb_errno_log_warn(c, n) cddb_errno_log(c, n, CDDB_LOG_WARN)
#define cddb_errno_log_error(c, n) cddb_errno_log(c, n, CDDB_LOG_ERROR)
#define cddb_errno_log_crit(c, n) cddb_errno_log(c, n, CDDB_LOG_CRITICAL)


#ifdef __cplusplus
    }
#endif

#endif /* CDDB_CONN_NI_H */
