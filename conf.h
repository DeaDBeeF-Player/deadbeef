#ifndef __CONF_H
#define __CONF_H

extern char conf_alsa_soundcard[1024];
extern int conf_samplerate;
extern int conf_src_quality;
extern char conf_hvsc_path[1024];
extern int conf_hvsc_enable;

int
conf_load (void);

#endif // __CONF_H
