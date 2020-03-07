#ifndef shellexecutil_h
#define shellexecutil_h

#include "../../deadbeef.h"

int
shellexec_eval_command (const char *shcommand, char *output, size_t size, DB_playItem_t *it);

#endif /* shellexecutil_h */
