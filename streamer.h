#ifndef __STREAMER_H
#define __STREAMER_H

int
streamer_init (void);

void
streamer_free (void);

int
streamer_read (char *bytes, int size);

void
streamer_reset (void);

void
streamer_lock (void);

void
streamer_unlock (void);

#endif // __STREAMER_H
