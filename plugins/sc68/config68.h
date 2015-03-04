/* sc68 global configuration */

#ifndef _SC68_CONFIG_H_
#define _SC68_CONFIG_H_

#if defined(HAVE_CONFIG_H) || defined(HAVE_CONFIG_CONFIG68_H)
/*# include <config_config68.h>*/
# ifndef HAVE_CONFIG_H
#  define HAVE_CONFIG_H 1
# endif
# ifndef HAVE_CONFIG_CONFIG68_H
#  define HAVE_CONFIG_CONFIG68_H 1
# endif
#else
# ifndef _MSC_VER
#  warning "package may be not properly configured."
# endif
# undef HAVE_CONFIG_H
# undef HAVE_CONFIG_CONFIG68_H
#endif

#include <config_package68.h>
#include <config_option68.h>
#include <config_type68.h>
#include <config_platform68.h>

#endif /* #ifndef _SC68_CONFIG_H_ */
