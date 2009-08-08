#ifndef __STREAMER_H
#define __STREAMER_H

int
streamer_init (void);

void
streamer_free (void);

int
streamer_read (char *bytes, int size);

void
streamer_reset (int full);

void
streamer_lock (void);

void
streamer_unlock (void);

// pstate indicates what to do with playback
// -1 means "don't do anything"
// otherwise "set state to this value"
void
streamer_set_nextsong (int song, int pstate);

#endif // __STREAMER_H
