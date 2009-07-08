#ifndef __STREAMER_H
#define __STREAMER_H

int
streamer_init (void);

void
streamer_free (void);

int
streamer_read (char *bytes, int size);

#endif // __STREAMER_H
