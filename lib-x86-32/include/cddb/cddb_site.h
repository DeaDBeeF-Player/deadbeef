/*
    $Id: cddb_site.h,v 1.3 2005/06/15 16:08:28 airborne Exp $

    Copyright (C) 2005 Kris Verbeeck <airborne@advalvas.be>

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

#ifndef CDDB_SITE_H
#define CDDB_SITE_H 1

#ifdef __cplusplus
    extern "C" {
#endif


#include "cddb/cddb_error.h"


/* --- type and structure definitions */


/**
 * Enumeration defining the CDDB protocol supported by a specific
 * site.
 */
typedef enum {
    PROTO_UNKNOWN = 0,          /**< Unknown protocol */
    PROTO_CDDBP,                /**< FreeDB custom protocol */
    PROTO_HTTP                  /**< Command tunneling over HTTP */
} cddb_protocol_t;

/**
 * The CDDB site structure.  Contains all information about one
 * particular CDDB server.
 */
typedef struct cddb_site_s cddb_site_t;


/* --- construction / destruction */


/**
 * Creates a new CDDB site structure.
 *
 * @return The CDDB site structure or NULL if memory allocation failed.
 */
cddb_site_t *cddb_site_new(void);

/**
 * Free all resources associated with the given CDDB site structure.
 *
 * @param site The CDDB site structure.
 */
cddb_error_t cddb_site_destroy(cddb_site_t *site);

/**
 * Creates a clone of the given site.
 *
 * @param site The CDDB site structure.
 */
cddb_site_t *cddb_site_clone(cddb_site_t *site);


/* --- setters / getters --- */


/**
 * Get the site's address.
 *
 * @param site The CDDB site structure.
 * @param address The address of the server upon returning.
 * @param port The port of the server upon returning.
 * @return Error code: CDDB_ERR_OK or CDDB_ERR_INVALID.
 */
cddb_error_t cddb_site_get_address(const cddb_site_t *site,
                                   const char **address, unsigned int *port);

/**
 * Set the site's address.  A copy of the address string is made.  So the caller
 * should free any memory associated with the input parameter.
 *
 * @param site The CDDB site structure.
 * @param address The address of the server.
 * @param port The port of the server.
 * @return Error code: CDDB_ERR_OK, CDDB_ERR_INVALID or CDDB_ERR_OUT_OF_MEMORY.
 */
cddb_error_t cddb_site_set_address(cddb_site_t *site,
                                   const char *address, unsigned int port);

/**
 * Get the protocol used by the site.
 *
 * @see cddb_protocol_t
 *
 * @param site The CDDB site structure.
 * @return The protocol.
 */
cddb_protocol_t cddb_site_get_protocol(const cddb_site_t *site);

/**
 * Set the protocol used by the site.
 *
 * @see cddb_protocol_t
 *
 * @param site The CDDB site structure.
 * @param proto The protocol.
 * @return Error code: CDDB_ERR_OK or CDDB_ERR_INVALID.
 */
cddb_error_t cddb_site_set_protocol(cddb_site_t *site, cddb_protocol_t proto);

/**
 * Get the query path in case the HTTP protocol is used.
 *
 * @param site The CDDB site structure.
 * @param path The query path upon returning.
 * @return Error code: CDDB_ERR_OK or CDDB_ERR_INVALID.
 */
cddb_error_t cddb_site_get_query_path(const cddb_site_t *site,
                                      const char **path);

/**
 * Set the query path in case the HTTP protocol is used.  A copy of the path
 * string is made.  So the caller should free any memory associated with the
 * input parameter.
 *
 * @param site The CDDB site structure.
 * @param path The query path.  A value of NULL deletes the current path.
 * @return Error code: CDDB_ERR_OK, CDDB_ERR_INVALID or CDDB_ERR_OUT_OF_MEMORY.
 */
cddb_error_t cddb_site_set_query_path(cddb_site_t *site, const char *path);

/**
 * Get the submit path in case the HTTP protocol is used.
 *
 * @param site The CDDB site structure.
 * @param path The submit path upon returning.
 * @return Error code: CDDB_ERR_OK or CDDB_ERR_INVALID.
 */
cddb_error_t cddb_site_get_submit_path(const cddb_site_t *site,
                                       const char **path);

/**
 * Set the submit path in case the HTTP protocol is used.  A copy of the path
 * string is made.  So the caller should free any memory associated with the
 * input parameter.
 *
 * @param site The CDDB site structure.
 * @param path The query path.  A value of NULL deletes the current path.
 * @return Error code: CDDB_ERR_OK, CDDB_ERR_INVALID or CDDB_ERR_OUT_OF_MEMORY.
 */
cddb_error_t cddb_site_set_submit_path(cddb_site_t *site, const char *path);

/**
 * Get the site's location.
 *
 * @param site The CDDB site structure.
 * @param latitude Will contain the server's latitude upon returning.
 *                 A positive number is used for the northern
 *                 hemisphere, a negative one for the southern
 *                 hemisphere.
 * @param longitude Will contain the server's longitude upon returning.
 *                  A positive number is used for the eastern
 *                  hemisphere, a negative one for the western
 *                  hemisphere.
 * @return Error code: CDDB_ERR_OK or CDDB_ERR_INVALID.
 */
cddb_error_t cddb_site_get_location(const cddb_site_t *site,
                                    float *latitude, float *longitude);

/**
 * Set the site's location.
 *
 * @param site The CDDB site structure.
 * @param latitude The server's latitude.  Use a positive number for the
 *                 northern hemisphere, a negative one for the southern
 *                 hemisphere.
 * @param longitude The server's longitude.  Use a positive number for the
 *                  eastern hemisphere, a negative one for the western
 *                  hemisphere.
 * @return Error code: CDDB_ERR_OK or CDDB_ERR_INVALID.
 */
cddb_error_t cddb_site_set_location(cddb_site_t *site,
                                    float latitude, float longitude);

/**
 * Get a description of the site.
 *
 * @param site The CDDB site structure.
 * @param desc The description upon returning.
 * @return Error code: CDDB_ERR_OK or CDDB_ERR_INVALID.
 */
cddb_error_t cddb_site_get_description(const cddb_site_t *site,
                                       const char **desc);

/**
 * Set a description for the site.  A copy of the description string is made.
 * So the caller should free any memory associated with the input parameter.
 *
 * @param site The CDDB site structure.
 * @param desc The description.  A value of NULL deletes the current
 *             description.
 * @return Error code: CDDB_ERR_OK, CDDB_ERR_INVALID or CDDB_ERR_OUT_OF_MEMORY.
 */
cddb_error_t cddb_site_set_description(cddb_site_t *site, const char *desc);


/* --- miscellaneous */


/**
 * Parses one line of data as returned by the sites command and
 * populates the given structure.
 *
 * @param site The CDDB site structure.
 * @param line The result line.
 * @return True in case of success or false on failure.
 */
int cddb_site_parse(cddb_site_t *site, const char *line);

/**
 * Prints information about the site on stdout.  This is just a
 * debugging routine to display the structure's content.
 *
 * @param site The CDDB site structure.
 * @return Error code: CDDB_ERR_OK or CDDB_ERR_INVALID.
 */
cddb_error_t cddb_site_print(const cddb_site_t *site);


#ifdef __cplusplus
    }
#endif

#endif /* CDDB_SITE_H */
