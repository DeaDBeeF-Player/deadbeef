/*
    $Id: cddb_error.h,v 1.12 2005/05/29 08:11:04 airborne Exp $

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

#ifndef CDDB_ERROR_H
#define CDDB_ERROR_H 1

#ifdef __cplusplus
    extern "C" {
#endif


#include <stdio.h>


/**
 * A list of error codes returned by various libcddb functions.
 */
typedef enum {

    CDDB_ERR_OK = 0,            /**< no error occurred */

    /* --- general errors --- */

    CDDB_ERR_OUT_OF_MEMORY,     /**< out of memory */
    CDDB_ERR_LINE_SIZE,         /**< internal buffer too small */
    CDDB_ERR_NOT_IMPLEMENTED,   /**< feature not (yet) implemented */
    CDDB_ERR_UNKNOWN,           /**< problem unknown */

    /* --- connection errors --- */

    CDDB_ERR_SERVER_ERROR,      /**< CDDB server error */
    CDDB_ERR_UNKNOWN_HOST_NAME, /**< unknown host name */ 
    CDDB_ERR_CONNECT,           /**< connection error */
    CDDB_ERR_PERMISSION_DENIED, /**< permission denied */
    CDDB_ERR_NOT_CONNECTED,     /**< not yet connected or connection
                                     has been closed */

    /* --- response parsing errors --- */

    CDDB_ERR_UNEXPECTED_EOF,    /**< unexpected end-of-file encountered */
    CDDB_ERR_INVALID_RESPONSE,  /**< invalid response data */
    CDDB_ERR_DISC_NOT_FOUND,    /**< no results found */

    /* --- library errors --- */

    CDDB_ERR_DATA_MISSING,      /**< some data is missing for executing
                                     a certain command */
    CDDB_ERR_TRACK_NOT_FOUND,   /**< specified track is not present */
    CDDB_ERR_REJECTED,          /**< posted data rejected */
    CDDB_ERR_EMAIL_INVALID,     /**< the e-mail address used when
                                     submitting is invalid */

    CDDB_ERR_INVALID_CHARSET,   /**< invalid character set or unsupported
                                     conversion */
    CDDB_ERR_ICONV_FAIL,        /**< character set conversion failed */

    /* --- new errors added to back of list for backward compatibility --- */

    CDDB_ERR_PROXY_AUTH,        /**< proxy authentication failed */
    CDDB_ERR_INVALID,           /**< invalid input parameter(s) */

    /* --- terminator --- */

    CDDB_ERR_LAST
} cddb_error_t;


/* --- error handling --- */


/**
 * Returns a string representation of the CDDB error code.
 *
 * @return The error string
 */
const char *cddb_error_str(cddb_error_t errnum);

/**
 * Prints the error message associated with the current error number
 * on the given stream.
 *
 * @param stream The stream
 * @param errnum The error number
 */
void cddb_error_stream_print(FILE *stream, cddb_error_t errnum);

/**
 * Prints the error message associated with the current error number
 * to stderr.
 *
 * @param errnum The error number
 */
void cddb_error_print(cddb_error_t errnum);


#ifdef __cplusplus
    }
#endif

#endif /* CDDB_ERROR_H */
