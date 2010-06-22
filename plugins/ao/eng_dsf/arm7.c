//
// ARM7 processor emulator
// version 1.6 / 2008-02-16
// (c) Radoslaw Balcewicz
//

#include "arm7.h"
#include "arm7i.h"

#ifdef ARM7_THUMB
#include "arm7thumb.h"
#endif

  //--------------------------------------------------------------------------
  // definitions and macros

  /** Macro for accessing banked registers. */
#define RX_BANK(t,r) (ARM7.Rx_bank [t][r - 8])
  //--------------------------------------------------------------------------

  //--------------------------------------------------------------------------
  // private functions

  /** CPU Reset. */
static void Reset (void);
  //--------------------------------------------------------------------------

  //--------------------------------------------------------------------------
  // public variables

  /** ARM7 state. */
struct sARM7 ARM7;

  // private variables

  /** Table for decoding bit-coded mode to zero based index. */
static const int s_tabTryb [32] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
 -1, -1, -1, -1, -1, -1, ARM7_MODE_usr, ARM7_MODE_fiq, ARM7_MODE_irq,
 ARM7_MODE_svc, -1, -1, -1, ARM7_MODE_abt, -1, -1, -1, ARM7_MODE_und,
 -1, -1, -1, ARM7_MODE_sys};
  //--------------------------------------------------------------------------


  // public functions


  //--------------------------------------------------------------------------
  /** ARM7 emulator init. */
void ARM7_Init ()
  {
  // sane startup values
  ARM7.fiq = 0;
  ARM7.irq = 0;
  ARM7.carry = 0;
  ARM7.overflow = 0;
  ARM7.flagi = FALSE;
  ARM7.cykle = 0;

  // reset will do the rest
  ARM7_HardReset ();
  }
  //--------------------------------------------------------------------------

  //--------------------------------------------------------------------------
  /** Power-ON reset. */
void ARM7_HardReset ()
  {
  // CPSR that makes sense
  ARM7.Rx [ARM7_CPSR] = ARM7_CPSR_I | ARM7_CPSR_F | ARM7_CPSR_M_svc;
  Reset ();
  }
  //--------------------------------------------------------------------------

  //--------------------------------------------------------------------------
  /** Hardware reset via /RESET line. */
void ARM7_SoftReset ()
  {
  Reset ();
  }
  //--------------------------------------------------------------------------

  //--------------------------------------------------------------------------
  /** CPSR update, possibly changing operating mode. */
void ARM7_SetCPSR (ARM7_REG sr)
  {
  int stary, nowy;

  stary = s_tabTryb [ARM7_CPSR_M (ARM7.Rx [ARM7_CPSR])];
  nowy = s_tabTryb [ARM7_CPSR_M (sr)];
  // do we have to change modes?
  if (nowy != stary)
    {
    // save this mode registers
    RX_BANK (stary, ARM7_SP) = ARM7.Rx [ARM7_SP],
    RX_BANK (stary, ARM7_LR) = ARM7.Rx [ARM7_LR],
    RX_BANK (stary, ARM7_SPSR) = ARM7.Rx [ARM7_SPSR];
    if (stary == ARM7_MODE_fiq)
      {
      // copy R8-R12
      RX_BANK (ARM7_MODE_fiq, 8) = ARM7.Rx [8],
      RX_BANK (ARM7_MODE_fiq, 9) = ARM7.Rx [9],
      RX_BANK (ARM7_MODE_fiq, 10) = ARM7.Rx [10],
      RX_BANK (ARM7_MODE_fiq, 11) = ARM7.Rx [11],
      RX_BANK (ARM7_MODE_fiq, 12) = ARM7.Rx [12];
      ARM7.Rx [8] = RX_BANK (ARM7_MODE_usr, 8),
      ARM7.Rx [9] = RX_BANK (ARM7_MODE_usr, 9),
      ARM7.Rx [10] = RX_BANK (ARM7_MODE_usr, 10),
      ARM7.Rx [11] = RX_BANK (ARM7_MODE_usr, 11),
      ARM7.Rx [12] = RX_BANK (ARM7_MODE_usr, 12);
      }

    // fetch new mode registers
    ARM7.Rx [ARM7_SP] = RX_BANK (nowy, ARM7_SP),
    ARM7.Rx [ARM7_LR] = RX_BANK (nowy, ARM7_LR),
    ARM7.Rx [ARM7_SPSR] = RX_BANK (nowy, ARM7_SPSR);
    if (nowy == ARM7_MODE_fiq)
      {
      // copy R8-R12
      RX_BANK (ARM7_MODE_usr, 8) = ARM7.Rx [8],
      RX_BANK (ARM7_MODE_usr, 9) = ARM7.Rx [9],
      RX_BANK (ARM7_MODE_usr, 10) = ARM7.Rx [10],
      RX_BANK (ARM7_MODE_usr, 11) = ARM7.Rx [11],
      RX_BANK (ARM7_MODE_usr, 12) = ARM7.Rx [12];
      ARM7.Rx [8] = RX_BANK (ARM7_MODE_fiq, 8),
      ARM7.Rx [9] = RX_BANK (ARM7_MODE_fiq, 9),
      ARM7.Rx [10] = RX_BANK (ARM7_MODE_fiq, 10),
      ARM7.Rx [11] = RX_BANK (ARM7_MODE_fiq, 11),
      ARM7.Rx [12] = RX_BANK (ARM7_MODE_fiq, 12);
      }
    }

  // new CPSR value
  ARM7.Rx [ARM7_CPSR] = sr;

  // mode change could've enabled interrups, so we test for those and set
  // appropriate flag for the instruction loop to catch
  if (ARM7.fiq)
    ARM7.flagi |= ARM7_FL_FIQ;
#ifndef ARM7_DREAMCAST
  if (ARM7.irq)
    ARM7.flagi |= ARM7_FL_IRQ;
#endif
  }
  //--------------------------------------------------------------------------

  //--------------------------------------------------------------------------
  /** Sets FIQ line state. */
