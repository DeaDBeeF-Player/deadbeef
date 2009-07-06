#include "codec.h"
#include "threading.h"

static uintptr_t mutex;

void
codec_init_locking (void) {
    mutex = mutex_create ();
}

void
codec_free_locking (void) {
    mutex_free (mutex);
}

void
codec_lock (void) {
    mutex_lock (mutex);
}

void
codec_unlock (void) {
    mutex_unlock (mutex);
}

