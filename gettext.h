#ifndef _GETTEXT_H
#define _GETTEXT_H 1

#if ENABLE_NLS

# include <libintl.h>
#define _(s) gettext(s)

#else

#define _(s) (s)

#endif

#endif
