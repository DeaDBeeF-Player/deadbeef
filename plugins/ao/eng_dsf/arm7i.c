//
// ARM7 processor emulator - interpreter core
// version 1.6 / 2008-02-16
// (c) Radoslaw Balcewicz
//

#include "arm7.h"
#include "arm7i.h"

  //--------------------------------------------------------------------------
  // definitions and macros

 /** PC is being incremented after every instruction fetch, so we adjust for
 that on all stores and jumps. */
#define PC_ADJUSTMENT (-4)

  /** Memory access routines. */
#include "arm7memil.c"

  /** Bit shifts compatible with IA32. */
#define SHL(w, k) (((UINT32)(w)) << (k))
#define SHR(w, k) (((UINT32)(w)) >> (k))
#define SAR(w, k) (((INT32)(w)) >> (k))
#define ROR(w, k) (SHR (w, k) | SHL (w, 32 - (k)))

  /** Byte rotation for unaligned 32-bit read. */
#define RBOD(w, i) (ROR (w, (i) * 8))

  /** Data processing macros. */
#define NEG(i) ((i) & (1 << 31))
#define POS(i) (~(i) & (1 << 31))
#define ADDCARRY(a, b, c) \
  ((NEG (a) & NEG (b)) |\
  (NEG (a) & POS (c)) |\
  (NEG (b) & POS (c))) ? 1 : 0;
#define ADDOVERFLOW(a, b, c) \
  ((NEG (a) & NEG (b) & POS (c)) |\
  (POS (a) & POS (b) & NEG (c))) ? 1 : 0;
#define SUBCARRY(a, b, c) \
  ((NEG (a) & POS (b)) |\
  (NEG (a) & POS (c)) |\
  (POS (b) & POS (c))) ? 1 : 0;
#define SUBOVERFLOW(a, b, c)\
  ((NEG (a) & POS (b) & POS (c)) |\
  (POS (a) & NEG (b) & NEG (c))) ? 1 : 0;
  //--------------------------------------------------------------------------

  //--------------------------------------------------------------------------
  // private functions
	   
  /** Condition EQ. */
static int R_WEQ (void);
  /** Condition NE. */
static int R_WNE (void);
  /** Condition CS. */
static int R_WCS (void);
  /** Condition CC. */
static int R_WCC (void);
  /** Condition MI. */
static int R_WMI (void);
  /** Condition PL. */
static int R_WPL (void);
  /** Condition VS. */
static int R_WVS (void);
  /** Condition VC. */
static int R_WVC (void);
  /** Condition HI. */
static int R_WHI (void);
  /** Condition LS. */
static int R_WLS (void);
  /** Condition GE. */
static int R_WGE (void);
  /** Condition LT. */
static int R_WLT (void);
  /** Condition GT. */
static int R_WGT (void);
  /** Condition LE. */
static int R_WLE (void);
  /** Condition AL. */
static int R_WAL (void);
  /** Undefined condition. */
static int R_Wxx (void);

  /** Calculates barrel shifter output. */
static UINT32 WyliczPrzes (void);
  /** Logical shift left. */
static UINT32 LSL_x (UINT32 w, int i);
  /** Logical shift right. */
static UINT32 LSR_x (UINT32 w, int i);
  /** Arithmetic shift right. */
static UINT32 ASR_x (UINT32 w, int i);
  /** Rotate right. */
static UINT32 ROR_x (UINT32 w, int i);
  /** Rotate right extended. */
static UINT32 RRX_1 (UINT32 w);

  /** Group 00x opcodes. */
static void R_G00x (void);
  /** Multiply instructions. */
static void R_MUL_MLA (void);
  /** Single data swap. */
static void R_SWP (void);
  /** PSR Transfer. */
static void R_PSR (void);
  /** Data processing instructions. */
static void R_DP (void);
  /** Data processing result writeback. */
static void R_WynikDP (ARM7_REG w);
  /** Data processing flags writeback. */
static void R_FlagiDP (ARM7_REG w);
  /** Single data transfer. */
static void R_SDT (void);
  /** Rozkaz "Undefined". */
static void R_Und ();
  /** Block Data Transfer. */
static void R_BDT ();
  /** Block load instructions. */
static void R_LDM (int Rn, UINT32 adres);
  /** Block store instructions. */
static void R_STM (int Rn, UINT32 adres);
  /** Branch/Branch with link. */
static void R_B_BL (void);
  /** Group 110 opcodes. */
static void R_G110 (void);
  /** Group 111 opcodes. */
static void R_G111 (void);

#ifdef ARM7_THUMB
  /** Halfword and Signed Data Transfer. */
static void R_HSDT ();
#endif
  //--------------------------------------------------------------------------

  //--------------------------------------------------------------------------
  // private data

  /** Flag testing functions for conditional execution. */
static int (*s_tabWar [16]) (void) = {R_WEQ, R_WNE, R_WCS, R_WCC, R_WMI, R_WPL,
 R_WVS, R_WVC, R_WHI, R_WLS, R_WGE, R_WLT, R_WGT, R_WLE, R_WAL, R_Wxx};
  /** Handler table for instruction groups. */
static void (*s_tabGrup [8]) (void) = {R_G00x, R_G00x, R_SDT, R_SDT, R_BDT,
 R_B_BL, R_G110, R_G111};
  /** Data processing instructions split to arithmetic and logical. */
