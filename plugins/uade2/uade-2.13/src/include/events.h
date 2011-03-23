 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Events
  * These are best for low-frequency events. Having too many of them,
  * or using them for events that occur too frequently, can cause massive
  * slowdown.
  *
  * Copyright 1995-1998 Bernd Schmidt
  */

extern void reset_frame_rate_hack (void);
extern int rpt_available;

extern unsigned long int cycles, nextevent, is_lastline;
extern unsigned long int sample_evtime;
typedef void (*evfunc)(void);

struct ev
{
    int active;
    unsigned long int evtime, oldcycles;
    evfunc handler;
};

enum {
    ev_hsync, ev_copper, ev_cia,
    ev_blitter, ev_diskblk, ev_diskindex,
    ev_max
};

extern struct ev eventtab[ev_max];

static void events_schedule (void)
{
  unsigned long int mintime = ~0L;
  unsigned long int eventtime;
  /* HSYNC */
  if(eventtab[ev_hsync].active) {
    eventtime = eventtab[ev_hsync].evtime - cycles;
    if (eventtime < mintime) mintime = eventtime;
  }
  /* AUDIO */
#if 0
  if(eventtab[ev_audio].active) {
    eventtime = eventtab[ev_audio].evtime - cycles;
    if (eventtime < mintime) mintime = eventtime;
  }
#endif
  /* CIA */
  if(eventtab[ev_cia].active) {
    eventtime = eventtab[ev_cia].evtime - cycles;
    if (eventtime < mintime) mintime = eventtime;
  }
  nextevent = cycles + mintime;
}

static void do_cycles_slow (unsigned long cycles_to_add) {
  if ((nextevent - cycles) <= cycles_to_add) {
    for (; cycles_to_add != 0; cycles_to_add--) {
      if (++cycles == nextevent) {
	/* HSYNC */
	if(eventtab[ev_hsync].active && eventtab[ev_hsync].evtime == cycles) {
	  (*eventtab[ev_hsync].handler)();
	}
	/* AUDIO */
#if 0
	if(eventtab[ev_audio].active && eventtab[ev_audio].evtime == cycles) {
	  (*eventtab[ev_audio].handler)();
	}
#endif
	/* CIA */
	if(eventtab[ev_cia].active && eventtab[ev_cia].evtime == cycles) {
	  (*eventtab[ev_cia].handler)();
	}
	events_schedule();
      }
    }
  }
  cycles += cycles_to_add;
}

#define do_cycles do_cycles_slow
