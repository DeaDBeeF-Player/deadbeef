#ifndef __PALSA_H
#define __PALSA_H

int
palsa_init (void);

void
palsa_free (void);

int
palsa_play (void);

int
palsa_stop (void);

int
palsa_ispaused (void);

int
palsa_pause (void);

int
palsa_unpause (void);

void
palsa_set_volume (float vol);

int
palsa_get_rate (void);

#endif // __PALSA_H

