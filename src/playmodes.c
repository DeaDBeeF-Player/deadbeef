#include "playmodes.h"
#include "conf.h"
#include "messagepump.h"
#include "threading.h"

static ddb_shuffle_t _shuffle;
static ddb_repeat_t _repeat;
static uintptr_t mutex;

void
streamer_playmodes_init (void) {
    mutex = mutex_create ();
}

void
streamer_playmodes_free (void) {
    mutex_free (mutex);
    mutex = 0;
}

void
streamer_set_shuffle (ddb_shuffle_t shuffle) {
    mutex_lock (mutex);
    ddb_shuffle_t prev = _shuffle;
    if (prev == shuffle) {
        mutex_unlock (mutex);
        return;
    }

    _shuffle = shuffle;
    mutex_unlock (mutex);

    conf_set_int ("playback.order", shuffle);

    messagepump_push (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

ddb_shuffle_t
streamer_get_shuffle (void) {
    mutex_lock (mutex);
    ddb_shuffle_t shuffle = _shuffle;
    mutex_unlock (mutex);
    return shuffle;
}

void
streamer_set_repeat (ddb_repeat_t repeat) {
    mutex_lock (mutex);
    ddb_repeat_t prev = _repeat;
    if (prev == repeat) {
        mutex_unlock (mutex);
        return;
    }

    _repeat = repeat;
    mutex_unlock (mutex);

    conf_set_int ("playback.loop", repeat);
    messagepump_push (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

ddb_repeat_t
streamer_get_repeat (void) {
    mutex_lock (mutex);
    ddb_repeat_t repeat = _repeat;
    mutex_unlock (mutex);
    return repeat;
}
