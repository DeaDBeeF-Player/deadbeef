/*
    $Id: cddb_cmd_ni.h,v 1.12 2006/10/15 08:59:20 airborne Exp $

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

#ifndef CDDB_CMD_NI_H
#define CDDB_CMD_NI_H 1

#ifdef __cplusplus
    extern "C" {
#endif


typedef enum {
       CMD_HELLO = 0,
       CMD_QUIT,
       CMD_READ,
       CMD_QUERY,
       CMD_WRITE,
       CMD_PROTO,
       CMD_SITES,
       CMD_SEARCH,
       CMD_ALBUM,
       /* dummy for array size */
       CMD_LAST
} cddb_cmd_t;


/* --- utility functions --- */


/**
 * Will read in one line from the response input stream and parse both
 * the code and message in that line.  Errors will be signaled by
 * returning -1.
 *
 * @param c   the CDDB connection structure
 * @param msg the CDDB response msg
 * @return the CDDB response code or -1 on error
 */
int cddb_get_response_code(cddb_conn_t *c, char **msg);

/**
 */
int cddb_send_cmd(cddb_conn_t *c, int cmd, ...);


#ifdef __cplusplus
    }
#endif

#endif /* CDDB_CMD_H */
