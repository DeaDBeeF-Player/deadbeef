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

void
streamer_set_nextsong (int song);

#endif // __STREAMER_H