void ARM7_SetFIQ (int stan)
  {
  stan = stan ? TRUE : FALSE;
  // we catch changes only
  if (stan ^ ARM7.fiq)
    {
    ARM7.fiq = stan;
    if (ARM7.fiq)
      ARM7.flagi |= ARM7_FL_FIQ;
    }
  }
  //--------------------------------------------------------------------------

  //--------------------------------------------------------------------------
  /** Sets IRQ line state. */
void ARM7_SetIRQ (int stan)
  {
  stan = stan ? TRUE : FALSE;
  // we catch changes only
  if (stan ^ ARM7.irq)
    {
    ARM7.irq = stan;
    if (ARM7.irq)
      ARM7.flagi |= ARM7_FL_IRQ;
    }
  }
  //--------------------------------------------------------------------------

  //--------------------------------------------------------------------------
  /** Tests for pending interrupts, switches to one if possible. */
void ARM7_CheckIRQ ()
  {
  UINT32 sr = ARM7.Rx [ARM7_CPSR];

  // clear all interrupt flags
  ARM7.flagi &= ~(ARM7_FL_FIQ | ARM7_FL_IRQ);
  
  // check for pending interrupts we can switch to
  // (FIQ can interrupt IRQ, but not the other way around)
  if (ARM7.fiq)
    {
    if (!(sr & ARM7_CPSR_F))
      {
      // FIQ
      ARM7_SetCPSR (ARM7_CPSR_MX (sr, ARM7_CPSR_M_fiq) | ARM7_CPSR_F | ARM7_CPSR_I);
      ARM7.Rx [ARM7_SPSR] = sr;
      // set new PC (return from interrupt will subtract 4)
      ARM7.Rx [ARM7_LR] = ARM7.Rx [ARM7_PC] + 4;
      ARM7.Rx [ARM7_PC] = 0x0000001c;
      }
    }
#ifndef ARM7_DREAMCAST
  if (ARM7.irq)
    {
    if (!(sr & ARM7_CPSR_I))
      {
      // IRQ
      ARM7_SetCPSR (ARM7_CPSR_MX (sr, ARM7_CPSR_M_irq) | ARM7_CPSR_I);
      ARM7.Rx [ARM7_SPSR] = sr;
      // set new PC (return from interrupt will subtract 4)
      ARM7.Rx [ARM7_LR] = ARM7.Rx [ARM7_PC] + 4;
      ARM7.Rx [ARM7_PC] = 0x00000018;
      ARM7.irq = 0;
      }
    }
#endif
  }
  //--------------------------------------------------------------------------

  //--------------------------------------------------------------------------
  /** Single step. */
void ARM7_Step ()
{
  // make a step
#ifdef ARM7_THUMB
  if (ARM7.Rx[ARM7_CPSR] & ARM7_CPSR_T)
  {
	ARM7i_Thumb_Step();
  }
  else
#endif
  {
        ARM7i_Step ();
  }
  // and test interrupts
  ARM7_CheckIRQ ();
}
  //--------------------------------------------------------------------------

  //--------------------------------------------------------------------------
  /** Runs emulation for at least n cycles, returns actual amount of cycles
 burned - normal interpreter. */
int ARM7_Execute (int n)
  {
  ARM7.cykle = 0;
  while (ARM7.cykle < n)
    {
    ARM7_CheckIRQ ();
    while (!ARM7.flagi && ARM7.cykle < n)
      // make one step, sum up cycles
      ARM7.cykle += ARM7i_Step ();
    }
  return ARM7.cykle;
  }
  //--------------------------------------------------------------------------


  // private functions


  //--------------------------------------------------------------------------
  /** CPU Reset. */
void Reset (void)
  {
  // clear ALU flags
  ARM7.carry = 0;
  ARM7.overflow = 0;
  // test CPSR mode and pick a valid one if necessary
  if (s_tabTryb [ARM7_CPSR_M (ARM7.Rx [ARM7_CPSR])] < 0)
    ARM7.Rx [ARM7_CPSR] = ARM7_CPSR_I | ARM7_CPSR_F | ARM7_CPSR_M_svc;
  // set up registers according to manual
  RX_BANK (ARM7_MODE_svc, ARM7_LR) = ARM7.Rx [ARM7_PC];
  RX_BANK (ARM7_MODE_svc, ARM7_SPSR) = ARM7.Rx [ARM7_CPSR];
  ARM7_SetCPSR (ARM7_CPSR_I | ARM7_CPSR_F | ARM7_CPSR_M_svc);
  ARM7.Rx [ARM7_PC] = 0x00000000;
  }
  //--------------------------------------------------------------------------