static int s_tabAL [16] = {FALSE, FALSE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE,
 FALSE, FALSE, TRUE, TRUE, FALSE, FALSE, FALSE, FALSE};

  /** Cycles it took for current instruction to complete. */
static int s_cykle;
  //--------------------------------------------------------------------------


  // public functions


  //--------------------------------------------------------------------------
  /** Single step, returns number of burned cycles. */
int ARM7i_Step ()
  {
  ARM7.kod = arm7_read_32 (ARM7.Rx [ARM7_PC] & ~3);

  // we increment PC here, and if there's a load from memory it will simply
  // overwrite it (all PC modyfing code should be aware of this)
  ARM7.Rx [ARM7_PC] += 4;
  s_cykle = 2;
  // condition test and group selection
  if (s_tabWar [(ARM7.kod >> 28) & 15] ())
    s_tabGrup [(ARM7.kod >> 25) & 7] ();
  return s_cykle;
  }
  //--------------------------------------------------------------------------


  // private functions


  //--------------------------------------------------------------------------
  /** Condition EQ. */
int R_WEQ ()
  {
  // "Z set"
  return ARM7.Rx [ARM7_CPSR] & ARM7_CPSR_Z;
  }
  //--------------------------------------------------------------------------

  //--------------------------------------------------------------------------
  /** Condition NE. */
int R_WNE ()
  {
  // "Z clear"
  return !(ARM7.Rx [ARM7_CPSR] & ARM7_CPSR_Z);
  }
  //--------------------------------------------------------------------------

  //--------------------------------------------------------------------------
  /** Condition CS. */
int R_WCS ()
  {
  // "C set"
  return ARM7.Rx [ARM7_CPSR] & ARM7_CPSR_C;
  }
  //--------------------------------------------------------------------------

  //--------------------------------------------------------------------------
  /** Condition CC. */
int R_WCC ()
  {
  // "C clear"
  return !(ARM7.Rx [ARM7_CPSR] & ARM7_CPSR_C);
  }
  //--------------------------------------------------------------------------

  //--------------------------------------------------------------------------
  /** Condition MI. */
int R_WMI ()
  {
  // "N set"
  return ARM7.Rx [ARM7_CPSR] & ARM7_CPSR_N;
  }
  //--------------------------------------------------------------------------

  //--------------------------------------------------------------------------
  /** Condition PL. */
int R_WPL ()
  {
  // "N clear"
  return !(ARM7.Rx [ARM7_CPSR] & ARM7_CPSR_N);
  }
  //--------------------------------------------------------------------------

  //--------------------------------------------------------------------------
  /** Condition VS. */
int R_WVS ()
  {
  // "V set"
  return ARM7.Rx [ARM7_CPSR] & ARM7_CPSR_V;
  }
  //--------------------------------------------------------------------------

  //--------------------------------------------------------------------------
  /** Condition VC. */
int R_WVC ()
  {
  // "V clear"
  return !(ARM7.Rx [ARM7_CPSR] & ARM7_CPSR_V);
  }
  //--------------------------------------------------------------------------

  //--------------------------------------------------------------------------
  /** Condition HI. */
int R_WHI ()
  {
  // "C set and Z clear"
  return (ARM7.Rx [ARM7_CPSR] & ARM7_CPSR_C) &&\
 !(ARM7.Rx [ARM7_CPSR] & ARM7_CPSR_Z);
  }
  //--------------------------------------------------------------------------

  //--------------------------------------------------------------------------
  /** Condition LS. */
int R_WLS ()
  {
  // "C clear or Z set"
  return !(ARM7.Rx [ARM7_CPSR] & ARM7_CPSR_C) ||\
 (ARM7.Rx [ARM7_CPSR] & ARM7_CPSR_Z);
  }
  //--------------------------------------------------------------------------

  //--------------------------------------------------------------------------
  /** Condition GE. */
int R_WGE ()
  {
  // "N equals V"
  return (ARM7.Rx [ARM7_CPSR] & ARM7_CPSR_N) &&\
 (ARM7.Rx [ARM7_CPSR] & ARM7_CPSR_V) || !(ARM7.Rx [ARM7_CPSR] & ARM7_CPSR_N) &&\
 !(ARM7.Rx [ARM7_CPSR] & ARM7_CPSR_V);
  }
  //--------------------------------------------------------------------------

  //--------------------------------------------------------------------------
  /** Condition LT. */
int R_WLT ()
  {
  // "N not equal to V"
  return !(ARM7.Rx [ARM7_CPSR] & ARM7_CPSR_N) &&\
 (ARM7.Rx [ARM7_CPSR] & ARM7_CPSR_V) || (ARM7.Rx [ARM7_CPSR] & ARM7_CPSR_N) &&\
 !(ARM7.Rx [ARM7_CPSR] & ARM7_CPSR_V);
  }
  //--------------------------------------------------------------------------

  //--------------------------------------------------------------------------
  /** Condition GT. */
int R_WGT ()
  {
  // "Z clear AND (N equals V)"
  return !(ARM7.Rx [ARM7_CPSR] & ARM7_CPSR_Z) && R_WGE ();
  }
  //--------------------------------------------------------------------------

  //--------------------------------------------------------------------------
  /** Condition LE. */
