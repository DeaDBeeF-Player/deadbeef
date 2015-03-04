/**
 * @ingroup   file68_devel
 * @file      file68/rsc68.h
 * @author    Benjamin Gerard <ben@sashipa.com>
 * @date      1998/10/07
 * @brief     sc68 resources access.
 *
 * $Id: rsc68.h,v 2.2 2003/09/20 19:24:30 benjihan Exp $
 *
 */

/* Copyright (C) 1998-2003 Benjamin Gerard */

#ifndef _RSC68_H_
#define _RSC68_H_

#include "file68/istream68.h"

#ifdef __cplusplus
extern "C" {
#endif

/** SC68 resource file type. */
typedef enum
{
  SC68rsc_replay,       /**< 68000 external replay.           */
  SC68rsc_config,       /**< Config file.                     */
  SC68rsc_sample,       /**< Sc68 sample files.               */
  SC68rsc_dll,          /**< Sc68 dynamic library.            */
} SC68rsc_t;

/** Resource handle function type. */ 
typedef istream_t * (*SC68rsc_handler_t)(SC68rsc_t , const char *, int);

/** Set shared resource path.
 *
 *    The SC68rsc_set_share() function set the shared resource path. The
 *    path will be duplicate by SC68strdup(). If path is null the current
 *    path is freed.
 *
 *    @param  path  New shared resource path (0 for free).
 *
 *    @return new path (duplicated string).
 *    @retval 0 error (except for freeing)
 *
 */
const char * SC68rsc_set_share(const char *path);

/** Set user resource path.
 *
 *    The SC68rsc_set_user() function set the user resource path. The
 *    path will be duplicate by SC68strdup(). If path is null the current
 *    path is freed.
 *
 *    @param  path  New user resource path (0 for free).
 *
 *    @return new path (duplicated string).
 *    @retval 0 error (except for freeing)
 */
const char * SC68rsc_set_user(const char *path);

/** Get resource pathes.
 */
void SC68rsc_get_path(const char **share, const char **user);

/** Set/Get resource handler.
 *
 *    The SC68rsc_set_handler() function set the current resource handler.
 *    If 0 is given as fct parameter the function does not set the handler.
 *    In all case the function returns the current handler. See below for
 *    more information about the default resource handler.
 *
 *    @par Resource handler
 *    The resource handler is a function called by the SC68rsc_open() function.
 *    Some preliminary tests has already been performed. So the handler can
 *    assume that the name is not a NULL pointer and the mode is valid
 *    (either 1:reading or 2:writing). The resource handler @b must return
 *    an @b already opened istream_t or 0 in error case.
 *
 *    @par Default resource handler
 *    The Default handler use a the istream_file_create() function.
 *    - If open mode is 2 (write mode) the default handler use the user
 *    resource path.
 *    - If open mode is 1 (read mode) the default handler tries in this
 *    order the user resource path and the shared resource path.
 *
 *    @param  fct  New resource handler (0 for reading current value).
 *    @return previous value.
 */
SC68rsc_handler_t SC68rsc_set_handler(SC68rsc_handler_t fct);

/** Open a resource in given mode.
 *
 *    The function SC68rsc_open() function opens an istream_t to access
 *    a resource.
 *
 *   @param  type  Type of resource to open.
 *   @param  name  Name of resource.
 *   @param  mode  1:read-access, 2:write-access.
 *
 *   @return  already opened istream_t stream.
 *   @retval  0 error.
 *
 * @see SC68rsc_set_handler() for more info on resource handler.
 */
istream_t * SC68rsc_open(SC68rsc_t type, const char *name, int mode);

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _RSC68_H_ */
