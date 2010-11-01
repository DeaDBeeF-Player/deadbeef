/*
    $Id: cddb_cmd.h,v 1.17 2006/10/15 08:58:51 airborne Exp $

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

#ifndef CDDB_CMD_H
#define CDDB_CMD_H 1

#ifdef __cplusplus
    extern "C" {
#endif


/* --- accessing data on the CDDB server --- */


/**
 * Retrieve a disc record from the CDDB server.  This function
 * requires that the category and disc ID of the provided disc
 * structure are valid.
 *
 * If nothing goes wrong, the function will return 1 and the error
 * code will be reset to:
 * - #CDDB_ERR_OK:
 *     If everything went as planned.
 *
 * If there is a problem with reading data from the CDDB server one of
 * the following error codes will be set:
 * - #CDDB_ERR_DATA_MISSING:
 *     If some required data is missing from the given disc
 *     structure to execute this command.
 * - #CDDB_ERR_DISC_NOT_FOUND:
 *     If the requested disc is not known by the CDDB server.
 * - #CDDB_ERR_SERVER_ERROR:
 *     If the server encountered an error while trying to process your
 *     request.
 * - #CDDB_ERR_UNKNOWN:
 *     If the server specified an unknown response code.  Please
 *     report this as a libcddb bug.
 *
 * When there are problems with the connection to the CDDB server one
 * of the following error codes will be set:
 * - #CDDB_ERR_UNKNOWN_HOST_NAME:
 *     If there was an error when resolving the host name of the CDDB
 *     server.
 * - #CDDB_ERR_CONNECT:
 *     If a connection to the CDDB server could not be established.
 *     This can be due to incorrect data about the location of the
 *     server (host name, port).
 * - #CDDB_ERR_NOT_CONNECTED:
 *     If something when wrong in the process and you got
 *     disconnected.  Retrying might succeed (but no guarantees).
 * - #CDDB_ERR_PERMISSION_DENIED:
 *     If the server is up and running but denied the connection.
 *     This can occur when the server is too highly loaded or the
 *     handshake information (user name, ...) is considered to be
 *     invalid.
 *
 * @param c    The CDDB connection structure.
 * @param disc A non-null CDDB disc structure.
 * @return 1 on succes, 0 on failure
 */
int cddb_read(cddb_conn_t *c, cddb_disc_t *disc);

/**
 * Query the CDDB database for a list of possible disc matches.  This
 * function requires that the disc ID and disc length of the provided
 * disc structure are valid.  The disc should also contain a number of
 * tracks and for each track its frame offset on the CD should be
 * valid.
 *
 * If there are multiple matches then only the first one will be
 * returned by this function.  For other matches you will have to use
 * the #cddb_query_next function.
 *
 * @param c    The CDDB connection structure.
 * @param disc A non-null CDDB disc structure.
 *
 * @return The number of matches found or -1 on error.
 */
int cddb_query(cddb_conn_t *c, cddb_disc_t *disc);

/**
 * Returns the next match in a CDDB query result set.  This function
 * should be used in conjunction with #cddb_query.
 *
 * @param c    The CDDB connection structure.
 * @param disc A non-null CDDB disc structure.
 */
int cddb_query_next(cddb_conn_t *c, cddb_disc_t *disc);

/**
 * Perform a text search in the CDDB database.  Instead of actually
 * needing information about a real disc like in #cddb_query this
 * function accept a string that is used for searching the database.
 *
 * If there are multiple matches then only the first one will be
 * returned by this function.  For other matches you will have to use
 * the #cddb_search_next function.
 *
 * @param c    The CDDB connection structure.
 * @param disc A non-null CDDB disc structure.
 * @param str  The search string
 *
 * @return The number of matches found or -1 on error.
 */
int cddb_search(cddb_conn_t *c, cddb_disc_t *disc, const char *str);

/**
 * Returns the next match in a CDDB search result set.  This function
 * should be used in conjunction with #cddb_search.
 *
 * @param c    The CDDB connection structure.
 * @param disc A non-null CDDB disc structure.
 */
int cddb_search_next(cddb_conn_t *c, cddb_disc_t *disc);

/**
 * Perform a text search in the CDDB database.  It uses the album
 * command implemented on the freedb2.org servers.  Either the album
 * title or artist's name should be filled in, in the disc structure.
 *
 * If there are multiple matches then only the first one will be
 * returned by this function.  For other matches you will have to use
 * the #cddb_album_next function.
 *
 * @param c The CDDB connection structure.
 * @param disc A non-null CDDB disc structure.
 *
 * @return The number of matches found or -1 on error.
 */
int cddb_album(cddb_conn_t *c, cddb_disc_t *disc);

/**
 * Returns the next match in a CDDB album result set.  This function
 * should be used in conjunction with #cddb_album.
 *
 * @param c    The CDDB connection structure.
 * @param disc A non-null CDDB disc structure.
 */
int cddb_album_next(cddb_conn_t *c, cddb_disc_t *disc);

/**
 * Submit a new or updated disc to the CDDB database.  This function
 * requires that the disc ID, length, category, artist and title of
 * the provided disc structure are valid.  The disc should also
 * contain a number of tracks and for each track its frame offset on
 * the CD and title should be valid.
 *
 * @param c    The CDDB connection structure.
 * @param disc A non-null CDDB disc structure.
 */
int cddb_write(cddb_conn_t *c, cddb_disc_t *disc);

/**
 * Query the currently configured server for a list of mirrors.
 * Accessing the list of mirror sites is done with the iterator
 * functions #cddb_first_site and #cddb_next_site.
 *
 * @param c The CDDB connection structure.
 */
int cddb_sites(cddb_conn_t *c);


#ifdef __cplusplus
    }
#endif

#endif /* CDDB_CMD_H */
