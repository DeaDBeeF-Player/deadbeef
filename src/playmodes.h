#ifndef playmodes_h
#define playmodes_h

#include <deadbeef/deadbeef.h>

#ifdef __cplusplus
extern "C" {
#endif

void
streamer_playmodes_init (void);

void
streamer_playmodes_free (void);

void
streamer_set_shuffle (ddb_shuffle_t shuffle);

ddb_shuffle_t
streamer_get_shuffle (void);

void
streamer_set_repeat (ddb_repeat_t repeat);

ddb_repeat_t
streamer_get_repeat (void);

#ifdef __cplusplus
}
#endif

#endif /* playmodes_h */