int R_WLE ()
  {
  // "Z set OR (N not equal to V)"
  return (ARM7.Rx [ARM7_CPSR] & ARM7_CPSR_Z) || R_WLT ();
  }
  //--------------------------------------------------------------------------

  //--------------------------------------------------------------------------
  /** Condition AL. */
int R_WAL ()
  {
  // "(ignored)"
  return TRUE;
  }
  //--------------------------------------------------------------------------

  //--------------------------------------------------------------------------
  /** Undefined condition. */
int R_Wxx ()
  {
  // behaviour undefined
  return FALSE;
  }
  //--------------------------------------------------------------------------

  //--------------------------------------------------------------------------
  /** Calculates barrel shifter output. */
UINT32 WyliczPrzes ()
  {
  int Rm, Rs, i;
  UINT32 w;

  // Rm is source for the shift operation
  Rm = ARM7.kod & 15;

  if (ARM7.kod & (1 << 4))
    {
    s_cykle++;
    // shift count in Rs (8 lowest bits)
    if (Rm != ARM7_PC)
      w = ARM7.Rx [Rm];
    else
      w = (ARM7.Rx [ARM7_PC] & ~3) + 12 + PC_ADJUSTMENT;
    // Rs can't be PC
    Rs = (ARM7.kod >> 8) & 15;
    i = (UINT8)ARM7.Rx [Rs];
    if (i == 0)
      {
      // special case
      ARM7.carry = (ARM7.Rx [ARM7_CPSR] & ARM7_CPSR_C) ? 1 : 0;
      return w;
      }

    switch ((ARM7.kod >> 5) & 3)
      {
      case 0:
        w = LSL_x (w, i);
        break;
      case 1:
        w = LSR_x (w, i);
        break;
      case 2:
        w = ASR_x (w, i);
        break;
      case 3:
        w = ROR_x (w, i);
        break;
      }
    }
  else
    {
    // shift count as immediate in opcode
    if (Rm != ARM7_PC)
      w = ARM7.Rx [Rm];
    else
      w = (ARM7.Rx [ARM7_PC] & ~3) + 8 + PC_ADJUSTMENT;
    i = (ARM7.kod >> 7) & 31;

    switch ((ARM7.kod >> 5) & 3)
      {
      case 0:
        w = LSL_x (w, i);
        break;
      case 1:
        if (i > 0)
          w = LSR_x (w, i);
        else
          w = LSR_x (w, 32);
        break;
      case 2:
        if (i > 0)
          w = ASR_x (w, i);
        else
          w = ASR_x (w, 32);
        break;
      case 3:
        if (i > 0)
          w = ROR_x (w, i);
        else
          w = RRX_1 (w);
        break;
      }
    }
  return w;
  }
  //--------------------------------------------------------------------------

  //--------------------------------------------------------------------------
  /** Logical shift left. */
UINT32 LSL_x (UINT32 w, int i)
{
	// LSL #0 copies C into carry out and returns unmodified value
	if (i == 0)
	{
		ARM7.carry = (ARM7.Rx [ARM7_CPSR] & ARM7_CPSR_C) ? 1 : 0;
		return w;
	}
	// LSL #32 copies LSB to carry out and returns zero
	if (i == 32)
	{
		ARM7.carry = w & 1;
		return 0;
	}
	// LSL > #32 returns zero for both carry and output
	if (i > 32)
	{
		ARM7.carry = 0;
		return 0;
	}
        // normal shift
	ARM7.carry = (w & (1 << (32 - i))) ? 1 : 0;
	w = SHL (w, i);
	return w;
}
  //--------------------------------------------------------------------------

  //--------------------------------------------------------------------------
  /** Logical shift right. */
UINT32 LSR_x (UINT32 w, int i)
{
	// LSR #32 copies MSB to carry out and returns zero
	if (i == 32)
	{
		ARM7.carry = (w & (1 << 31)) ? 1 : 0;
		return 0;
	}
	// LSR > #32 returns zero for both carry and output
	if (i > 32)
	{
		ARM7.carry = 0;
		return 0;
	}
        // normal shift
	ARM7.carry = (w & (1 << (i - 1))) ? 1 : 0;
	w = SHR (w, i);
	return w;
}
  //--------------------------------------------------------------------------

  //--------------------------------------------------------------------------
  /** Arithmetic shift right. */
UINT32 ASR_x (UINT32 w, int i)
{
	// ASR >= #32 carry out and output value depends on the minus sign
	if (i >= 32)
	{
		if (w & (1 << 31))
		{
			ARM7.carry = 1;
			return ~0;
		}

		ARM7.carry = 0;
		return 0;
	}
	// normal shift
	ARM7.carry = (w & (1 << (i - 1))) ? 1 : 0;
	w = SAR (w, i);
	return w;
}
  //--------------------------------------------------------------------------

  //--------------------------------------------------------------------------
  /** Rotate right. */
UINT32 ROR_x (UINT32 w, int i)
{
	// mask count to [0; 31]
	i &= 0x1f;
	// ROR #32,#64,etc. copies MSB into carry out and returns unmodified value
	if (i == 0)
	{
		ARM7.carry = (w & (1 << 31)) ? 1 : 0;
		return w;
	}
	// normal shift
	ARM7.carry = (w & (1 << (i-1))) ? 1 : 0;
	w = ROR (w, i);
	return w;
}
  //--------------------------------------------------------------------------

  //--------------------------------------------------------------------------
  /** Rotate right extended. */
