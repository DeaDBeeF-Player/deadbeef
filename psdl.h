#ifndef __PSDL_H
#define __PSDL_H

struct playItem_s;

int
psdl_init (void);

void
psdl_free (void);

int
psdl_play (struct playItem_s *it);

int
psdl_stop (void);

int
psdl_pause (void);

#endif // __PSDL_H
