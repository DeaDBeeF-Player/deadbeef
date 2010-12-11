/*
    $Id: cddb_conn.h,v 1.31 2009/03/01 03:28:07 jcaratzas Exp $

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

#ifndef CDDB_CONN_H
#define CDDB_CONN_H 1

#ifdef __cplusplus
    extern "C" {
#endif


#include <stdio.h>
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#include "cddb/cddb_site.h"


typedef enum {
    CACHE_OFF = 0,              /**< do not use local CDDB cache, network
                                     only */
    CACHE_ON,                   /**< use local CDDB cache, if possible */
    CACHE_ONLY                  /**< only use local CDDB cache, no network
                                     access */
} cddb_cache_mode_t;

/**
 * Forward declaration of opaque structure used for character set
 * conversions.
 */
typedef struct cddb_iconv_s *cddb_iconv_t;

/**
 * An opaque structure for keeping state about the connection to a
 * CDDB server.
 */
typedef struct cddb_conn_s cddb_conn_t;

/**
 * Which fields to use for the full text search is defined by one or
 * more of the constants below.
 */
typedef enum {
    SEARCH_NONE = 0,            /**< no fields */
    SEARCH_ARTIST = 1,          /**< artist name field */
    SEARCH_TITLE = 2,           /**< disc title field */
    SEARCH_TRACK = 4,           /**< track title field */
    SEARCH_OTHER = 8,           /**< other fields */
    SEARCH_ALL = ~0,            /**< all fields */
} cddb_search_t;

/**
 * Macro to be used for building the category search bit-string from
 * the values of #cddb_cat_t.
 */
#define SEARCHCAT(c) (1 << (c))


/* --- construction / destruction --- */


/**
 * Creates a new CDDB connection structure.  This structure will have
 * to be passed to all libcddb functions.  Default values will be used
 * for the connection parameters allowing it to contact the CDDB
 * server at freedb.org.
 *
 * @return The CDDB connection structure or NULL if something went wrong.
 */
cddb_conn_t *cddb_new(void);

/**
 * Free all resources associated with the given CDDB connection
 * structure.
 */
void cddb_destroy(cddb_conn_t *c);


/* --- getters & setters --- */


/**
 * Set the character set.  By default the FreeDB server uses UTF-8 when
 * providing CD data.  When a character set is defined with this function
 * any strings retrieved from or sent to the server will automatically be
 * converted.
 *
 * @param c The connection structure.
 * @param cs The character set that will be used.
 * @return False if the specified character set is unknown, or no conversion
 *         from/to UTF-8 is available.  True otherwise.
 */
int cddb_set_charset(cddb_conn_t *c, const char *cs);

/**
 * Change the size of the internal buffer.
 *
 * @param c The connection structure.
 * @param size The new buffer size.
 */
void cddb_set_buf_size(cddb_conn_t *c, unsigned int size);

/**
 * Set all server details in one go through the use of a site structure.  This
 * function initializzes the server address, port, protocol and query path in
 * case of HTTP.
 *
 * @see cddb_sites
 * @see cddb_first_site
 * @see cddb_next_site
 *
 * @param c The connection structure.
 * @param site The site to use.
 * @return Error code: CDDB_ERR_OK or CDDB_ERR_INVALID.
 */
cddb_error_t cddb_set_site(cddb_conn_t *c, const cddb_site_t *site);

/**
 * Get the host name of the CDDB server that is currently being used.
 *
 * @see cddb_set_server_name
 *
 * @param c The connection structure.
 * @return The server host name.
 */
const char *cddb_get_server_name(const cddb_conn_t *c);

/**
 * Set the host name of the CDDB server.  The default value for the
 * server is 'freedb.org'.
 *
 * @see cddb_get_server_name
 *
 * @param c      The connection structure.
 * @param server The server host name.
 */
void cddb_set_server_name(cddb_conn_t *c, const char *server);

