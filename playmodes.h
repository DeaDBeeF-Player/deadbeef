#ifndef playmodes_h
#define playmodes_h

#include "deadbeef.h"

void
streamer_set_shuffle (ddb_shuffle_t shuffle);

ddb_shuffle_t
streamer_get_shuffle (void);

void
streamer_set_repeat (ddb_repeat_t repeat);

ddb_repeat_t
streamer_get_repeat (void);


#endif /* playmodes_h */