UINT32 RRX_1 (UINT32 w)
  {
  // same as RCR by 1 in IA32
  ARM7.carry = w & 1;
  return (w >> 1) | ((ARM7.Rx [ARM7_CPSR] & ARM7_CPSR_C) << 2);
  }
  //--------------------------------------------------------------------------

  //--------------------------------------------------------------------------
  /** Group 00x opcodes. */
void R_G00x ()
  {
#ifdef ARM7_THUMB
  // 24 constant bits
  if ((ARM7.kod & 0x0ffffff0) == 0x012fff10)	// BX - branch with possible mode transfer
  {
  #ifdef ARM7_THUMB
  	int Rn = ARM7.Rx[ARM7.kod & 0xf];

	// switching to Thumb mode?
	if (Rn & 1)
	{
		ARM7_SetCPSR(ARM7.Rx[ARM7_CPSR] | ARM7_CPSR_T);
	}
       
	ARM7.Rx[ARM7_PC] = Rn & ~1;
  #endif
  }
  // 15 constant bits
  else if ((ARM7.kod & 0x0fb00ff0) == 0x01000090)
    R_SWP ();
  // 10 constant bits
  else if ((ARM7.kod & 0x0fc000f0) == 0x00000090)
    R_MUL_MLA ();
  // 10 constant bits
  else if ((ARM7.kod & 0x0e400f90) == 0x00000090)
    R_HSDT ();
  // 9 constant bits
  else if ((ARM7.kod & 0x0f8000f0) == 0x00800090)
  {
//    logerror("G00x / Multiply long\n");
  }
  // 6 constant bits	 
  else if ((ARM7.kod & 0x0e400090) == 0x00400090)
    R_HSDT ();
  // 2 constant bits
  else
    {
    if ((ARM7.kod & 0x01900000) == 0x01000000)
      // TST, TEQ, CMP & CMN without S bit are "PSR Transfer"
      R_PSR ();
    else
      // the rest is "Data processing"
      R_DP ();
    }
#else
  if ((ARM7.kod & 0x03b00090) == 0x01000090)
    R_SWP ();
  else if ((ARM7.kod & 0x03c00090) == 0x00000090)
    R_MUL_MLA ();
  else
    {
    if ((ARM7.kod & 0x01900000) == 0x01000000)
      // TST, TEQ, CMP & CMN without S bit are "PSR Transfer"
      R_PSR ();
    else
      // the rest is "Data processing"
      R_DP ();
    }
#endif
  }
  //--------------------------------------------------------------------------

  //--------------------------------------------------------------------------
  /** Single data swap. */
void R_SWP ()
  {
  int Rn, Rd, Rm;
  UINT32 adres, w;

#define BIT_B (ARM7.kod & (1 << 21))

  s_cykle += 4;
  // none of these can be PC
  Rn = (ARM7.kod >> 16) & 15;
  Rd = (ARM7.kod >> 12) & 15;
  Rm = ARM7.kod & 15;
  adres = ARM7.Rx [Rn];

  if (BIT_B)
    {
    // "byte"
    w = arm7_read_8 (adres);
    arm7_write_8 (adres, (UINT8)ARM7.Rx [Rm]);
    }
  else
    {
    // "word"
    w = RBOD (arm7_read_32 (adres & ~3), adres & 3);
    arm7_write_32 (adres & ~3, ARM7.Rx [Rm]);
    }
  ARM7.Rx [Rd] = w;

#undef BIT_B
  }
  //--------------------------------------------------------------------------

  //--------------------------------------------------------------------------
  /** Multiply instructions. */
void R_MUL_MLA ()
  {
  int Rm, Rs, Rn, Rd;
  UINT32 wynik;

#define BIT_A (ARM7.kod & (1 << 21))
#define BIT_S (ARM7.kod & (1 << 20))

  s_cykle += 2;
  // none of these can be PC, also Rd != Rm
  Rd = (ARM7.kod >> 16) & 15,
  Rs = (ARM7.kod >> 8) & 15,
  Rm = ARM7.kod & 15;

  // MUL
  wynik = ARM7.Rx [Rm] * ARM7.Rx [Rs];
  if (BIT_A)
    {
    // MLA
    Rn = (ARM7.kod >> 12) & 15;
    wynik += ARM7.Rx [Rn];
    }
  ARM7.Rx [Rd] = wynik;

  if (BIT_S)
    {
    // V remains unchanged, C is undefined
    ARM7.Rx [ARM7_CPSR] &= ~(ARM7_CPSR_N | ARM7_CPSR_Z);
    if (wynik == 0)
      ARM7.Rx [ARM7_CPSR] |= ARM7_CPSR_Z;
    ARM7.Rx [ARM7_CPSR] |= wynik & 0x80000000;
    }

#undef BIT_S
#undef BIT_A
  }
  //--------------------------------------------------------------------------

  //--------------------------------------------------------------------------
  /** PSR Transfer. */
