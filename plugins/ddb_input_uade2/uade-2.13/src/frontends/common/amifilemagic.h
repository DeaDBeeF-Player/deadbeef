#ifndef _UADE_AMIFILEMAGIC_H_
#define _UADE_AMIFILEMAGIC_H_

#include <stdio.h>

void uade_filemagic(unsigned char *buf, size_t bufsize, char *pre,
		    size_t realfilesize, const char *path, int verbose);

#endif
