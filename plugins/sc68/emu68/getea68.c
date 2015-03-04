/**
 * @ingroup   emu68_devel
 * @file      getea68.c
 * @author    Ben(jamin) Gerard <ben@sashipa.com>
 * @date      1999/03/13
 * @brief     Effective address calculation
 * @version   $Id: getea68.c,v 2.0 2003/08/21 04:58:35 benjihan Exp $
 */

/* Copyright (C) 1998-2001 Ben(jamin) Gerard */

#include "emu68/getea68.h"
#include "emu68/error68.h"
#include "emu68/mem68.h"

extern reg68_t reg68;

static u32 ea_error(u32 reg)
{
  EMU68error_add("get_ea(%d)",reg);
  return -1;
}

/* (AN) */
static u32 ea_inAN(u32 reg)
{
  return reg68.a[reg];
}

/* (AN)+ */
static u32 ea_inANpb(u32 reg)
{
  u32 addr = reg68.a[reg];
  reg68.a[reg] += 1+(reg==7);
  return addr;
}

static u32 ea_inANpw(u32 reg)
{
  u32 addr = reg68.a[reg];
  reg68.a[reg] += 2;
  return addr;
}

static u32 ea_inANpl(u32 reg)
{
  u32 addr = reg68.a[reg];
  reg68.a[reg] += 4;
  return addr;
}

/* -(AN) */
static u32 ea_inmANb(u32 reg)
{
  return reg68.a[reg]-=1+(reg==7);
}

static u32 ea_inmANw(u32 reg)
{
  return reg68.a[reg]-=2;
}

static u32 ea_inmANl(u32 reg)
{
  return reg68.a[reg]-=4;
}

/* d(AN) */
static u32 ea_indAN(u32 reg)
{
  return reg68.a[reg]+get_nextw();
}

/* d(AN,Xi) */
static u32 ea_inANXI(u32 reg)
{
  s32 w = get_nextw();
  s32 reg2;
  reg2 = (w>>12)&15;
  reg2 = (w&(1<<11)) ? (s32)reg68.d[reg2] : (s32)(s16)reg68.d[reg2];
  return reg68.a[reg]+(s8)w+reg2;
}

/* ABS.W */
static u32 ea_inABSW(u32 reg)
{
  return get_nextw();
}

/* ABS.L */
static u32 ea_inABSL(u32 reg)
{
  return get_nextl();
}

/* d(PC) */
static u32 ea_inrelPC(u32 reg)
{
  u32 pc = reg68.pc;
  return pc+get_nextw();
}

/* d(PC,Xi) */
static u32 ea_inPCXI(u32 reg)
{
  s32 w = get_nextw();
  s32 reg2;
  reg2 = (w>>12)&15;
  reg2 = (w&(1<<11)) ? (s32)reg68.d[reg2] : (s32)(s16)reg68.d[reg2];
  return reg68.pc+(s8)w+reg2-2;
}

/* # */
static u32 ea_inIMMb(u32 reg)
{
  u32 pc = reg68.pc;
  reg68.pc += 2;
  return pc+1;
}

static u32 ea_inIMMw(u32 reg)
{
  u32 pc = reg68.pc;
  reg68.pc += 2;
  return pc;
}

static u32 ea_inIMMl(u32 reg)
{
  u32 pc = reg68.pc;
  reg68.pc += 4;
  return pc;
}

/***************************
*                          *
* Opmode tables for Get EA *
*                          *
***************************/

/* Mode 7 tables */

static u32 (*ea_b7[8])(u32) =
{
  ea_inABSW, ea_inABSL, ea_inrelPC, ea_inPCXI,
  ea_inIMMb, ea_error, ea_error, ea_error
};

static u32 (*ea_w7[8])(u32) =
{
  ea_inABSW, ea_inABSL, ea_inrelPC, ea_inPCXI,
  ea_inIMMw, ea_error, ea_error, ea_error
};

static u32 (*ea_l7[8])(u32) =
{
  ea_inABSW, ea_inABSL, ea_inrelPC, ea_inPCXI,
  ea_inIMMl, ea_error, ea_error, ea_error
};

/* Mode 7 redirect functions */

static u32 ea_mode7b(u32 reg)
{
  return (ea_b7[reg])(reg);
}

static u32 ea_mode7w(u32 reg)
{
  return (ea_w7[reg])(reg);
}

static u32 ea_mode7l(u32 reg)
{
  return (ea_l7[reg])(reg);
}

u32 (*get_eab[8])(u32 reg) =
{
  ea_error, ea_error, ea_inAN, ea_inANpb,
  ea_inmANb,ea_indAN,ea_inANXI,
  ea_mode7b,
};

u32 (*get_eaw[8])(u32 reg) =
{
  ea_error, ea_error, ea_inAN, ea_inANpw,
  ea_inmANw,ea_indAN,ea_inANXI,
  ea_mode7w,
};

u32 (*get_eal[8])(u32 reg) =
{
  ea_error, ea_error, ea_inAN, ea_inANpl,
  ea_inmANl,ea_indAN,ea_inANXI,
  ea_mode7l,
};