void R_PSR ()
  {
  int Rd, Rm;
  UINT32 w, arg;

#define BIT_I (ARM7.kod & (1 << 25))
#define BIT_P (ARM7.kod & (1 << 22))

  // none of the registers involved can be PC

  if (ARM7.kod & (1 << 21))
    {
    // MSR
    Rm = ARM7.kod & 15;
    if (BIT_I)
      // immediate (lower 12 bits)
      arg = ROR (ARM7.kod & 0xff, ((ARM7.kod >> 8) & 0xf) * 2);
    else
      // register
      arg = ARM7.Rx [Rm];

    // decode mask bits
    if (BIT_P)
      {
      w = ARM7.Rx [ARM7_SPSR];
      if (ARM7_CPSR_M (ARM7.Rx [ARM7_CPSR]) > ARM7_CPSR_M_usr &&\
 ARM7_CPSR_M (ARM7.Rx [ARM7_CPSR]) < ARM7_CPSR_M_sys)
        {
        if (ARM7.kod & (1 << 16))
          w = (w & 0xffffff00) | (arg & 0x000000ff);
        if (ARM7.kod & (1 << 17))
          w = (w & 0xffff00ff) | (arg & 0x0000ff00);
        if (ARM7.kod & (1 << 18))
          w = (w & 0xff00ffff) | (arg & 0x00ff0000);
        if (ARM7.kod & (1 << 19))
          // ARMv5E should have 0xf8000000 argument mask
          w = (w & 0x00ffffff) | (arg & 0xf0000000);
        }
      // force valid mode
      w |= 0x10;
      ARM7.Rx [ARM7_SPSR] = w;
      }
    else
      {
      w = ARM7.Rx [ARM7_CPSR];
      // only flags can be changed in User mode
      if (ARM7_CPSR_M (ARM7.Rx [ARM7_CPSR]) != ARM7_CPSR_M_usr)
        {
        if (ARM7.kod & (1 << 16))
          w = (w & 0xffffff00) | (arg & 0x000000ff);
        if (ARM7.kod & (1 << 17))
          w = (w & 0xffff00ff) | (arg & 0x0000ff00);
        if (ARM7.kod & (1 << 18))
          w = (w & 0xff00ffff) | (arg & 0x00ff0000);
        }
      if (ARM7.kod & (1 << 19))
        // ARMv5E should have 0xf8000000 argument mask
        w = (w & 0x00ffffff) | (arg & 0xf0000000);
      // force valid mode
      w |= 0x10;
      ARM7_SetCPSR (w);
      }
    }
  else
    {
    // MRS
    Rd = (ARM7.kod >> 12) & 15;
    if (BIT_P)
      ARM7.Rx [Rd] = ARM7.Rx [ARM7_SPSR];
    else
      ARM7.Rx [Rd] = ARM7.Rx [ARM7_CPSR];
    }

#undef BIT_P
#undef BIT_I
  }
  //--------------------------------------------------------------------------

  //--------------------------------------------------------------------------
  /** Data processing instructions. */
void R_DP ()
  {
  int Rn;
  ARM7_REG arg1, arg2, w;

#define BIT_I (ARM7.kod & (1 << 25))

  // Rn can be PC, so we need to account for that
  Rn = (ARM7.kod >> 16) & 15;

  if (BIT_I)
    {
    if (Rn != ARM7_PC)
      arg1 = ARM7.Rx [Rn];
    else
      arg1 = (ARM7.Rx [ARM7_PC] & ~3) + 8 + PC_ADJUSTMENT;
    // immediate in lowest 12 bits
    arg2 = ROR (ARM7.kod & 0xff, ((ARM7.kod >> 8) & 0xf) * 2);
    // preload carry out from C
    ARM7.carry = (ARM7.Rx [ARM7_CPSR] & ARM7_CPSR_C) ? 1 : 0;
    }
  else
    {
    if (Rn != ARM7_PC)
      arg1 = ARM7.Rx [Rn];
    else
      // register or immediate shift?
      if (ARM7.kod & (1 << 4))
        arg1 = (ARM7.Rx [ARM7_PC] & ~3) + 12 + PC_ADJUSTMENT;
      else
        arg1 = (ARM7.Rx [ARM7_PC] & ~3) + 8 + PC_ADJUSTMENT;
    // calculate in barrel shifter
    arg2 = WyliczPrzes ();
    }

  // decode instruction type
  switch ((ARM7.kod >> 21) & 15)
    {
    case 0:
      // AND
      R_WynikDP (arg1 & arg2);
      break;

    case 1:
      // EOR
      R_WynikDP (arg1 ^ arg2);
      break;

    case 2:
      // SUB
      w = arg1 - arg2;
      ARM7.carry = SUBCARRY (arg1, arg2, w);
      ARM7.overflow = SUBOVERFLOW (arg1, arg2, w);
      R_WynikDP (w);
      break;

    case 3:
      // RSB
      w = arg2 - arg1;
      ARM7.carry = SUBCARRY (arg2, arg1, w);
      ARM7.overflow = SUBOVERFLOW (arg2, arg1, w);
      R_WynikDP (w);
      break;

    case 4:
      // ADD
      w = arg1 + arg2;
      ARM7.carry = ADDCARRY (arg1, arg2, w);
      ARM7.overflow = ADDOVERFLOW (arg1, arg2, w);
      R_WynikDP (w);
      break;

    case 5:
      // ADC
      w = arg1 + arg2 + ((ARM7.Rx [ARM7_CPSR] & ARM7_CPSR_C) ? 1 : 0);
      ARM7.carry = ADDCARRY (arg1, arg2, w);
      ARM7.overflow = ADDOVERFLOW (arg1, arg2, w);
      R_WynikDP (w);
      break;

    case 6:
      // SBC
      w = arg1 - arg2 - ((ARM7.Rx [ARM7_CPSR] & ARM7_CPSR_C) ? 0 : 1);
      ARM7.carry = SUBCARRY (arg1, arg2, w);
      ARM7.overflow = SUBOVERFLOW (arg1, arg2, w);
      R_WynikDP (w);
      break;

    case 7:
      // RSC
      w = arg2 - arg1 - ((ARM7.Rx [ARM7_CPSR] & ARM7_CPSR_C) ? 0 : 1);
      ARM7.carry = SUBCARRY (arg2, arg1, w);
      ARM7.overflow = SUBOVERFLOW (arg2, arg1, w);
      R_WynikDP (w);
      break;

    case 8:
      // TST
      R_FlagiDP (arg1 & arg2);
      break;

    case 9:
      // TEQ
      R_FlagiDP (arg1 ^ arg2);
      break;

    case 10:
      // CMP
      w = arg1 - arg2;
      ARM7.carry = SUBCARRY (arg1, arg2, w);
      ARM7.overflow = SUBOVERFLOW (arg1, arg2, w);
      R_FlagiDP (w);
      break;

    case 11:
      // CMN
      w = arg1 + arg2;
      ARM7.carry = ADDCARRY (arg1, arg2, w);
      ARM7.overflow = ADDOVERFLOW (arg1, arg2, w);
      R_FlagiDP (w);
      break;

    case 12:
      // ORR
      R_WynikDP (arg1 | arg2);
      break;

    case 13:
      // MOV
      R_WynikDP (arg2);
      break;

    case 14:
      // BIC
      R_WynikDP (arg1 & ~arg2);
      break;

    case 15:
      // MVN
      R_WynikDP (~arg2);
      break;
    }

#undef BIT_I
  }
  //--------------------------------------------------------------------------

  //--------------------------------------------------------------------------
  /** Data processing result writeback. */
