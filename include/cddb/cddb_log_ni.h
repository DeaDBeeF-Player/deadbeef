/*
    $Id: cddb_log_ni.h,v 1.3 2005/03/11 21:29:29 airborne Exp $

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

#ifndef CDDB_LOH_NI_H
#define CDDB_LOG_NI_H

#ifdef __cplusplus
    extern "C" {
#endif


/**
 */
void cddb_log(cddb_log_level_t level, const char *format, ...);

/**
 */
#define cddb_log_debug(...) cddb_log(CDDB_LOG_DEBUG, __VA_ARGS__)

/**
 */
#define cddb_log_info(...) cddb_log(CDDB_LOG_INFO, __VA_ARGS__)

/**
 */
#define cddb_log_warn(...) cddb_log(CDDB_LOG_WARN, __VA_ARGS__)

/**
 */
#define cddb_log_error(...) cddb_log(CDDB_LOG_ERROR, __VA_ARGS__)

/**
 */
#define cddb_log_crit(...) cddb_log(CDDB_LOG_CRITICAL, __VA_ARGS__)


#ifdef __cplusplus
    }
#endif

#endif /* CDDB_LOG_NI_H */