/**
 * Get the port of the CDDB server that is currently being used.
 *
 * @see cddb_set_server_port
 *
 * @param c The connection structure.
 * @return The server port.
 */
unsigned int cddb_get_server_port(const cddb_conn_t *c);

/**
 * Set the port of the CDDB server.  The default value is 888.
 *
 * @see cddb_get_server_port
 *
 * @param c    The connection structure.
 * @param port The server port.
 */
void cddb_set_server_port(cddb_conn_t *c, int port);

/**
 * Get the network time out value (in seconds).
 *
 * @see cddb_set_timeout
 *
 * @param c The connection structure.
 * @return The current time out in seconds.
 */
unsigned int cddb_get_timeout(const cddb_conn_t *c);

/**
 * Set the network time out value (in seconds).  The default is 10
 * seconds.
 *
 * @see cddb_get_timeout
 *
 * @param c The connection structure.
 * @param t The new time out in seconds.
 */
void cddb_set_timeout(cddb_conn_t *c, unsigned int t);

/**
 * Get the URL path for querying a CDDB server through HTTP.
 *
 * @see cddb_set_http_path_query
 *
 * @param c The connection structure.
 * @return The URL path.
 */
const char *cddb_get_http_path_query(const cddb_conn_t *c);

/**
 * Set the URL path for querying a CDDB server through HTTP.  The
 * default value is '/~cddb/cddb.cgi'.
 *
 * @see cddb_get_http_path_query
 *
 * @param c    The connection structure.
 * @param path The URL path.
 */
void cddb_set_http_path_query(cddb_conn_t *c, const char *path);

/**
 * Get the URL path for submitting to a CDDB server through HTTP.
 *
 * @see cddb_set_http_path_submit
 *
 * @param c The connection structure.
 * @return The URL path.
 */
const char *cddb_get_http_path_submit(const cddb_conn_t *c);

/**
 * Set the URL path for submitting to a CDDB server through HTTP.  The
 * default value is '/~cddb/submit.cgi'.
 *
 * @see cddb_get_http_path_submit
 *
 * @param c The connection structure.
 * @param path The URL path.
 */
void cddb_set_http_path_submit(cddb_conn_t *c, const char *path);

/**
 * Returns true if the HTTP protocol is currently enabled and false if
 * CDDBP is enabled.
 *
 * @see cddb_http_enable
 * @see cddb_http_disable
 *
 * @param c The CDDB connection structure.
 * @return True or false.
 */
unsigned int cddb_is_http_enabled(const cddb_conn_t *c);

/**
 * Enable HTTP tunneling to connect to the CDDB server.  By default
 * this option is disabled.
 *
 * @see cddb_is_http_enabled
 * @see cddb_http_disable
 *
 * @param c The CDDB connection structure.
 */
void cddb_http_enable(cddb_conn_t *c);

/**
 * Disable HTTP tunneling to connect to the CDDB server.  By default this
 * option is disabled.
 *
 * @see cddb_is_http_enabled
 * @see cddb_http_enable
 *
 * @param c The CDDB connection structure.
 */
void cddb_http_disable(cddb_conn_t *c);

/**
 * Returns true if the proxy support is currently enabled and false if
 * it is not.  This fucntion does not check whether HTTP is enabled.
 * So it is possible that true will be returned while in reality the
 * CDDBP protocol is being used (no proxy support).
 *
 * @see cddb_http_proxy_enable
 * @see cddb_http_proxy_disable
 *
 * @param c The CDDB connection structure.
 * @return True or false.
 */
unsigned int cddb_is_http_proxy_enabled(const cddb_conn_t *c);

/**
 * Enable HTTP tunneling through an HTTP proxy server to connect to
 * the CDDB server.  The usage of an HTTP proxy implies normal HTTP
 * tunneling instead of connecting directly to the CDDB server.  By
 * default this option is disabled.
 *
 * @see cddb_is_http_proxy_enabled
 * @see cddb_http_proxy_disable
 *
 * @param c The CDDB connection structure.
 */