void R_WynikDP (ARM7_REG w)
  {
  int Rd;

#define BIT_S (ARM7.kod & (1 << 20))

  Rd = (ARM7.kod >> 12) & 15;
  ARM7.Rx [Rd] = w;
  if (BIT_S)
    {
    if (Rd == ARM7_PC)
      {
      s_cykle += 4;
      // copy current SPSR to CPSR
      ARM7_SetCPSR (ARM7.Rx [ARM7_SPSR]);
      }
    else
      // save new flags
      R_FlagiDP (w);
    }

#undef BIT_S
  }
  //--------------------------------------------------------------------------

  //--------------------------------------------------------------------------
  /** Data processing flags writeback. */
void R_FlagiDP (ARM7_REG w)
  {
  // arithmetic or logical instruction?
  if (s_tabAL [(ARM7.kod >> 21) & 15])
    {
    ARM7.Rx [ARM7_CPSR] &= ~(ARM7_CPSR_N | ARM7_CPSR_Z | ARM7_CPSR_C |\
 ARM7_CPSR_V);
    ARM7.Rx [ARM7_CPSR] |= ARM7.overflow << 28;
    }
  else
    ARM7.Rx [ARM7_CPSR] &= ~(ARM7_CPSR_N | ARM7_CPSR_Z | ARM7_CPSR_C);
  ARM7.Rx [ARM7_CPSR] |= ARM7.carry << 29;
  if (w == 0)
    ARM7.Rx [ARM7_CPSR] |= ARM7_CPSR_Z;
  ARM7.Rx [ARM7_CPSR] |= w & 0x80000000;
  }
  //--------------------------------------------------------------------------

  //--------------------------------------------------------------------------
  /** Single data transfer. */
void R_SDT (void)
  {
  int Rn, Rd, offset;
  UINT32 adres, w = 0;

#define BIT_I (ARM7.kod & (1 << 25))
#define BIT_P (ARM7.kod & (1 << 24))
#define BIT_U (ARM7.kod & (1 << 23))
#define BIT_B (ARM7.kod & (1 << 22))
#define BIT_W (ARM7.kod & (1 << 21))
#define BIT_L (ARM7.kod & (1 << 20))

  if (BIT_I && (ARM7.kod & (1 << 4)))
    {
    R_Und ();
    return;
    }

  Rn = (ARM7.kod >> 16) & 15,
  Rd = (ARM7.kod >> 12) & 15;
  if (Rn != ARM7_PC)
    adres = ARM7.Rx [Rn];
  else
    adres = ARM7.Rx [ARM7_PC] & ~3;
  if (!BIT_L)
    if (Rd != ARM7_PC)
      w = ARM7.Rx [Rd];
    else
      w = (ARM7.Rx [ARM7_PC] & ~3) + 12 + PC_ADJUSTMENT;

  if (BIT_I)
    // calculate value in barrel shifter
    offset = WyliczPrzes ();
  else
    // immediate in lowest 12 bits
    offset = ARM7.kod & 0xfff;

  if (!BIT_U)
    offset = -offset;
  if (BIT_P)
    {
    // "pre-index"
    adres += offset;
    if (BIT_W)
      // "write-back"
      ARM7.Rx [Rn] = adres;
    }
  else
    // "post-index"
    ARM7.Rx [Rn] += offset;
  if (Rn == ARM7_PC)
    adres += 8 + PC_ADJUSTMENT;

  if (BIT_L)
    {
    s_cykle += 3;
    // "load"
    if (BIT_B)
      // "byte"
      ARM7.Rx [Rd] = arm7_read_8 (adres);
    else
      // "word"
      ARM7.Rx [Rd] = RBOD (arm7_read_32 (adres & ~3), adres & 3);
    }
  else
    {
    s_cykle += 2;
    // "store"
    if (BIT_B)
      // "byte"
      arm7_write_8 (adres, (UINT8)w);
    else
      // "word"
      arm7_write_32 (adres & ~3, w);
    }

#undef BIT_L
#undef BIT_W
#undef BIT_B
#undef BIT_U
#undef BIT_P
#undef BIT_I
  }
  //--------------------------------------------------------------------------

  //--------------------------------------------------------------------------
  /** Undefined. */
