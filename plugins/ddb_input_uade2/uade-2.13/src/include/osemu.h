 /*
  * UAE - The Un*x Amiga Emulator
  *
  * OS emulation prototypes
  *
  * Copyright 1996 Bernd Schmidt
  */

static inline char *raddr(uaecptr p)
{
    return p == 0 ? NULL : (char *)get_real_address(p);
}

extern void gfxlib_install(void);

/* graphics.library */

extern int GFX_WritePixel(uaecptr rp, int x, int y);

