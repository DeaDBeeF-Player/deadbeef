#ifndef __DDB_PLATFORM_H
#define __DDB_PLATFORM_H

#include <string.h>

#ifndef _GNU_SOURCE
#ifndef strdupa
# define strdupa(s)							      \
    ({									      \
      const char *old = (s);					      \
      size_t len = strlen (old) + 1;				      \
      char *new = (char *) alloca (len);			      \
      (char *) memcpy (new, old, len);				      \
    })
#endif
#endif

#endif
