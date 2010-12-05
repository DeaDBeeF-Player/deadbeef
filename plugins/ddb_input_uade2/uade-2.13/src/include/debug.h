 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Debugger
  *
  * (c) 1995 Bernd Schmidt
  *
  */

#define	MAX_HIST	10000

extern int firsthist;
extern int lasthist;
extern int debugging;
extern int debug_interrupt_happened;

#ifdef NEED_TO_DEBUG_BADLY
extern struct regstruct history[MAX_HIST];
extern union flagu historyf[MAX_HIST];
#else
extern uaecptr history[MAX_HIST];
#endif

extern void debug(void);
extern void activate_debugger(void);
