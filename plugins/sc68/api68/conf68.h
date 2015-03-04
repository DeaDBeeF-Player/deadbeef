/**
 * @ingroup   api68_devel
 * @file      api68/conf68.h
 * @author    Benjamin Gerard<ben@sashipa.com>
 * @date      1999/07/27
 * @brief     configuration file.
 *
 * $Id: conf68.h,v 2.1 2003/08/26 23:14:02 benjihan Exp $
 */

#ifndef _CONF68_H_
#define _CONF68_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup  api68_conf  configuration file
 *  @ingroup   api68_devel
 *
 *  This module prodives functions to access sc68 configuration file.
 *
 *  @{
 */

/** Config array. */
typedef int SC68config_t[8];

/** Check config values and correct invalid ones.
 */
int SC68config_valid(SC68config_t * conf);

/** Get index of named field in SC68config_t array.
 *
 *  @param  name  name of config field.
 *
 *  @return field index.
 *  @retval -1 error
 */
int SC68config_get_id(const char * name);

/** Load config from file.
 */
int SC68config_load(SC68config_t * conf);

/** Save config into file.
 */
int SC68config_save(SC68config_t * conf);

/** Fill config struct with default value.
 */
int SC68config_default(SC68config_t * conf);

/**
 *  @}
 */

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _CONF68_H_ */