void R_Und ()
  {
  UINT32 sr = ARM7.Rx [ARM7_CPSR];
  ARM7_SetCPSR (ARM7_CPSR_MX (sr, ARM7_CPSR_M_und) | ARM7_CPSR_I);
  ARM7.Rx [ARM7_SPSR] = sr;
  ARM7.Rx [ARM7_LR] = ARM7.Rx [ARM7_PC] + 4;
  ARM7.Rx [ARM7_PC] = 0x00000004;
  }
  //--------------------------------------------------------------------------

#define BIT_U (ARM7.kod & (1 << 23))
#define BIT_S (ARM7.kod & (1 << 22))
  //--------------------------------------------------------------------------
  /** Block Data Transfer. */
void R_BDT ()
  {
  int Rn, usr = FALSE;
  UINT32 adres;
  ARM7_REG cpsr = 0;

#define BIT_L (ARM7.kod & (1 << 20))

  // Rn can't be PC
  Rn = (ARM7.kod >> 16) & 15;
  adres = ARM7.Rx [Rn];

  // transfer in User mode
  if (BIT_S)
    if (!BIT_L || !(ARM7.kod & (1 << ARM7_PC)))
      usr = TRUE;

  if (usr)
    {
//EMU_BLAD (BLAD_WEWNETRZNY, "BDT: user transfer");
    cpsr = ARM7.Rx [ARM7_CPSR];
    ARM7_SetCPSR (ARM7_CPSR_MX (cpsr, ARM7_CPSR_M_usr));
    }

  if (BIT_L)
    // "load"
    R_LDM (Rn, adres);
  else
    // "store"
    R_STM (Rn, adres);

  if (usr)
    ARM7_SetCPSR (cpsr);

#undef BIT_L
  }
  //--------------------------------------------------------------------------

#define BIT_P (ARM7.kod & (1 << 24))
#define BIT_W (ARM7.kod & (1 << 21))
  //--------------------------------------------------------------------------
  /** Block load instructions. */
void R_LDM (int Rn, UINT32 adres)
  {
  int i, n, sp;

  // count registers on the list
  for (i = 0, n = 0; i < 16; i++)
    if (ARM7.kod & (1 << i))
      n++;
  s_cykle += n * 2 + 1;

  n <<= 2;
  // transfer type
  sp = BIT_P;
  if (!BIT_U)
    {
    // "down"
    n = -n;
    adres += n;
    sp = !sp;
    }
  if (BIT_W)
    // "write-back"
    ARM7.Rx [Rn] += n;

  // for all registers in mask
  if (sp)
    for (i = 0; i < 16; i++)
      {
      if (!(ARM7.kod & (1 << i)))
        continue;
      adres += 4;
      ARM7.Rx [i] = arm7_read_32 (adres);
      }
  else
    for (i = 0; i < 16; i++)
      {
      if (!(ARM7.kod & (1 << i)))
        continue;
      ARM7.Rx [i] = arm7_read_32 (adres);
      adres += 4;
      }

  // special case - mode change when PC is written
  if ((ARM7.kod & (1 << ARM7_PC)) && BIT_S)
    ARM7_SetCPSR (ARM7.Rx [ARM7_SPSR]);
  }
  //--------------------------------------------------------------------------

  //--------------------------------------------------------------------------
  /** Block store instructions. */
void R_STM (int Rn, UINT32 adres)
  {
  int i, n, p, sp;

  // count registers on the list and remember the first one
  for (i = 0, n = 0, p = -1; i < 16; i++)
    if (ARM7.kod & (1 << i))
      {
      n++;
      if (p < 0)
        p = i;
      }
  s_cykle += n * 2;

  n <<= 2;
  // transfer type
  sp = BIT_P;
  if (!BIT_U)
    {
    // "down"
    n = -n;
    adres += n;
    sp = !sp;
    }
  // if base register is not the first one to transfer, writeback happens here
  if (BIT_W && Rn != p)
    // "write-back"
    ARM7.Rx [Rn] += n;

  // registers R0-R14
  if (sp)
    for (i = 0; i < 15; i++)
      {
      if (!(ARM7.kod & (1 << i)))
        continue;
      adres += 4;
      arm7_write_32 (adres, ARM7.Rx [i]);
      }
  else
    for (i = 0; i < 15; i++)
      {
      if (!(ARM7.kod & (1 << i)))
        continue;
      arm7_write_32 (adres, ARM7.Rx [i]);
      adres += 4;
      }

  // PC is a special case
  if (ARM7.kod & (1 << ARM7_PC))
  {
    if (sp)
      {
      adres += 4;
      arm7_write_32 (adres, (ARM7.Rx [ARM7_PC] & ~3) + 12 + PC_ADJUSTMENT);
      }
    else
      {
      arm7_write_32 (adres, (ARM7.Rx [ARM7_PC] & ~3) + 12 + PC_ADJUSTMENT);
      adres += 4;
      }
   }

  // if base register is the first one to transfer, writeback happens here
  if (BIT_W && Rn == p)
    // "write-back"
    ARM7.Rx [Rn] += n;
  }
  //--------------------------------------------------------------------------
