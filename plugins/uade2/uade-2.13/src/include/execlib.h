 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Miscellaneous bits for exec emulation
  *
  * Copyright 1996 Bernd Schmidt
  */

#define CMD_INVALID 0
#define CMD_RESET   1
#define CMD_READ    2
#define CMD_WRITE   3
#define CMD_UPDATE  4
#define CMD_CLEAR   5
#define CMD_STOP    6
#define CMD_START   7
#define CMD_FLUSH   8
#define CMD_NONSTD  9

#define NT_TASK     1
#define NT_DEVICE   3
#define NT_MSGPORT  4
#define NT_MESSAGE  5
#define NT_FREEMSG  6
#define NT_REPLYMSG 7
#define NT_RESOURCE 8
#define NT_LIBRARY  9
#define NT_SIGNALSEM 15

#ifndef MEMF_PUBLIC /* protection for AmigaDOS */
#define MEMF_PUBLIC 1
#define MEMF_CHIP 2
#define MEMF_FAST 4
#define MEMF_LOCAL 256
#define MEMF_24BITDMA 512
#define MEMF_CLEAR (1<<16)
#define MEMF_LARGEST (1<<17)
#define MEMF_REVERSE (1<<18)
#define MEMF_TOTAL (1<<19)
#endif
