 /*
  * UAE - The Un*x Amiga Emulator
  *
  * custom chip support
  *
  * (c) 1995 Bernd Schmidt
  */

/* These are the masks that are ORed together in the chipset_mask option.
 * If CSMASK_AGA is set, the ECS bits are guaranteed to be set as well.  */
#define CSMASK_ECS_AGNUS 1
#define CSMASK_ECS_DENISE 2
#define CSMASK_AGA 4

extern void custom_init (void);
extern void customreset (void);
extern int intlev (void);
extern void dumpcustom (void);

extern void do_disk (void);

extern void notice_new_xcolors (void);
extern void notice_screen_contents_lost (void);
extern void init_row_map (void);

extern int picasso_requested_on;
extern int picasso_on;

/* Set to 1 to leave out the current frame in average frame time calculation.
 * Useful if the debugger was active.  */
extern int bogusframe;

extern uae_u16 dmacon;
extern uae_u16 intena,intreq;

extern int current_hpos (void);

static inline int dmaen (unsigned int dmamask)
{
    return (dmamask & dmacon) && (dmacon & 0x200);
}

#define SPCFLAG_STOP 2
#define SPCFLAG_DISK 4
#define SPCFLAG_INT  8
#define SPCFLAG_BRK  16
#define SPCFLAG_EXTRA_CYCLES 32
#define SPCFLAG_TRACE 64
#define SPCFLAG_DOTRACE 128
#define SPCFLAG_DOINT 256
#define SPCFLAG_BLTNASTY 512
#define SPCFLAG_EXEC 1024
#define SPCFLAG_MODE_CHANGE 8192

extern int dskdmaen;
extern uae_u16 adkcon;

extern unsigned int joy0dir, joy1dir;
extern int joy0button, joy1button;

extern void INTREQ (uae_u16);
extern uae_u16 INTREQR (void);

/* maximums for statically allocated tables */

/* PAL/NTSC values */

/* The HRM says: The vertical blanking area (PAL) ranges from line 0 to line 29,
 * and no data can be displayed there. Nevertheless, we lose some overscan data
 * if minfirstline is set to 29. */

#define MAXHPOS_PAL 227
#define MAXHPOS_NTSC 227
#define MAXVPOS_PAL 312
#define MAXVPOS_NTSC 262
#define MINFIRSTLINE_PAL 21
#define MINFIRSTLINE_NTSC 18
#define VBLANK_ENDLINE_PAL 29
#define VBLANK_ENDLINE_NTSC 24
#define VBLANK_HZ_PAL 50
#define VBLANK_HZ_NTSC 60

#define SOUNDTICKS_PAL 3546895
#define SOUNDTICKS_NTSC 3579545

#define MAXHPOS (MAXHPOS_PAL)
#define MAXVPOS (MAXVPOS_PAL)
#define SOUNDTICKS (SOUNDTICKS_PAL)

extern int maxhpos, maxvpos, minfirstline, vblank_endline, numscrlines, vblank_hz;
extern unsigned long syncbase;
#define NUMSCRLINES (maxvpos+1-minfirstline+1)

#define DMA_AUD0      0x0001
#define DMA_AUD1      0x0002
#define DMA_AUD2      0x0004
#define DMA_AUD3      0x0008
#define DMA_DISK      0x0010
#define DMA_SPRITE    0x0020
#define DMA_BLITTER   0x0040
#define DMA_COPPER    0x0080
#define DMA_BITPLANE  0x0100
#define DMA_BLITPRI   0x0400

extern unsigned long frametime, timeframes;

/* 50 words give you 800 horizontal pixels. An A500 can't do that, so it ought
 * to be enough.  Don't forget to update the definition in genp2c.c as well.  */
#define MAX_WORDS_PER_LINE 50

extern uae_u32 hirestab_h[256][2];
extern uae_u32 lorestab_h[256][4];

extern uae_u32 hirestab_l[256][1];
extern uae_u32 lorestab_l[256][2];