#undef BIT_W
#undef BIT_P
#undef BIT_S
#undef BIT_U

  //--------------------------------------------------------------------------
  /** Branch/Branch with link. */
void R_B_BL ()
  {
  INT32 offset;

#define BIT_L (ARM7.kod & (1 << 24))

  s_cykle += 4;
  offset = (ARM7.kod & 0x00ffffff) << 2;
  if (offset & 0x02000000)
    offset |= 0xfc000000;
  offset += 8 + PC_ADJUSTMENT;
  if (BIT_L)
    // "Branch with link"
    ARM7.Rx [ARM7_LR] = (ARM7.Rx [ARM7_PC] & ~3) + 4 + PC_ADJUSTMENT;
  // "Branch"
  ARM7.Rx [ARM7_PC] += offset;

#undef BIT_L
  }
  //--------------------------------------------------------------------------

  //--------------------------------------------------------------------------
  /** Group 110 opcodes. */
void R_G110 ()
  {
//	logerror("ARM7: G110 / Coprocessor data transfer\n");
  }
  //--------------------------------------------------------------------------

  //--------------------------------------------------------------------------
  /** Group 111 opcodes. */
void R_G111 ()
  {
  if ((ARM7.kod & 0xf0000000) == 0xe0000000)
    {
/*    if (ARM7.kod & (1 << 4))
	logerror("ARM7: G111 / Coprocessor register transfer\n");
    else
	logerror("ARM7: G111 / Coprocessor data operation\n"); */
    }
  else
    {
    UINT32 sr = ARM7.Rx [ARM7_CPSR];
    ARM7_SetCPSR (ARM7_CPSR_MX (sr, ARM7_CPSR_M_svc) | ARM7_CPSR_I);
    ARM7.Rx [ARM7_SPSR] = sr;
    ARM7.Rx [ARM7_LR] = ARM7.Rx [ARM7_PC];
    ARM7.Rx [ARM7_PC] = 0x00000008;
    }
  }
  //--------------------------------------------------------------------------

#ifdef ARM7_THUMB
  //--------------------------------------------------------------------------
  /** Halfword and Signed Data Transfer. */
void R_HSDT ()
  {
  int Rm, Rd, Rn, offset;
  uint32_t adres, w;

#define BIT_P (ARM7.kod & (1 << 24))
#define BIT_U (ARM7.kod & (1 << 23))
#define BIT_W (ARM7.kod & (1 << 21))
#define BIT_L (ARM7.kod & (1 << 20))
#define BIT_S (ARM7.kod & (1 << 6))
#define BIT_H (ARM7.kod & (1 << 5))

  // Rm can't be PC
  Rn = (ARM7.kod >> 16) & 15;
  Rd = (ARM7.kod >> 12) & 15;
  if (Rn != ARM7_PC)
    adres = ARM7.Rx [Rn];
  else
    adres = ARM7.Rx [ARM7_PC] & ~3;
  if (!BIT_L)
    if (Rd != ARM7_PC)
      w = ARM7.Rx [Rd];
    else
      w = (ARM7.Rx [ARM7_PC] & ~3) + 12 + POPRAWKA_PC;

  if (1 << 22)
    // immediate
    offset = ((ARM7.kod >> 4) & 0xf0) | (ARM7.kod & 15);
  else
    {
    // register
    Rm = ARM7.kod & 15;
    offset = ARM7.Rx [Rm];
    }

  if (!BIT_U)
    offset = -offset;
  if (BIT_P)
    {
    // "pre-index"
    adres += offset;
    if (BIT_W)
      // "write-back"
      ARM7.Rx [Rn] = adres;
    }
  else
    // "post-index"
    ARM7.Rx [Rn] += offset;
  if (Rn == ARM7_PC)
    adres += 8 + POPRAWKA_PC;

  if (BIT_L)
    {
    // "load"
    s_cykle += 3;
    if (BIT_S)
      {
      if (BIT_H)
        // "signed halfword"
        ARM7.Rx [Rd] = (INT32)(INT16)arm7_read_16 (adres);
      else
        // "signed byte"
        ARM7.Rx [Rd] = (INT32)(INT8)arm7_read_8 (adres);
      }
    else
      // "unsigned halfword"
      ARM7.Rx [Rd] = arm7_read_16 (adres);
    }
  else
    {
    // store
    s_cykle += 2;
    if (BIT_H)
      // "halfword"
      arm7_write_16 (adres, (UINT16)w);
    else
      // "byte"
      arm7_write_8 (adres, (UINT8)w);
    }

#undef BIT_H
#undef BIT_S
#undef BIT_L
#undef BIT_W
#undef BIT_U
#undef BIT_P
  }
  //--------------------------------------------------------------------------
#endif
