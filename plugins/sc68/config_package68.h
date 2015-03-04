/* sc68 package version configuration. */

#ifndef _SC68_CONFIG_PACKAGE68_H_
#define _SC68_CONFIG_PACKAGE68_H_

#if defined(HAVE_CONFIG_H) || defined(_MSC_VER)

#define PACKAGE68           "sc68"
#define PACKAGE68_BUGREPORT "bug@sashipa.com"
#define PACKAGE68_NAME      "sc68"
#define PACKAGE68_STRING    "sc68 2.2.1"
#define PACKAGE68_TARNAME   "sc68"
#define PACKAGE68_VERSION   "2.2.1"
#define VERSION68           "2.2.1"
#define VERSION68_NUM       221
#define PACKAGE68_URL       "http://sashipa.ben.free.fr/sc68"

#endif /* #ifdef HAVE_CONFIG_H */

/* Fallback */
#ifndef NO_FALLBACK_CONFIG

#ifndef PACKAGE68
# define PACKAGE68           "undefined package"
#endif
#ifndef PACKAGE68_BUGREPORT
# define PACKAGE68_BUGREPORT "undefined bug-report"
#endif
#ifndef PACKAGE68_NAME
# define PACKAGE68_NAME      PACKAGE68
#endif
#ifndef PACKAGE68_STRING
# define PACKAGE68_STRING    PACKAGE68_NAME " " PACKAGE68_VERSION
#endif
#ifndef PACKAGE68_TARNAME
# define PACKAGE68_TARNAME   PACKAGE68
#endif
#ifndef PACKAGE68_VERSION
# define PACKAGE68_VERSION   "undefined version"
#endif
#ifndef VERSION68
# define VERSION68           PACKAGE68_VERSION    
#endif
#ifndef VERSION68_NUM
# define VERSION68_NUM       0
#endif
#ifndef PACKAGE68_URL
# define PACKAGE68_URL       "undefined website URL"
#endif

#endif /* #ifndef NO_FALLBACK_CONFIG */

#endif /* #ifndef _SC68_CONFIG_PACKAGE68_H_ */
