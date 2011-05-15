/*
    $Id: cddb_log.h,v 1.4 2005/03/11 21:29:29 airborne Exp $

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

#ifndef CDDB_LOH_H
#define CDDB_LOG_H

#ifdef __cplusplus
    extern "C" {
#endif

/**
 * The different log levels supported by libcddb.
 */
typedef enum {
    CDDB_LOG_DEBUG = 1,         /**< Debug-level messages. */
    CDDB_LOG_INFO,              /**< Informational. */
    CDDB_LOG_WARN,              /**< Warning conditions. */
    CDDB_LOG_ERROR,             /**< Error conditions.  */
    CDDB_LOG_CRITICAL,          /**< Critical conditions. */
    CDDB_LOG_NONE = 99          /**< No log messages. */
} cddb_log_level_t;


/**
 * This type defines the signature of a libcddb log handler.  For
 * every message being logged by libcddb, the handler will receive the
 * log level and the message string.
 *
 * @see cddb_log_set_handler
 * @see cddb_log_level_t
 *
 * @param level   The log level.
 * @param message The log message.
 */
typedef void (*cddb_log_handler_t)(cddb_log_level_t level, const char *message);

/**
 * Set a custom log handler for libcddb.  The return value is the log
 * handler being replaced.  If the provided parameter is NULL, then
 * the handler will be reset to the default handler.
 *
 * @see cddb_log_handler_t
 *
 * @param new_handler The new log handler.
 * @return The previous log handler.
 */
cddb_log_handler_t cddb_log_set_handler(cddb_log_handler_t new_handler);

/**
 * Set the minimum log level.  This function is only useful in
 * conjunction with the default log handler.  The default log handler
 * will print any log messages that have a log level equal or higher
 * than this minimum log level to stderr.  By default the minimum log
 * level is set to CDDB_LOG_WARN.  This means that only warning, error
 * and critical messages will be printed.  You can silence the default
 * log handler by setting the minimum log level to CDDB_LOG_NONE.
 *
 * @see cddb_log_level_t
 *
 * @param level The minimum log level.
 */
void cddb_log_set_level(cddb_log_level_t level);


#ifdef __cplusplus
    }
#endif

#endif /* CDDB_LOG_H */
