#ifndef __PSDL_H
#define __PSDL_H

int
psdl_init (void);

void
psdl_free (void);

int
psdl_play (void);

int
psdl_stop (void);

int
psdl_ispaused (void);

int
psdl_pause (void);

int
psdl_unpause (void);

void
psdl_set_volume (float vol);

int
psdl_get_rate (void);

#endif // __PSDL_H