void cddb_http_proxy_enable(cddb_conn_t *c);

/**
 * Disable HTTP tunneling through an HTTP proxy server to connect to
 * the CDDB server.  By default this option is disabled.
 *
 * @see cddb_is_http_proxy_enabled
 * @see cddb_http_proxy_enable
 *
 * @param c The CDDB connection structure.
 */
void cddb_http_proxy_disable(cddb_conn_t *c);

/**
 * Get the host name of the HTTP proxy server.
 *
 * @see cddb_set_http_proxy_server_name
 *
 * @param c The connection structure.
 * @return The proxy server host name.
 */
const char *cddb_get_http_proxy_server_name(const cddb_conn_t *c);

/**
 * Set the host name of the HTTP proxy server.  There is no default
 * value.
 *
 * @see cddb_get_http_proxy_server_name
 *
 * @param c      The connection structure.
 * @param server The server host name.
 */
void cddb_set_http_proxy_server_name(cddb_conn_t *c, const char *server);

/**
 * Get the port of the HTTP proxy server.
 *
 * @see cddb_set_http_proxy_server_port
 *
 * @param c The connection structure.
 * @return The proxy server port.
 */
unsigned int cddb_get_http_proxy_server_port(const cddb_conn_t *c);

/**
 * Set the port of the HTTP proxy server.  The default value is 8080.
 *
 * @see cddb_get_http_proxy_server_port
 *
 * @param c    The connection structure.
 * @param port The server port.
 */
void cddb_set_http_proxy_server_port(cddb_conn_t *c, int port);

/**
 * Set the HTTP proxy user name which is used when Basic Authentication
 * is required.
 *
 * @param c        The connection structure.
 * @param username The user name.
 */
void cddb_set_http_proxy_username(cddb_conn_t* c, const char* username);

/**
 * Get the HTTP proxy user name.
 *
 * @param c The connection structure.
 * @return The user name.
 */
const char *cddb_get_http_proxy_username(const cddb_conn_t *c);

/**
 * Set the HTTP proxy password which is used when Basic Authentication
 * is required.
 *
 * @param c      The connection structure.
 * @param passwd The password.
 */
void cddb_set_http_proxy_password(cddb_conn_t* c, const char* passwd);

/**
 * Get the HTTP proxy password. 
 *
 * @param c The connection structure.
 * @return The password.
 */
const char *cddb_get_http_proxy_password(const cddb_conn_t *c);

/**
 * Set the HTTP proxy user name and password in one go.  These
 * credentials are used when Basic Authentication is required.  The
 * advantage of using this function over setting the user name and
 * password seperately is that the cleartext user name and password
 * are not kept in memory longer than needed.
 *
 * @param c        The connection structure.
 * @param username The user name.
 * @param passwd   The password.
 */
void cddb_set_http_proxy_credentials(cddb_conn_t* c,
                                     const char *username, const char* passwd);

/**
 * Get the error number returned by the last libcddb command.
 *
 * @param c The CDDB connection structure.
 * @return The error number.
 */
cddb_error_t cddb_errno(const cddb_conn_t *c);

/**
 * Set the name and version of the client program overwriting the
 * previous values.  This function will make a copy of the provided
 * strings.  The defaults are 'libcddb' and the version number of the
 * libcddb library in use.  Both parameters must be valid strings.  If
 * any of teh strings is NULL, this fucntion will return without
 * changing anything.
 *
 * @param c        The connection structure.
 * @param cname    The name of the client program.
 * @param cversion The version number of the client program.
 */
void cddb_set_client(cddb_conn_t *c, const char *cname, const char *cversion);

/**
 * Sets the user name and host name of the local machine.  This
 * function will parse out the user name and host name from the e-mail
 * address.
 *
 * @param c     The connection structure.
 * @param email The e-mail address of the user.
 */
int cddb_set_email_address(cddb_conn_t *c, const char *email);

