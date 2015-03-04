/**
 * @ingroup   emu68_devel
 * @file      ioplug68.c
 * @author    Ben(jamin) Gerard <ben@sashipa.com>
 * @date      1999/03/13
 * @brief     68000 IO manager
 * @version   $Id: ioplug68.c,v 2.0 2003/08/21 04:58:35 benjihan Exp $
 */

/* Copyright (C) 1998-2001 Ben(jamin) Gerard */
 
#include "emu68/ioplug68.h"
#include "emu68/mem68.h"

extern reg68_t reg68;

/* Unplug all IO
*/
void EMU68ioplug_unplug_all(void)
{
  io68_t *io;
  for(io=reg68.iohead; io!=NULL; io=io->next)
    EMU68ioplug_unplug(io);
}

/*  Unplug an IO :
 *  - remove from IO-list
 *  - remove memory access handler
 *
 *  return 0 if IO successfully unplugged
*/
int EMU68ioplug_unplug(io68_t *this_io)
{
  io68_t *io,*pio;

  if(this_io==NULL)
    return 0;

  for(io=reg68.iohead, pio=NULL; io!=NULL; pio=io, io=io->next)
  {
    /* Find it ??? */
    if(io==this_io)
    {
      /* Remove from IO-list */
      if(pio==NULL)
        reg68.iohead = io->next;
      else
        pio->next = io->next;
      reg68.nio--;

      /* Remove memory acces handler */
      EMU68memory_reset_area((io->addr_low>>8)&255);

      return 0;
    }
  }
  return -1;
}

/* Avoid circular linkage */
static io68_t *exist_io(io68_t *iolist, io68_t *io)
{
  for(; iolist!=NULL && io!=iolist; iolist=iolist->next);
  return iolist;
}

/*  Plug new IO :
 *  - add to io list
 *  - add new memory access handler
*/
void EMU68ioplug(io68_t *io)
{
  /* Check if exist */
  if(exist_io(reg68.iohead,io)!=NULL) return;

  /* Add to IO-list */
  io->next = reg68.iohead;
  reg68.iohead = io;
  reg68.nio++;

  /* Add memory access handler */
  EMU68memory_new_area((io->addr_low>>8)&255, io->Rfunc, io->Wfunc);
}
