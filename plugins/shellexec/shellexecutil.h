#ifndef shellexecutil_h
#define shellexecutil_h

#include <deadbeef/deadbeef.h>

#ifdef __cplusplus
extern "C" {
#endif

int
shellexec_eval_command (const char *shcommand, char *output, size_t size, DB_playItem_t *it);

#ifdef __cplusplus
}
#endif

#endif /* shellexecutil_h */
