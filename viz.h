#ifndef viz_h
#define viz_h

#include "deadbeef.h"

void
viz_process (char * restrict bytes, int bytes_size, DB_output_t *output);

void
viz_init (void);

void
viz_free (void);

#endif /* viz_h */
