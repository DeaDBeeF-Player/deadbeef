#include "playmodes.h"
#include "conf.h"
#include "messagepump.h"

static ddb_shuffle_t _shuffle;
static ddb_repeat_t _repeat;

void
streamer_set_shuffle (ddb_shuffle_t shuffle) {
    ddb_shuffle_t prev = _shuffle;
    if (prev == shuffle) {
        return;
    }

    _shuffle = shuffle;

    conf_set_int ("playback.order", _shuffle);

    messagepump_push (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

ddb_shuffle_t
streamer_get_shuffle (void) {
    return _shuffle;
}

void
streamer_set_repeat (ddb_repeat_t repeat) {
    ddb_repeat_t prev = _repeat;
    if (prev == repeat) {
        return;
    }

    _repeat = repeat;

    conf_set_int ("playback.loop", _repeat);
    messagepump_push (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

ddb_repeat_t
streamer_get_repeat (void) {
    return _repeat;
}