/**
 * Returns the current cache mode.  This can be either on, off or
 * cache only.
 *
 * @see CACHE_ON
 * @see CACHE_ONLY
 * @see CACHE_OFF
 * @see cddb_cache_enable
 * @see cddb_cache_only
 * @see cddb_cache_disable
 *
 * @param c The connection structure.
 */
cddb_cache_mode_t cddb_cache_mode(const cddb_conn_t *c);

/**
 * Enable caching of CDDB entries locally.  Caching is enabled by
 * default.  The cache directory can be changed with the
 * cddb_cache_set_dir function.
 *
 * @see cddb_cache_mode
 * @see cddb_cache_disable
 * @see cddb_cache_only
 *
 * @param c The connection structure.
 */
void cddb_cache_enable(cddb_conn_t *c);

/**
 * Only use the local CDDB cache.  Never contact a server to retrieve
 * any data.  The cache directory can be changed with the
 * cddb_cache_set_dir function.
 *
 * @see cddb_cache_mode
 * @see cddb_cache_enable
 * @see cddb_cache_disable
 *
 * @param c The connection structure.
 */
void cddb_cache_only(cddb_conn_t *c);

/**
 * Disable caching of CDDB entries locally.  All data will be fetched
 * from a CDDB server everytime and the retrieved data will not be
 * cached locally.
 *
 * @see cddb_cache_mode
 * @see cddb_cache_enable
 * @see cddb_cache_only
 *
 * @param c The connection structure.
 */
void cddb_cache_disable(cddb_conn_t *c);

/**
 * Return the directory currently being used for caching.
 *
 * @see cddb_cache_set_dir
 *
 * @param c The connection structure.
 * @return The directory being used for caching.
 */
const char *cddb_cache_get_dir(const cddb_conn_t *c);

/**
 * Change the directory used for caching CDDB entries locally.  The
 * default location of the cached entries is a subdirectory
 * (.cddbslave) of the user's home directory.  If the first character
 * of the directory is '~', then it will be expanded to the contents
 * of $HOME.
 *
 * @see cddb_cache_get_dir
 *
 * @param c   The connection structure.
 * @param dir The directory to use for caching.
 */
int cddb_cache_set_dir(cddb_conn_t *c, const char *dir);

/**
 * Retrieve the first CDDB mirror site.
 *
 * @param c The connection structure.
 * @return The first mirror site or NULL if not found.
 */
const cddb_site_t *cddb_first_site(cddb_conn_t *c);

/**
 * Retrieve the next CDDB mirror site.
 *
 * @param c The connection structure.
 * @return The next mirror site or NULL if not found.
 */
const cddb_site_t *cddb_next_site(cddb_conn_t *c);

/**
 * Set the bit-string specifying which fields to examine when
 * performing a text search.  By default only the artist and disc
 * title fields are searched.
 *
 * @param c The connection structure.
 * @param fields A bitwise ORed set of values from #cddb_search_t.
 */
void cddb_search_set_fields(cddb_conn_t *c, unsigned int fields);

/**
 * Set the bit-string specifying which categories to examine when
 * performing a text search.  The #SEARCHCAT macro needs to be used to
 * build the actual bit-string from individual categories.  The
 * #cddb_search_t values #SEARCH_NONE and #SEARCH_ALL are also valid.
 * The example below shows some possible combinations.  By default all
 * categories are searched.
 *
 * @code
 * unsigned int cats = SEARCHCAT(CDDB_CAT_ROCK) | SEARCHCAT(CDDB_CAT_MISC);
 * unsigned int cats = SEARCH_ALL;
 * unsigned int cats = SEARCH_NONE;
 * @endcode
 *
 * @param c The connection structure.
 * @param cats A bitwise ORed set of values from #SEARCHCAT(#cddb_cat_t).
 */
void cddb_search_set_categories(cddb_conn_t *c, unsigned int cats);


#ifdef __cplusplus
    }
#endif

#endif /* CDDB_CONN_H */
