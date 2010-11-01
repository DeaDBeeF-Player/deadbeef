/*
    $Id: cddb_net.h,v 1.11 2005/03/11 21:29:29 airborne Exp $

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

#ifndef CDDB_NET_H
#define CDDB_NET_H 1

#ifdef __cplusplus
    extern "C" {
#endif


#include <stdarg.h>

#if defined( UNDER_CE )
#   include <winsock.h>
#elif defined( WIN32 )
#   include <winsock2.h>
#   include <ws2tcpip.h>
#endif 

#include <cddb/cddb_ni.h>
#include <cddb/cddb_config.h>

#if defined(CDDB_NEED_SYS_SOCKET_H) || defined(HAVE_SYS_SOCKET_H)
#include <sys/socket.h>
#endif


/* --- socket-based work-alikes --- */


/**
 * This function performs the same task as the standard fgets except
 * for the fact that it might time-out if the socket read takes too
 * long.  In case of a time out, errno will be set to ETIMEDOUT.
 *
 * @param s       The string buffer.
 * @param size    Size of the buffer.
 * @param c       The CDDB connection structure.
 * @return The string that was read or NULL on error or EOF when no
 *         characters were read.
 */
char *sock_fgets(char *s, int size, cddb_conn_t *c);

/**
 * This function performs the same task as the standard fwrite except
 * for the fact that it might time-out if the socket write takes too
 * long.  In case of a time out, errno will be set to ETIMEDOUT.
 *
 * @param ptr     Pointer to data record.
 * @param size    Size of data record.
 * @param nmemb   The number of data records to write.
 * @param c       The CDDB connection structure.
 * @return The number of records written.
 */
size_t sock_fwrite(const void *ptr, size_t size, size_t nmemb, cddb_conn_t *c);

/**
 * This function performs the same task as the standard fprintf except
 * for the fact that it might time-out if the socket write takes too
 * long.  In case of a time out, errno will be set to ETIMEDOUT.
 *
 * @param c       The CDDB connection structure.
 * @param format  Pointer to data record.
 * @return The number of characters written.
 */
int sock_fprintf(cddb_conn_t *c, const char *format, ...);

/**
 * This function performs the same task as the standard vfprintf
 * except for the fact that it might time-out if the socket write
 * takes too long.  In case of a time out, errno will be set to
 * ETIMEDOUT.
 *
 * @param c       The CDDB connection structure.
 * @param format  Pointer to data record.
 * @param ap      Variable argument list.
 * @return The number of characters written.
 */
int sock_vfprintf(cddb_conn_t *c, const char *format, va_list ap);

/* --- time-out enabled work-alikes --- */

/**
 * This function performs the same task as the standard gethostbyname
 * except for the fact that it might time-out if the query takes too
 * long.  In case of a time out, errno will be set to ETIMEDOUT.
 *
 * @param hostname The hostname that needs to be resolved.
 * @param timeout  Number of seconds after which to time out.
 * @return The host entity for given host name or NULL if not found or
 *         timed out (errno will be set).
 */
struct hostent *timeout_gethostbyname(const char *hostname, int timeout);

/**
 * This function performs the same task as the standard connect except
 * for the fact that it might time-out if the connect takes too long.
 * In case of a time out, errno will be set to ETIMEDOUT.
 * 
 * @param sockfd   The socket.
 * @param addr     The address to connect to.
 * @param len      The size of the address structure.
 * @param timeout  Number of seconds after which to time out.
 * @return Zero on success, -1 on failure (errno will be set).
 */
int timeout_connect(int sockfd, const struct sockaddr *addr, size_t len, 
                    int timeout);


#ifdef __cplusplus
    }
#endif

#endif /* CDDB_NET_H */
