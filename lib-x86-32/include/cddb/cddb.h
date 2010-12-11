/*
    $Id: cddb.h,v 1.14 2006/10/15 12:54:33 airborne Exp $

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

#ifndef CDDB_H
#define CDDB_H 1

#include <cddb/version.h>

#ifdef __cplusplus
    extern "C" {
#endif


#include <cddb/cddb_config.h>
#include <cddb/cddb_error.h>
#include <cddb/cddb_track.h>
#include <cddb/cddb_disc.h>
#include <cddb/cddb_site.h>
#include <cddb/cddb_conn.h>
#include <cddb/cddb_cmd.h>
#include <cddb/cddb_log.h>


/**
 * \mainpage libCDDB, a C API for CDDB server access
 */


#define BIT(n) (1 << n)

/**
 * An enumeration of flags that influence the behaviour of the
 * library.  You can set or reset these flags using the
 * #libcddb_set_flags and #libcddb_reset_flags functions.
 */
typedef enum {
    CDDB_F_EMPTY_STR = BIT(0),    /**< never return NULL pointer strings
                                       (default), return an empty string
                                       instead */
    CDDB_F_NO_TRACK_ARTIST = BIT(1), /**< do not return the disc artist as the
                                       track artist (default), return NULL
                                       instead */
} cddb_flag_t;

/**
 * Initializes the library.  This is used to setup any globally used
 * variables.  The first time you create a new CDDB connection structure
 * the library will automatically initialize itself.  So, there is no
 * need to explicitly call this function.
 */
void libcddb_init(void);

/**
 * Frees up any global (cross connection) resources.  You should call
 * this function before terminating your program.  Using any library
 * calls after shutting down are bound to give problems.
 */
void libcddb_shutdown(void);

/**
 * Set one or more flags that influence the library behvaiour
 *
 * @param flags A bitwise ORed set of values from #cddb_flag_t.
 */
void libcddb_set_flags(unsigned int flags);

/**
 * Reset one or more flags that influence the library behvaiour
 *
 * @param flags A bitwise ORed set of values from #cddb_flag_t.
 */
void libcddb_reset_flags(unsigned int flags);


#ifdef __cplusplus
    }
#endif

#endif /* CDDB_H */
