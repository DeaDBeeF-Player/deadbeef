/*****************************************************************************
 *
 *	 z80.c
 *	 Portable Z80 emulator V3.5
 *
 *	 Copyright (C) 1998,1999,2000 Juergen Buchmueller, all rights reserved.
 *
 *	 - This source code is released as freeware for non-commercial purposes.
 *	 - You are free to use and redistribute this code in modified or
 *	   unmodified form, provided you list me in the credits.
 *	 - If you modify this source code, you must add a notice to each modified
 *	   source file that it has been changed.  If you're a nice person, you
 *	   will clearly mark each change too.  :)
 *	 - If you wish to use this for commercial purposes, please contact me at
 *	   pullmoll@t-online.de
 *	 - The author of this copywritten work reserves the right to change the
 *	   terms of its usage and license at any time, including retroactively
 *	 - This entire notice must remain in the source code.
 *
 *	 Changes in 3.5
 *	 - Implemented OTIR, INIR, etc. without look-up table for PF flag.
 *	   [Ramsoft, Sean Young]
 *	 Changes in 3.4
 *	 - Removed Z80-MSX specific code as it's not needed any more.
 *	 - Implemented DAA without look-up table [Ramsoft, Sean Young]
 *	 Changes in 3.3
 *	 - Fixed undocumented flags XF & YF in the non-asm versions of CP,
 *	   and all the 16 bit arithmetic instructions. [Sean Young]
 *	 Changes in 3.2
 *	 - Fixed undocumented flags XF & YF of RRCA, and CF and HF of
 *	   INI/IND/OUTI/OUTD/INIR/INDR/OTIR/OTDR [Sean Young]
 *	 Changes in 3.1
 *	 - removed the REPEAT_AT_ONCE execution of LDIR/CPIR etc. opcodes
 *	   for readabilities sake and because the implementation was buggy
 *	   (and I was not able to find the difference)
 *	 Changes in 3.0
 *	 - 'finished' switch to dynamically overrideable cycle count tables
 *	 Changes in 2.9:
 *	 - added methods to access and override the cycle count tables
 *	 - fixed handling and timing of multiple DD/FD prefixed opcodes
 *	 Changes in 2.8:
 *	 - OUTI/OUTD/OTIR/OTDR also pre-decrement the B register now.
 *	   This was wrong because of a bug fix on the wrong side
 *	   (astrocade sound driver).
 *	 Changes in 2.7:
 *	  - removed z80_vm specific code, it's not needed (and never was).
 *	 Changes in 2.6:
 *	  - BUSY_LOOP_HACKS needed to call change_pc16() earlier, before
 *		checking the opcodes at the new address, because otherwise they
 *		might access the old (wrong or even NULL) banked memory region.
 *		Thanks to Sean Young for finding this nasty bug.
 *	 Changes in 2.5:
 *	  - Burning cycles always adjusts the ICount by a multiple of 4.
 *	  - In REPEAT_AT_ONCE cases the R register wasn't incremented twice
 *		per repetition as it should have been. Those repeated opcodes
 *		could also underflow the ICount.
 *	  - Simplified TIME_LOOP_HACKS for BC and added two more for DE + HL
 *		timing loops. I think those hacks weren't endian safe before too.
 *	 Changes in 2.4:
 *	  - z80_reset zaps the entire context, sets IX and IY to 0xffff(!) and
 *		sets the Z flag. With these changes the Tehkan World Cup driver
 *		_seems_ to work again.
 *	 Changes in 2.3:
 *	  - External termination of the execution loop calls z80_burn() and
 *		z80_vm_burn() to burn an amount of cycles (R adjustment)
 *	  - Shortcuts which burn CPU cycles (BUSY_LOOP_HACKS and TIME_LOOP_HACKS)
 *		now also adjust the R register depending on the skipped opcodes.
 *	 Changes in 2.2:
 *	  - Fixed bugs in CPL, SCF and CCF instructions flag handling.
 *	  - Changed variable EA and ARG16() function to UINT32; this
 *		produces slightly more efficient code.
 *	  - The DD/FD XY CB opcodes where XY is 40-7F and Y is not 6/E
 *		are changed to calls to the X6/XE opcodes to reduce object size.
 *		They're hardly ever used so this should not yield a speed penalty.
 *	 New in 2.0:
 *	  - Optional more exact Z80 emulation (#define Z80_EXACT 1) according
 *		to a detailed description by Sean Young which can be found at:
 *		http://www.msxnet.org/tech/z80-documented.pdf
 *****************************************************************************/

#include "ao.h"

#include "cpuintrf.h"
#include "z80.h"

#include "mem.h"

#undef VERBOSE
#define VERBOSE 0

#if VERBOSE
#define LOG(x)	logerror x
#else
#define LOG(x)
#endif

/* execute main opcodes inside a big switch statement */
#ifndef BIG_SWITCH
#define BIG_SWITCH			1
#endif

/* big flags array for ADD/ADC/SUB/SBC/CP results */
#define BIG_FLAGS_ARRAY		1

/* Set to 1 for a more exact (but somewhat slower) Z80 emulation */
#define Z80_EXACT			1

/* on JP and JR opcodes check for tight loops */
#define BUSY_LOOP_HACKS		1

/* check for delay loops counting down BC */
#define TIME_LOOP_HACKS		1

#ifdef X86_ASM
#undef	BIG_FLAGS_ARRAY
#define BIG_FLAGS_ARRAY		0
#endif

static UINT8 z80_reg_layout[] = {
	Z80_PC, Z80_SP, Z80_AF, Z80_BC, Z80_DE, Z80_HL, -1,
	Z80_IX, Z80_IY, Z80_AF2,Z80_BC2,Z80_DE2,Z80_HL2,-1,
	Z80_R,	Z80_I,	Z80_IM, Z80_IFF1,Z80_IFF2, -1,
	Z80_NMI_STATE,Z80_IRQ_STATE,Z80_DC0,Z80_DC1,Z80_DC2,Z80_DC3, 0
};

static UINT8 z80_win_layout[] = {
	27, 0,53, 4,	/* register window (top rows) */
	 0, 0,26,22,	/* disassembler window (left colums) */
	27, 5,53, 8,	/* memory #1 window (right, upper middle) */
	27,14,53, 8,	/* memory #2 window (right, lower middle) */
	 0,23,80, 1,	/* command line window (bottom rows) */
};

/****************************************************************************/
/* The Z80 registers. HALT is set to 1 when the CPU is halted, the refresh	*/
/* register is calculated as follows: refresh=(Regs.R&127)|(Regs.R2&128)	*/
/****************************************************************************/
typedef struct {
/* 00 */	PAIR	PREPC,PC,SP,AF,BC,DE,HL,IX,IY;
/* 24 */	PAIR	AF2,BC2,DE2,HL2;
/* 34 */	UINT8	R,R2,IFF1,IFF2,HALT,IM,I;
/* 3B */	UINT8	irq_max;			/* number of daisy chain devices		*/
/* 3C */	INT8	request_irq;		/* daisy chain next request device		*/
/* 3D */	INT8	service_irq;		/* daisy chain next reti handling device */
/* 3E */	UINT8	nmi_state;			/* nmi line state */
/* 3F */	UINT8	irq_state;			/* irq line state */
/* 40 */	UINT8	int_state[Z80_MAXDAISY];
/* 44 */	Z80_DaisyChain irq[Z80_MAXDAISY];
/* 84 */	int		(*irq_callback)(int irqline);
/* 88 */	int		extra_cycles;		/* extra cycles for interrupts */
}	Z80_Regs;

#define CF	0x01
#define NF	0x02
#define PF	0x04
#define VF	PF
#define XF	0x08
#define HF	0x10
#define YF	0x20
#define ZF	0x40
#define SF	0x80

#define INT_IRQ 0x01
#define NMI_IRQ 0x02

#define _PPC	Z80.PREPC.d		/* previous program counter */

#define _PCD	Z80.PC.d
#define _PC		Z80.PC.w.l

#define _SPD	Z80.SP.d
#define _SP		Z80.SP.w.l

#define _AFD	Z80.AF.d
#define _AF		Z80.AF.w.l
#define _Z80_A		Z80.AF.b.h
#define _F		Z80.AF.b.l

#define _BCD	Z80.BC.d
#define _BC		Z80.BC.w.l
#define _Z80_B		Z80.BC.b.h
#define _Z80_C		Z80.BC.b.l

#define _DED	Z80.DE.d
#define _DE		Z80.DE.w.l
#define _Z80_D		Z80.DE.b.h
#define _Z80_E		Z80.DE.b.l

#define _HLD	Z80.HL.d
#define _HL		Z80.HL.w.l
#define _Z80_H		Z80.HL.b.h
#define _Z80_L		Z80.HL.b.l

#define _IXD	Z80.IX.d
#define _IX		Z80.IX.w.l
#define _HX		Z80.IX.b.h
#define _LX		Z80.IX.b.l

#define _IYD	Z80.IY.d
#define _IY		Z80.IY.w.l
#define _HY		Z80.IY.b.h
#define _LY		Z80.IY.b.l

#define _Z80_I		Z80.I
#define _Z80_R		Z80.R
#define _R2		Z80.R2
#define _IM		Z80.IM
#define _IFF1	Z80.IFF1
#define _IFF2	Z80.IFF2
#define _HALT	Z80.HALT

int z80_ICount;
static Z80_Regs Z80;
static UINT32 EA;
static int after_EI = 0;

static UINT8 SZ[256];		/* zero and sign flags */
static UINT8 SZ_BIT[256];	/* zero, sign and parity/overflow (=zero) flags for BIT opcode */
static UINT8 SZP[256];		/* zero, sign and parity flags */
static UINT8 SZHV_inc[256]; /* zero, sign, half carry and overflow flags INC r8 */
static UINT8 SZHV_dec[256]; /* zero, sign, half carry and overflow flags DEC r8 */

#if BIG_FLAGS_ARRAY
static UINT8 *SZHVC_add = 0;
static UINT8 *SZHVC_sub = 0;
#endif

static const UINT8 cc_op[0x100] = {
 4,10, 7, 6, 4, 4, 7, 4, 4,11, 7, 6, 4, 4, 7, 4,
 8,10, 7, 6, 4, 4, 7, 4,12,11, 7, 6, 4, 4, 7, 4,
 7,10,16, 6, 4, 4, 7, 4, 7,11,16, 6, 4, 4, 7, 4,
 7,10,13, 6,11,11,10, 4, 7,11,13, 6, 4, 4, 7, 4,
 4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
 4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
 4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
 7, 7, 7, 7, 7, 7, 4, 7, 4, 4, 4, 4, 4, 4, 7, 4,
 4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
 4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
 4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
 4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
 5,10,10,10,10,11, 7,11, 5,10,10, 0,10,17, 7,11,
 5,10,10,11,10,11, 7,11, 5, 4,10,11,10, 0, 7,11,
 5,10,10,19,10,11, 7,11, 5, 4,10, 4,10, 0, 7,11,
 5,10,10, 4,10,11, 7,11, 5, 6,10, 4,10, 0, 7,11};

static const UINT8 cc_cb[0x100] = {
 8, 8, 8, 8, 8, 8,15, 8, 8, 8, 8, 8, 8, 8,15, 8,
 8, 8, 8, 8, 8, 8,15, 8, 8, 8, 8, 8, 8, 8,15, 8,
 8, 8, 8, 8, 8, 8,15, 8, 8, 8, 8, 8, 8, 8,15, 8,
 8, 8, 8, 8, 8, 8,15, 8, 8, 8, 8, 8, 8, 8,15, 8,
 8, 8, 8, 8, 8, 8,12, 8, 8, 8, 8, 8, 8, 8,12, 8,
 8, 8, 8, 8, 8, 8,12, 8, 8, 8, 8, 8, 8, 8,12, 8,
 8, 8, 8, 8, 8, 8,12, 8, 8, 8, 8, 8, 8, 8,12, 8,
 8, 8, 8, 8, 8, 8,12, 8, 8, 8, 8, 8, 8, 8,12, 8,
 8, 8, 8, 8, 8, 8,15, 8, 8, 8, 8, 8, 8, 8,15, 8,
 8, 8, 8, 8, 8, 8,15, 8, 8, 8, 8, 8, 8, 8,15, 8,
 8, 8, 8, 8, 8, 8,15, 8, 8, 8, 8, 8, 8, 8,15, 8,
 8, 8, 8, 8, 8, 8,15, 8, 8, 8, 8, 8, 8, 8,15, 8,
 8, 8, 8, 8, 8, 8,15, 8, 8, 8, 8, 8, 8, 8,15, 8,
 8, 8, 8, 8, 8, 8,15, 8, 8, 8, 8, 8, 8, 8,15, 8,
 8, 8, 8, 8, 8, 8,15, 8, 8, 8, 8, 8, 8, 8,15, 8,
 8, 8, 8, 8, 8, 8,15, 8, 8, 8, 8, 8, 8, 8,15, 8};

static const UINT8 cc_ed[0x100] = {
 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
12,12,15,20, 8, 8, 8, 9,12,12,15,20, 8, 8, 8, 9,
12,12,15,20, 8, 8, 8, 9,12,12,15,20, 8, 8, 8, 9,
12,12,15,20, 8, 8, 8,18,12,12,15,20, 8, 8, 8,18,
12,12,15,20, 8, 8, 8, 8,12,12,15,20, 8, 8, 8, 8,
 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
16,16,16,16, 8, 8, 8, 8,16,16,16,16, 8, 8, 8, 8,
16,16,16,16, 8, 8, 8, 8,16,16,16,16, 8, 8, 8, 8,
 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8};

static const UINT8 cc_xy[0x100] = {
 4, 4, 4, 4, 4, 4, 4, 4, 4,15, 4, 4, 4, 4, 4, 4,
 4, 4, 4, 4, 4, 4, 4, 4, 4,15, 4, 4, 4, 4, 4, 4,
 4,14,20,10, 9, 9, 9, 4, 4,15,20,10, 9, 9, 9, 4,
 4, 4, 4, 4,23,23,19, 4, 4,15, 4, 4, 4, 4, 4, 4,
 4, 4, 4, 4, 9, 9,19, 4, 4, 4, 4, 4, 9, 9,19, 4,
 4, 4, 4, 4, 9, 9,19, 4, 4, 4, 4, 4, 9, 9,19, 4,
 9, 9, 9, 9, 9, 9,19, 9, 9, 9, 9, 9, 9, 9,19, 9,
19,19,19,19,19,19, 4,19, 4, 4, 4, 4, 9, 9,19, 4,
 4, 4, 4, 4, 9, 9,19, 4, 4, 4, 4, 4, 9, 9,19, 4,
 4, 4, 4, 4, 9, 9,19, 4, 4, 4, 4, 4, 9, 9,19, 4,
 4, 4, 4, 4, 9, 9,19, 4, 4, 4, 4, 4, 9, 9,19, 4,
 4, 4, 4, 4, 9, 9,19, 4, 4, 4, 4, 4, 9, 9,19, 4,
 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 0, 4, 4, 4, 4,
 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
 4,14, 4,23, 4,15, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4,
 4, 4, 4, 4, 4, 4, 4, 4, 4,10, 4, 4, 4, 4, 4, 4};

static const UINT8 cc_xycb[0x100] = {
23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,
23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,
23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,
23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,
23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,
23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,
23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,
23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,
23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,
23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,
23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,23};

/* extra cycles if jr/jp/call taken and 'interrupt latency' on rst 0-7 */
static const UINT8 cc_ex[0x100] = {
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* DJNZ */
 5, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0,	/* JR NZ/JR Z */
 5, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0,	/* JR NC/JR C */
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 5, 5, 5, 5, 0, 0, 0, 0, 5, 5, 5, 5, 0, 0, 0, 0,	/* LDIR/CPIR/INIR/OTIR LDDR/CPDR/INDR/OTDR */
 6, 0, 0, 0, 7, 0, 0, 2, 6, 0, 0, 0, 7, 0, 0, 2,
 6, 0, 0, 0, 7, 0, 0, 2, 6, 0, 0, 0, 7, 0, 0, 2,
 6, 0, 0, 0, 7, 0, 0, 2, 6, 0, 0, 0, 7, 0, 0, 2,
 6, 0, 0, 0, 7, 0, 0, 2, 6, 0, 0, 0, 7, 0, 0, 2};

static const UINT8 *cc[6] = { cc_op, cc_cb, cc_ed, cc_xy, cc_xycb, cc_ex };
#define Z80_TABLE_dd	Z80_TABLE_xy
#define Z80_TABLE_fd	Z80_TABLE_xy

static void take_interrupt(void);

typedef void (*funcptr)(void);

#define PROTOTYPES(tablename,prefix) \
	INLINE void prefix##_00(void); INLINE void prefix##_01(void); INLINE void prefix##_02(void); INLINE void prefix##_03(void); \
	INLINE void prefix##_04(void); INLINE void prefix##_05(void); INLINE void prefix##_06(void); INLINE void prefix##_07(void); \
	INLINE void prefix##_08(void); INLINE void prefix##_09(void); INLINE void prefix##_0a(void); INLINE void prefix##_0b(void); \
	INLINE void prefix##_0c(void); INLINE void prefix##_0d(void); INLINE void prefix##_0e(void); INLINE void prefix##_0f(void); \
	INLINE void prefix##_10(void); INLINE void prefix##_11(void); INLINE void prefix##_12(void); INLINE void prefix##_13(void); \
	INLINE void prefix##_14(void); INLINE void prefix##_15(void); INLINE void prefix##_16(void); INLINE void prefix##_17(void); \
	INLINE void prefix##_18(void); INLINE void prefix##_19(void); INLINE void prefix##_1a(void); INLINE void prefix##_1b(void); \
	INLINE void prefix##_1c(void); INLINE void prefix##_1d(void); INLINE void prefix##_1e(void); INLINE void prefix##_1f(void); \
	INLINE void prefix##_20(void); INLINE void prefix##_21(void); INLINE void prefix##_22(void); INLINE void prefix##_23(void); \
	INLINE void prefix##_24(void); INLINE void prefix##_25(void); INLINE void prefix##_26(void); INLINE void prefix##_27(void); \
	INLINE void prefix##_28(void); INLINE void prefix##_29(void); INLINE void prefix##_2a(void); INLINE void prefix##_2b(void); \
	INLINE void prefix##_2c(void); INLINE void prefix##_2d(void); INLINE void prefix##_2e(void); INLINE void prefix##_2f(void); \
	INLINE void prefix##_30(void); INLINE void prefix##_31(void); INLINE void prefix##_32(void); INLINE void prefix##_33(void); \
	INLINE void prefix##_34(void); INLINE void prefix##_35(void); INLINE void prefix##_36(void); INLINE void prefix##_37(void); \
	INLINE void prefix##_38(void); INLINE void prefix##_39(void); INLINE void prefix##_3a(void); INLINE void prefix##_3b(void); \
	INLINE void prefix##_3c(void); INLINE void prefix##_3d(void); INLINE void prefix##_3e(void); INLINE void prefix##_3f(void); \
	INLINE void prefix##_40(void); INLINE void prefix##_41(void); INLINE void prefix##_42(void); INLINE void prefix##_43(void); \
	INLINE void prefix##_44(void); INLINE void prefix##_45(void); INLINE void prefix##_46(void); INLINE void prefix##_47(void); \
	INLINE void prefix##_48(void); INLINE void prefix##_49(void); INLINE void prefix##_4a(void); INLINE void prefix##_4b(void); \
	INLINE void prefix##_4c(void); INLINE void prefix##_4d(void); INLINE void prefix##_4e(void); INLINE void prefix##_4f(void); \
	INLINE void prefix##_50(void); INLINE void prefix##_51(void); INLINE void prefix##_52(void); INLINE void prefix##_53(void); \
	INLINE void prefix##_54(void); INLINE void prefix##_55(void); INLINE void prefix##_56(void); INLINE void prefix##_57(void); \
	INLINE void prefix##_58(void); INLINE void prefix##_59(void); INLINE void prefix##_5a(void); INLINE void prefix##_5b(void); \
	INLINE void prefix##_5c(void); INLINE void prefix##_5d(void); INLINE void prefix##_5e(void); INLINE void prefix##_5f(void); \
	INLINE void prefix##_60(void); INLINE void prefix##_61(void); INLINE void prefix##_62(void); INLINE void prefix##_63(void); \
	INLINE void prefix##_64(void); INLINE void prefix##_65(void); INLINE void prefix##_66(void); INLINE void prefix##_67(void); \
	INLINE void prefix##_68(void); INLINE void prefix##_69(void); INLINE void prefix##_6a(void); INLINE void prefix##_6b(void); \
	INLINE void prefix##_6c(void); INLINE void prefix##_6d(void); INLINE void prefix##_6e(void); INLINE void prefix##_6f(void); \
	INLINE void prefix##_70(void); INLINE void prefix##_71(void); INLINE void prefix##_72(void); INLINE void prefix##_73(void); \
	INLINE void prefix##_74(void); INLINE void prefix##_75(void); INLINE void prefix##_76(void); INLINE void prefix##_77(void); \
	INLINE void prefix##_78(void); INLINE void prefix##_79(void); INLINE void prefix##_7a(void); INLINE void prefix##_7b(void); \
	INLINE void prefix##_7c(void); INLINE void prefix##_7d(void); INLINE void prefix##_7e(void); INLINE void prefix##_7f(void); \
	INLINE void prefix##_80(void); INLINE void prefix##_81(void); INLINE void prefix##_82(void); INLINE void prefix##_83(void); \
	INLINE void prefix##_84(void); INLINE void prefix##_85(void); INLINE void prefix##_86(void); INLINE void prefix##_87(void); \
	INLINE void prefix##_88(void); INLINE void prefix##_89(void); INLINE void prefix##_8a(void); INLINE void prefix##_8b(void); \
	INLINE void prefix##_8c(void); INLINE void prefix##_8d(void); INLINE void prefix##_8e(void); INLINE void prefix##_8f(void); \
	INLINE void prefix##_90(void); INLINE void prefix##_91(void); INLINE void prefix##_92(void); INLINE void prefix##_93(void); \
	INLINE void prefix##_94(void); INLINE void prefix##_95(void); INLINE void prefix##_96(void); INLINE void prefix##_97(void); \
	INLINE void prefix##_98(void); INLINE void prefix##_99(void); INLINE void prefix##_9a(void); INLINE void prefix##_9b(void); \
	INLINE void prefix##_9c(void); INLINE void prefix##_9d(void); INLINE void prefix##_9e(void); INLINE void prefix##_9f(void); \
	INLINE void prefix##_a0(void); INLINE void prefix##_a1(void); INLINE void prefix##_a2(void); INLINE void prefix##_a3(void); \
	INLINE void prefix##_a4(void); INLINE void prefix##_a5(void); INLINE void prefix##_a6(void); INLINE void prefix##_a7(void); \
	INLINE void prefix##_a8(void); INLINE void prefix##_a9(void); INLINE void prefix##_aa(void); INLINE void prefix##_ab(void); \
	INLINE void prefix##_ac(void); INLINE void prefix##_ad(void); INLINE void prefix##_ae(void); INLINE void prefix##_af(void); \
	INLINE void prefix##_b0(void); INLINE void prefix##_b1(void); INLINE void prefix##_b2(void); INLINE void prefix##_b3(void); \
	INLINE void prefix##_b4(void); INLINE void prefix##_b5(void); INLINE void prefix##_b6(void); INLINE void prefix##_b7(void); \
	INLINE void prefix##_b8(void); INLINE void prefix##_b9(void); INLINE void prefix##_ba(void); INLINE void prefix##_bb(void); \
	INLINE void prefix##_bc(void); INLINE void prefix##_bd(void); INLINE void prefix##_be(void); INLINE void prefix##_bf(void); \
	INLINE void prefix##_c0(void); INLINE void prefix##_c1(void); INLINE void prefix##_c2(void); INLINE void prefix##_c3(void); \
	INLINE void prefix##_c4(void); INLINE void prefix##_c5(void); INLINE void prefix##_c6(void); INLINE void prefix##_c7(void); \
	INLINE void prefix##_c8(void); INLINE void prefix##_c9(void); INLINE void prefix##_ca(void); INLINE void prefix##_cb(void); \
	INLINE void prefix##_cc(void); INLINE void prefix##_cd(void); INLINE void prefix##_ce(void); INLINE void prefix##_cf(void); \
	INLINE void prefix##_d0(void); INLINE void prefix##_d1(void); INLINE void prefix##_d2(void); INLINE void prefix##_d3(void); \
	INLINE void prefix##_d4(void); INLINE void prefix##_d5(void); INLINE void prefix##_d6(void); INLINE void prefix##_d7(void); \
	INLINE void prefix##_d8(void); INLINE void prefix##_d9(void); INLINE void prefix##_da(void); INLINE void prefix##_db(void); \
	INLINE void prefix##_dc(void); INLINE void prefix##_dd(void); INLINE void prefix##_de(void); INLINE void prefix##_df(void); \
	INLINE void prefix##_e0(void); INLINE void prefix##_e1(void); INLINE void prefix##_e2(void); INLINE void prefix##_e3(void); \
	INLINE void prefix##_e4(void); INLINE void prefix##_e5(void); INLINE void prefix##_e6(void); INLINE void prefix##_e7(void); \
	INLINE void prefix##_e8(void); INLINE void prefix##_e9(void); INLINE void prefix##_ea(void); INLINE void prefix##_eb(void); \
	INLINE void prefix##_ec(void); INLINE void prefix##_ed(void); INLINE void prefix##_ee(void); INLINE void prefix##_ef(void); \
	INLINE void prefix##_f0(void); INLINE void prefix##_f1(void); INLINE void prefix##_f2(void); INLINE void prefix##_f3(void); \
	INLINE void prefix##_f4(void); INLINE void prefix##_f5(void); INLINE void prefix##_f6(void); INLINE void prefix##_f7(void); \
	INLINE void prefix##_f8(void); INLINE void prefix##_f9(void); INLINE void prefix##_fa(void); INLINE void prefix##_fb(void); \
	INLINE void prefix##_fc(void); INLINE void prefix##_fd(void); INLINE void prefix##_fe(void); INLINE void prefix##_ff(void); \
static const funcptr tablename[0x100] = {	\
	prefix##_00,prefix##_01,prefix##_02,prefix##_03,prefix##_04,prefix##_05,prefix##_06,prefix##_07, \
	prefix##_08,prefix##_09,prefix##_0a,prefix##_0b,prefix##_0c,prefix##_0d,prefix##_0e,prefix##_0f, \
	prefix##_10,prefix##_11,prefix##_12,prefix##_13,prefix##_14,prefix##_15,prefix##_16,prefix##_17, \
	prefix##_18,prefix##_19,prefix##_1a,prefix##_1b,prefix##_1c,prefix##_1d,prefix##_1e,prefix##_1f, \
	prefix##_20,prefix##_21,prefix##_22,prefix##_23,prefix##_24,prefix##_25,prefix##_26,prefix##_27, \
	prefix##_28,prefix##_29,prefix##_2a,prefix##_2b,prefix##_2c,prefix##_2d,prefix##_2e,prefix##_2f, \
	prefix##_30,prefix##_31,prefix##_32,prefix##_33,prefix##_34,prefix##_35,prefix##_36,prefix##_37, \
	prefix##_38,prefix##_39,prefix##_3a,prefix##_3b,prefix##_3c,prefix##_3d,prefix##_3e,prefix##_3f, \
	prefix##_40,prefix##_41,prefix##_42,prefix##_43,prefix##_44,prefix##_45,prefix##_46,prefix##_47, \
	prefix##_48,prefix##_49,prefix##_4a,prefix##_4b,prefix##_4c,prefix##_4d,prefix##_4e,prefix##_4f, \
	prefix##_50,prefix##_51,prefix##_52,prefix##_53,prefix##_54,prefix##_55,prefix##_56,prefix##_57, \
	prefix##_58,prefix##_59,prefix##_5a,prefix##_5b,prefix##_5c,prefix##_5d,prefix##_5e,prefix##_5f, \
	prefix##_60,prefix##_61,prefix##_62,prefix##_63,prefix##_64,prefix##_65,prefix##_66,prefix##_67, \
	prefix##_68,prefix##_69,prefix##_6a,prefix##_6b,prefix##_6c,prefix##_6d,prefix##_6e,prefix##_6f, \
	prefix##_70,prefix##_71,prefix##_72,prefix##_73,prefix##_74,prefix##_75,prefix##_76,prefix##_77, \
	prefix##_78,prefix##_79,prefix##_7a,prefix##_7b,prefix##_7c,prefix##_7d,prefix##_7e,prefix##_7f, \
	prefix##_80,prefix##_81,prefix##_82,prefix##_83,prefix##_84,prefix##_85,prefix##_86,prefix##_87, \
	prefix##_88,prefix##_89,prefix##_8a,prefix##_8b,prefix##_8c,prefix##_8d,prefix##_8e,prefix##_8f, \
	prefix##_90,prefix##_91,prefix##_92,prefix##_93,prefix##_94,prefix##_95,prefix##_96,prefix##_97, \
	prefix##_98,prefix##_99,prefix##_9a,prefix##_9b,prefix##_9c,prefix##_9d,prefix##_9e,prefix##_9f, \
	prefix##_a0,prefix##_a1,prefix##_a2,prefix##_a3,prefix##_a4,prefix##_a5,prefix##_a6,prefix##_a7, \
	prefix##_a8,prefix##_a9,prefix##_aa,prefix##_ab,prefix##_ac,prefix##_ad,prefix##_ae,prefix##_af, \
	prefix##_b0,prefix##_b1,prefix##_b2,prefix##_b3,prefix##_b4,prefix##_b5,prefix##_b6,prefix##_b7, \
	prefix##_b8,prefix##_b9,prefix##_ba,prefix##_bb,prefix##_bc,prefix##_bd,prefix##_be,prefix##_bf, \
	prefix##_c0,prefix##_c1,prefix##_c2,prefix##_c3,prefix##_c4,prefix##_c5,prefix##_c6,prefix##_c7, \
	prefix##_c8,prefix##_c9,prefix##_ca,prefix##_cb,prefix##_cc,prefix##_cd,prefix##_ce,prefix##_cf, \
	prefix##_d0,prefix##_d1,prefix##_d2,prefix##_d3,prefix##_d4,prefix##_d5,prefix##_d6,prefix##_d7, \
	prefix##_d8,prefix##_d9,prefix##_da,prefix##_db,prefix##_dc,prefix##_dd,prefix##_de,prefix##_df, \
	prefix##_e0,prefix##_e1,prefix##_e2,prefix##_e3,prefix##_e4,prefix##_e5,prefix##_e6,prefix##_e7, \
	prefix##_e8,prefix##_e9,prefix##_ea,prefix##_eb,prefix##_ec,prefix##_ed,prefix##_ee,prefix##_ef, \
	prefix##_f0,prefix##_f1,prefix##_f2,prefix##_f3,prefix##_f4,prefix##_f5,prefix##_f6,prefix##_f7, \
	prefix##_f8,prefix##_f9,prefix##_fa,prefix##_fb,prefix##_fc,prefix##_fd,prefix##_fe,prefix##_ff  \
}

PROTOTYPES(Z80op,op);
PROTOTYPES(Z80cb,cb);
PROTOTYPES(Z80dd,dd);
PROTOTYPES(Z80ed,ed);
PROTOTYPES(Z80fd,fd);
PROTOTYPES(Z80xycb,xycb);

/****************************************************************************/
/* Burn an odd amount of cycles, that is instructions taking something		*/
/* different from 4 T-states per opcode (and R increment)					*/
/****************************************************************************/
INLINE void BURNODD(int cycles, int opcodes, int cyclesum)
{
	if( cycles > 0 )
	{
		_Z80_R += (cycles / cyclesum) * opcodes;
		z80_ICount -= (cycles / cyclesum) * cyclesum;
	}
}

/***************************************************************
 * define an opcode function
 ***************************************************************/
#define OP(prefix,opcode)  INLINE void prefix##_##opcode(void)

/***************************************************************
 * adjust cycle count by n T-states
 ***************************************************************/
#define CC(prefix,opcode) z80_ICount -= cc[Z80_TABLE_##prefix][opcode]

/***************************************************************
 * execute an opcode
 ***************************************************************/
#define EXEC(prefix,opcode)										\
{																\
	unsigned op = opcode;										\
	CC(prefix,op);												\
	(*Z80##prefix[op])();										\
}

#if BIG_SWITCH
#define EXEC_INLINE(prefix,opcode)								\
{																\
	unsigned op = opcode;										\
	CC(prefix,op);												\
	switch(op)													\
	{															\
	case 0x00:prefix##_##00();break; case 0x01:prefix##_##01();break; case 0x02:prefix##_##02();break; case 0x03:prefix##_##03();break; \
	case 0x04:prefix##_##04();break; case 0x05:prefix##_##05();break; case 0x06:prefix##_##06();break; case 0x07:prefix##_##07();break; \
	case 0x08:prefix##_##08();break; case 0x09:prefix##_##09();break; case 0x0a:prefix##_##0a();break; case 0x0b:prefix##_##0b();break; \
	case 0x0c:prefix##_##0c();break; case 0x0d:prefix##_##0d();break; case 0x0e:prefix##_##0e();break; case 0x0f:prefix##_##0f();break; \
	case 0x10:prefix##_##10();break; case 0x11:prefix##_##11();break; case 0x12:prefix##_##12();break; case 0x13:prefix##_##13();break; \
	case 0x14:prefix##_##14();break; case 0x15:prefix##_##15();break; case 0x16:prefix##_##16();break; case 0x17:prefix##_##17();break; \
	case 0x18:prefix##_##18();break; case 0x19:prefix##_##19();break; case 0x1a:prefix##_##1a();break; case 0x1b:prefix##_##1b();break; \
	case 0x1c:prefix##_##1c();break; case 0x1d:prefix##_##1d();break; case 0x1e:prefix##_##1e();break; case 0x1f:prefix##_##1f();break; \
	case 0x20:prefix##_##20();break; case 0x21:prefix##_##21();break; case 0x22:prefix##_##22();break; case 0x23:prefix##_##23();break; \
	case 0x24:prefix##_##24();break; case 0x25:prefix##_##25();break; case 0x26:prefix##_##26();break; case 0x27:prefix##_##27();break; \
	case 0x28:prefix##_##28();break; case 0x29:prefix##_##29();break; case 0x2a:prefix##_##2a();break; case 0x2b:prefix##_##2b();break; \
	case 0x2c:prefix##_##2c();break; case 0x2d:prefix##_##2d();break; case 0x2e:prefix##_##2e();break; case 0x2f:prefix##_##2f();break; \
	case 0x30:prefix##_##30();break; case 0x31:prefix##_##31();break; case 0x32:prefix##_##32();break; case 0x33:prefix##_##33();break; \
	case 0x34:prefix##_##34();break; case 0x35:prefix##_##35();break; case 0x36:prefix##_##36();break; case 0x37:prefix##_##37();break; \
	case 0x38:prefix##_##38();break; case 0x39:prefix##_##39();break; case 0x3a:prefix##_##3a();break; case 0x3b:prefix##_##3b();break; \
	case 0x3c:prefix##_##3c();break; case 0x3d:prefix##_##3d();break; case 0x3e:prefix##_##3e();break; case 0x3f:prefix##_##3f();break; \
	case 0x40:prefix##_##40();break; case 0x41:prefix##_##41();break; case 0x42:prefix##_##42();break; case 0x43:prefix##_##43();break; \
	case 0x44:prefix##_##44();break; case 0x45:prefix##_##45();break; case 0x46:prefix##_##46();break; case 0x47:prefix##_##47();break; \
	case 0x48:prefix##_##48();break; case 0x49:prefix##_##49();break; case 0x4a:prefix##_##4a();break; case 0x4b:prefix##_##4b();break; \
	case 0x4c:prefix##_##4c();break; case 0x4d:prefix##_##4d();break; case 0x4e:prefix##_##4e();break; case 0x4f:prefix##_##4f();break; \
	case 0x50:prefix##_##50();break; case 0x51:prefix##_##51();break; case 0x52:prefix##_##52();break; case 0x53:prefix##_##53();break; \
	case 0x54:prefix##_##54();break; case 0x55:prefix##_##55();break; case 0x56:prefix##_##56();break; case 0x57:prefix##_##57();break; \
	case 0x58:prefix##_##58();break; case 0x59:prefix##_##59();break; case 0x5a:prefix##_##5a();break; case 0x5b:prefix##_##5b();break; \
	case 0x5c:prefix##_##5c();break; case 0x5d:prefix##_##5d();break; case 0x5e:prefix##_##5e();break; case 0x5f:prefix##_##5f();break; \
	case 0x60:prefix##_##60();break; case 0x61:prefix##_##61();break; case 0x62:prefix##_##62();break; case 0x63:prefix##_##63();break; \
	case 0x64:prefix##_##64();break; case 0x65:prefix##_##65();break; case 0x66:prefix##_##66();break; case 0x67:prefix##_##67();break; \
	case 0x68:prefix##_##68();break; case 0x69:prefix##_##69();break; case 0x6a:prefix##_##6a();break; case 0x6b:prefix##_##6b();break; \
	case 0x6c:prefix##_##6c();break; case 0x6d:prefix##_##6d();break; case 0x6e:prefix##_##6e();break; case 0x6f:prefix##_##6f();break; \
	case 0x70:prefix##_##70();break; case 0x71:prefix##_##71();break; case 0x72:prefix##_##72();break; case 0x73:prefix##_##73();break; \
	case 0x74:prefix##_##74();break; case 0x75:prefix##_##75();break; case 0x76:prefix##_##76();break; case 0x77:prefix##_##77();break; \
	case 0x78:prefix##_##78();break; case 0x79:prefix##_##79();break; case 0x7a:prefix##_##7a();break; case 0x7b:prefix##_##7b();break; \
	case 0x7c:prefix##_##7c();break; case 0x7d:prefix##_##7d();break; case 0x7e:prefix##_##7e();break; case 0x7f:prefix##_##7f();break; \
	case 0x80:prefix##_##80();break; case 0x81:prefix##_##81();break; case 0x82:prefix##_##82();break; case 0x83:prefix##_##83();break; \
	case 0x84:prefix##_##84();break; case 0x85:prefix##_##85();break; case 0x86:prefix##_##86();break; case 0x87:prefix##_##87();break; \
	case 0x88:prefix##_##88();break; case 0x89:prefix##_##89();break; case 0x8a:prefix##_##8a();break; case 0x8b:prefix##_##8b();break; \
	case 0x8c:prefix##_##8c();break; case 0x8d:prefix##_##8d();break; case 0x8e:prefix##_##8e();break; case 0x8f:prefix##_##8f();break; \
	case 0x90:prefix##_##90();break; case 0x91:prefix##_##91();break; case 0x92:prefix##_##92();break; case 0x93:prefix##_##93();break; \
	case 0x94:prefix##_##94();break; case 0x95:prefix##_##95();break; case 0x96:prefix##_##96();break; case 0x97:prefix##_##97();break; \
	case 0x98:prefix##_##98();break; case 0x99:prefix##_##99();break; case 0x9a:prefix##_##9a();break; case 0x9b:prefix##_##9b();break; \
	case 0x9c:prefix##_##9c();break; case 0x9d:prefix##_##9d();break; case 0x9e:prefix##_##9e();break; case 0x9f:prefix##_##9f();break; \
	case 0xa0:prefix##_##a0();break; case 0xa1:prefix##_##a1();break; case 0xa2:prefix##_##a2();break; case 0xa3:prefix##_##a3();break; \
	case 0xa4:prefix##_##a4();break; case 0xa5:prefix##_##a5();break; case 0xa6:prefix##_##a6();break; case 0xa7:prefix##_##a7();break; \
	case 0xa8:prefix##_##a8();break; case 0xa9:prefix##_##a9();break; case 0xaa:prefix##_##aa();break; case 0xab:prefix##_##ab();break; \
	case 0xac:prefix##_##ac();break; case 0xad:prefix##_##ad();break; case 0xae:prefix##_##ae();break; case 0xaf:prefix##_##af();break; \
	case 0xb0:prefix##_##b0();break; case 0xb1:prefix##_##b1();break; case 0xb2:prefix##_##b2();break; case 0xb3:prefix##_##b3();break; \
	case 0xb4:prefix##_##b4();break; case 0xb5:prefix##_##b5();break; case 0xb6:prefix##_##b6();break; case 0xb7:prefix##_##b7();break; \
	case 0xb8:prefix##_##b8();break; case 0xb9:prefix##_##b9();break; case 0xba:prefix##_##ba();break; case 0xbb:prefix##_##bb();break; \
	case 0xbc:prefix##_##bc();break; case 0xbd:prefix##_##bd();break; case 0xbe:prefix##_##be();break; case 0xbf:prefix##_##bf();break; \
	case 0xc0:prefix##_##c0();break; case 0xc1:prefix##_##c1();break; case 0xc2:prefix##_##c2();break; case 0xc3:prefix##_##c3();break; \
	case 0xc4:prefix##_##c4();break; case 0xc5:prefix##_##c5();break; case 0xc6:prefix##_##c6();break; case 0xc7:prefix##_##c7();break; \
	case 0xc8:prefix##_##c8();break; case 0xc9:prefix##_##c9();break; case 0xca:prefix##_##ca();break; case 0xcb:prefix##_##cb();break; \
	case 0xcc:prefix##_##cc();break; case 0xcd:prefix##_##cd();break; case 0xce:prefix##_##ce();break; case 0xcf:prefix##_##cf();break; \
	case 0xd0:prefix##_##d0();break; case 0xd1:prefix##_##d1();break; case 0xd2:prefix##_##d2();break; case 0xd3:prefix##_##d3();break; \
	case 0xd4:prefix##_##d4();break; case 0xd5:prefix##_##d5();break; case 0xd6:prefix##_##d6();break; case 0xd7:prefix##_##d7();break; \
	case 0xd8:prefix##_##d8();break; case 0xd9:prefix##_##d9();break; case 0xda:prefix##_##da();break; case 0xdb:prefix##_##db();break; \
	case 0xdc:prefix##_##dc();break; case 0xdd:prefix##_##dd();break; case 0xde:prefix##_##de();break; case 0xdf:prefix##_##df();break; \
	case 0xe0:prefix##_##e0();break; case 0xe1:prefix##_##e1();break; case 0xe2:prefix##_##e2();break; case 0xe3:prefix##_##e3();break; \
	case 0xe4:prefix##_##e4();break; case 0xe5:prefix##_##e5();break; case 0xe6:prefix##_##e6();break; case 0xe7:prefix##_##e7();break; \
	case 0xe8:prefix##_##e8();break; case 0xe9:prefix##_##e9();break; case 0xea:prefix##_##ea();break; case 0xeb:prefix##_##eb();break; \
	case 0xec:prefix##_##ec();break; case 0xed:prefix##_##ed();break; case 0xee:prefix##_##ee();break; case 0xef:prefix##_##ef();break; \
	case 0xf0:prefix##_##f0();break; case 0xf1:prefix##_##f1();break; case 0xf2:prefix##_##f2();break; case 0xf3:prefix##_##f3();break; \
	case 0xf4:prefix##_##f4();break; case 0xf5:prefix##_##f5();break; case 0xf6:prefix##_##f6();break; case 0xf7:prefix##_##f7();break; \
	case 0xf8:prefix##_##f8();break; case 0xf9:prefix##_##f9();break; case 0xfa:prefix##_##fa();break; case 0xfb:prefix##_##fb();break; \
	case 0xfc:prefix##_##fc();break; case 0xfd:prefix##_##fd();break; case 0xfe:prefix##_##fe();break; case 0xff:prefix##_##ff();break; \
	}																																	\
}
#else
#define EXEC_INLINE EXEC
#endif


/***************************************************************
 * Enter HALT state; write 1 to fake port on first execution
 ***************************************************************/
#define ENTER_HALT {											\
	_PC--;														\
	_HALT = 1;													\
	if( !after_EI )												\
		z80_burn( z80_ICount );									\
}

/***************************************************************
 * Leave HALT state; write 0 to fake port
 ***************************************************************/
#define LEAVE_HALT {											\
	if( _HALT )													\
	{															\
		_HALT = 0;												\
		_PC++;													\
	}															\
}

/***************************************************************
 * Input a byte from given I/O port
 ***************************************************************/
#define IN(port)   ((UINT8)cpu_readport16(port))

/***************************************************************
 * Output a byte to given I/O port
 ***************************************************************/
#define OUT(port,value) cpu_writeport16(port,value)

/***************************************************************
 * Read a byte from given memory location
 ***************************************************************/
#define RM(addr) (UINT8)cpu_readmem16(addr)

/***************************************************************
 * Read a word from given memory location
 ***************************************************************/
INLINE void RM16( UINT32 addr, PAIR *r )
{
	r->b.l = RM(addr);
	r->b.h = RM((addr+1)&0xffff);
}

/***************************************************************
 * Write a byte to given memory location
 ***************************************************************/
#define WM(addr,value) cpu_writemem16(addr,value)

/***************************************************************
 * Write a word to given memory location
 ***************************************************************/
INLINE void WM16( UINT32 addr, PAIR *r )
{
	WM(addr,r->b.l);
	WM((addr+1)&0xffff,r->b.h);
}

/***************************************************************
 * ROP() is identical to RM() except it is used for
 * reading opcodes. In case of system with memory mapped I/O,
 * this function can be used to greatly speed up emulation
 ***************************************************************/
INLINE UINT8 ROP(void)
{
	unsigned pc = _PCD;
	_PC++;
	return cpu_readop(pc);
}

/****************************************************************
 * ARG() is identical to ROP() except it is used
 * for reading opcode arguments. This difference can be used to
 * support systems that use different encoding mechanisms for
 * opcodes and opcode arguments
 ***************************************************************/
INLINE UINT8 ARG(void)
{
	unsigned pc = _PCD;
	_PC++;
	return cpu_readop_arg(pc);
}

INLINE UINT32 ARG16(void)
{
	unsigned pc = _PCD;
	_PC += 2;
	return cpu_readop_arg(pc) | (cpu_readop_arg((pc+1)&0xffff) << 8);
}

/***************************************************************
 * Calculate the effective address EA of an opcode using
 * IX+offset resp. IY+offset addressing.
 ***************************************************************/
#define EAX EA = (UINT32)(UINT16)(_IX+(INT8)ARG())
#define EAY EA = (UINT32)(UINT16)(_IY+(INT8)ARG())

/***************************************************************
 * POP
 ***************************************************************/
#define POP(DR) { RM16( _SPD, &Z80.DR ); _SP += 2; }

/***************************************************************
 * PUSH
 ***************************************************************/
#define PUSH(SR) { _SP -= 2; WM16( _SPD, &Z80.SR ); }

/***************************************************************
 * JP
 ***************************************************************/
#if BUSY_LOOP_HACKS
#define JP {													\
	unsigned oldpc = _PCD-1;									\
	_PCD = ARG16();												\
	change_pc16(_PCD);											\
	/* speed up busy loop */									\
	if( _PCD == oldpc )											\
	{															\
		if( !after_EI )											\
			BURNODD( z80_ICount, 1, cc[Z80_TABLE_op][0xc3] );	\
	}															\
	else														\
	{															\
		UINT8 op = cpu_readop(_PCD);							\
		if( _PCD == oldpc-1 )									\
		{														\
			/* NOP - JP $-1 or EI - JP $-1 */					\
			if ( op == 0x00 || op == 0xfb )						\
			{													\
				if( !after_EI )									\
					BURNODD( z80_ICount-cc[Z80_TABLE_op][0x00], \
						2, cc[Z80_TABLE_op][0x00]+cc[Z80_TABLE_op][0xc3]); \
			}													\
		}														\
		else													\
		/* LD SP,#xxxx - JP $-3 (Galaga) */						\
		if( _PCD == oldpc-3 && op == 0x31 )						\
		{														\
			if( !after_EI )										\
				BURNODD( z80_ICount-cc[Z80_TABLE_op][0x31],		\
					2, cc[Z80_TABLE_op][0x31]+cc[Z80_TABLE_op][0xc3]); \
		}														\
	}															\
}
#else
#define JP {													\
	_PCD = ARG16();												\
	change_pc16(_PCD);											\
}
#endif

/***************************************************************
 * JP_COND
 ***************************************************************/

#define JP_COND(cond)											\
	if( cond )													\
	{															\
		_PCD = ARG16();											\
		change_pc16(_PCD);										\
	}															\
	else														\
	{															\
		_PC += 2;												\
	}

/***************************************************************
 * JR
 ***************************************************************/
#define JR()													\
{																\
	unsigned oldpc = _PCD-1;									\
	INT8 arg = (INT8)ARG(); /* ARG() also increments _PC */		\
	_PC += arg;				/* so don't do _PC += ARG() */		\
	change_pc16(_PCD);											\
	/* speed up busy loop */									\
	if( _PCD == oldpc )											\
	{															\
		if( !after_EI )											\
			BURNODD( z80_ICount, 1, cc[Z80_TABLE_op][0x18] );	\
	}															\
	else														\
	{															\
		UINT8 op = cpu_readop(_PCD);							\
		if( _PCD == oldpc-1 )									\
		{														\
			/* NOP - JR $-1 or EI - JR $-1 */					\
			if ( op == 0x00 || op == 0xfb )						\
			{													\
				if( !after_EI )									\
				   BURNODD( z80_ICount-cc[Z80_TABLE_op][0x00],	\
					   2, cc[Z80_TABLE_op][0x00]+cc[Z80_TABLE_op][0x18]); \
			}													\
		}														\
		else													\
		/* LD SP,#xxxx - JR $-3 */								\
		if( _PCD == oldpc-3 && op == 0x31 )						\
		{														\
			if( !after_EI )										\
			   BURNODD( z80_ICount-cc[Z80_TABLE_op][0x31],		\
				   2, cc[Z80_TABLE_op][0x31]+cc[Z80_TABLE_op][0x18]); \
		}														\
	}															\
}

/***************************************************************
 * JR_COND
 ***************************************************************/
#define JR_COND(cond,opcode)									\
	if( cond )													\
	{															\
		INT8 arg = (INT8)ARG(); /* ARG() also increments _PC */ \
		_PC += arg;				/* so don't do _PC += ARG() */	\
		CC(ex,opcode);											\
		change_pc16(_PCD);										\
	}															\
	else _PC++;													\

/***************************************************************
 * CALL
 ***************************************************************/
#define CALL()													\
	EA = ARG16();												\
	PUSH( PC );													\
	_PCD = EA;													\
	change_pc16(_PCD)

/***************************************************************
 * CALL_COND
 ***************************************************************/
#define CALL_COND(cond,opcode)									\
	if( cond )													\
	{															\
		EA = ARG16();											\
		PUSH( PC );												\
		_PCD = EA;												\
		CC(ex,opcode);											\
		change_pc16(_PCD);										\
	}															\
	else														\
	{															\
		_PC+=2;													\
	}

/***************************************************************
 * RET_COND
 ***************************************************************/
#define RET_COND(cond,opcode)									\
	if( cond )													\
	{															\
		POP(PC);												\
		change_pc16(_PCD);										\
		CC(ex,opcode);											\
	}

/***************************************************************
 * RETN
 ***************************************************************/
#define RETN	{												\
	POP(PC);													\
	change_pc16(_PCD);											\
	if( _IFF1 == 0 && _IFF2 == 1 )								\
	{															\
		_IFF1 = 1;												\
		if( Z80.irq_state != CLEAR_LINE ||						\
			Z80.request_irq >= 0 )								\
		{														\
			take_interrupt();									\
		}														\
	}															\
	else _IFF1 = _IFF2;											\
}

/***************************************************************
 * RETI
 ***************************************************************/
#define RETI	{												\
	int device = Z80.service_irq;								\
	POP(PC);													\
	change_pc16(_PCD);											\
/* according to http://www.msxnet.org/tech/z80-documented.pdf */\
/*	_IFF1 = _IFF2;	*/											\
	if( device >= 0 )											\
	{															\
		Z80.irq[device].interrupt_reti(Z80.irq[device].irq_param); \
	}															\
}

/***************************************************************
 * LD	R,A
 ***************************************************************/
#define LD_R_A {												\
	_Z80_R = _Z80_A;													\
	_R2 = _Z80_A & 0x80;				/* keep bit 7 of R */		\
}

/***************************************************************
 * LD	A,R
 ***************************************************************/
#define LD_A_R {												\
	_Z80_A = (_Z80_R & 0x7f) | _R2;										\
	_F = (_F & CF) | SZ[_Z80_A] | ( _IFF2 << 2 );					\
}

/***************************************************************
 * LD	I,A
 ***************************************************************/
#define LD_I_A {												\
	_Z80_I = _Z80_A;													\
}

/***************************************************************
 * LD	A,I
 ***************************************************************/
#define LD_A_I {												\
	_Z80_A = _Z80_I;													\
	_F = (_F & CF) | SZ[_Z80_A] | ( _IFF2 << 2 );					\
}

/***************************************************************
 * RST
 ***************************************************************/
#define RST(addr)												\
	PUSH( PC );													\
	_PCD = addr;												\
	change_pc16(_PCD)

/***************************************************************
 * INC	r8
 ***************************************************************/
INLINE UINT8 INC(UINT8 value)
{
	UINT8 res = value + 1;
	_F = (_F & CF) | SZHV_inc[res];
	return (UINT8)res;
}

/***************************************************************
 * DEC	r8
 ***************************************************************/
INLINE UINT8 DEC(UINT8 value)
{
	UINT8 res = value - 1;
	_F = (_F & CF) | SZHV_dec[res];
	return res;
}

/***************************************************************
 * RLCA
 ***************************************************************/
#if Z80_EXACT
#define RLCA													\
	_Z80_A = (_Z80_A << 1) | (_Z80_A >> 7);									\
	_F = (_F & (SF | ZF | PF)) | (_Z80_A & (YF | XF | CF))
#else
#define RLCA													\
	_Z80_A = (_Z80_A << 1) | (_Z80_A >> 7);									\
	_F = (_F & (SF | ZF | YF | XF | PF)) | (_Z80_A & CF)
#endif

/***************************************************************
 * RRCA
 ***************************************************************/
#if Z80_EXACT
#define RRCA													\
	_F = (_F & (SF | ZF | PF)) | (_Z80_A & CF);						\
	_Z80_A = (_Z80_A >> 1) | (_Z80_A << 7);									\
	_F |= (_Z80_A & (YF | XF) )
#else
#define RRCA													\
	_F = (_F & (SF | ZF | YF | XF | PF)) | (_Z80_A & CF);			\
	_Z80_A = (_Z80_A >> 1) | (_Z80_A << 7)
#endif

/***************************************************************
 * RLA
 ***************************************************************/
#if Z80_EXACT
#define RLA {													\
	UINT8 res = (_Z80_A << 1) | (_F & CF);							\
	UINT8 c = (_Z80_A & 0x80) ? CF : 0;								\
	_F = (_F & (SF | ZF | PF)) | c | (res & (YF | XF));			\
	_Z80_A = res;													\
}
#else
#define RLA {													\
	UINT8 res = (_Z80_A << 1) | (_F & CF);							\
	UINT8 c = (_Z80_A & 0x80) ? CF : 0;								\
	_F = (_F & (SF | ZF | YF | XF | PF)) | c;					\
	_Z80_A = res;													\
}
#endif

/***************************************************************
 * RRA
 ***************************************************************/
#if Z80_EXACT
#define RRA {													\
	UINT8 res = (_Z80_A >> 1) | (_F << 7);							\
	UINT8 c = (_Z80_A & 0x01) ? CF : 0;								\
	_F = (_F & (SF | ZF | PF)) | c | (res & (YF | XF));			\
	_Z80_A = res;													\
}
#else
#define RRA {													\
	UINT8 res = (_Z80_A >> 1) | (_F << 7);							\
	UINT8 c = (_Z80_A & 0x01) ? CF : 0;								\
	_F = (_F & (SF | ZF | YF | XF | PF)) | c;					\
	_Z80_A = res;													\
}
#endif

/***************************************************************
 * RRD
 ***************************************************************/
#define RRD {													\
	UINT8 n = RM(_HL);											\
	WM( _HL, (n >> 4) | (_Z80_A << 4) );							\
	_Z80_A = (_Z80_A & 0xf0) | (n & 0x0f);								\
	_F = (_F & CF) | SZP[_Z80_A];									\
}

/***************************************************************
 * RLD
 ***************************************************************/
#define RLD {													\
	UINT8 n = RM(_HL);											\
	WM( _HL, (n << 4) | (_Z80_A & 0x0f) );							\
	_Z80_A = (_Z80_A & 0xf0) | (n >> 4);								\
	_F = (_F & CF) | SZP[_Z80_A];									\
}

/***************************************************************
 * ADD	A,n
 ***************************************************************/
#ifdef X86_ASM
#if Z80_EXACT
#define ADD(value)												\
 asm (															\
 " addb %2,%0			\n"										\
 " lahf					\n"										\
 " setob %1				\n" /* al = 1 if overflow */			\
 " addb %1,%1			\n"										\
 " addb %1,%1			\n" /* shift to P/V bit position */		\
 " andb $0xd1,%%ah		\n" /* sign, zero, half carry, carry */ \
 " orb %%ah,%1			\n"										\
 " movb %0,%%ah			\n" /* get result */					\
 " andb $0x28,%%ah		\n" /* maks flags 5+3 */				\
 " orb %%ah,%1			\n" /* put them into flags */			\
 :"=q" (_Z80_A), "=q" (_F)											\
 :"q" (value), "1" (_F), "0" (_Z80_A)								\
 )
#else
#define ADD(value)												\
 asm (															\
 " addb %2,%0			\n"										\
 " lahf					\n"										\
 " setob %1				\n" /* al = 1 if overflow */			\
 " addb %1,%1			\n"										\
 " addb %1,%1			\n" /* shift to P/V bit position */		\
 " andb $0xd1,%%ah		\n" /* sign, zero, half carry, carry */ \
 " orb %%ah,%1			\n"										\
 :"=q" (_Z80_A), "=q" (_F)											\
 :"q" (value), "1" (_F), "0" (_Z80_A)								\
 )
#endif
#else
#if BIG_FLAGS_ARRAY
#define ADD(value)												\
{																\
	UINT32 ah = _AFD & 0xff00;									\
	UINT32 res = (UINT8)((ah >> 8) + value);					\
	_F = SZHVC_add[ah | res];									\
	_Z80_A = res;													\
}
#else
#define ADD(value)												\
{																\
	unsigned val = value;										\
	unsigned res = _Z80_A + val;									\
	_F = SZ[(UINT8)res] | ((res >> 8) & CF) |					\
		((_Z80_A ^ res ^ val) & HF) |								\
		(((val ^ _Z80_A ^ 0x80) & (val ^ res) & 0x80) >> 5);		\
	_Z80_A = (UINT8)res;											\
}
#endif
#endif

/***************************************************************
 * ADC	A,n
 ***************************************************************/
#ifdef X86_ASM
#if Z80_EXACT
#define ADC(value)												\
 asm (															\
 " shrb $1,%1			\n"										\
 " adcb %2,%0			\n"										\
 " lahf					\n"										\
 " setob %1				\n" /* al = 1 if overflow */			\
 " addb %1,%1			\n" /* shift to P/V bit position */		\
 " addb %1,%1			\n"										\
 " andb $0xd1,%%ah		\n" /* sign, zero, half carry, carry */ \
 " orb %%ah,%1			\n" /* combine with P/V */				\
 " movb %0,%%ah			\n" /* get result */					\
 " andb $0x28,%%ah		\n" /* maks flags 5+3 */				\
 " orb %%ah,%1			\n" /* put them into flags */			\
 :"=q" (_Z80_A), "=q" (_F)											\
 :"q" (value), "1" (_F), "0" (_Z80_A)								\
 )
#else
#define ADC(value)												\
 asm (															\
 " shrb $1,%1			\n"										\
 " adcb %2,%0			\n"										\
 " lahf					\n"										\
 " setob %1				\n" /* al = 1 if overflow */			\
 " addb %1,%1			\n" /* shift to P/V bit position */		\
 " addb %1,%1			\n"										\
 " andb $0xd1,%%ah		\n" /* sign, zero, half carry, carry */ \
 " orb %%ah,%1			\n" /* combine with P/V */				\
 :"=q" (_Z80_A), "=q" (_F)											\
 :"q" (value), "1" (_F), "0" (_Z80_A)								\
 )
#endif
#else
#if BIG_FLAGS_ARRAY
#define ADC(value)												\
{																\
	UINT32 ah = _AFD & 0xff00, c = _AFD & 1;					\
	UINT32 res = (UINT8)((ah >> 8) + value + c);				\
	_F = SZHVC_add[(c << 16) | ah | res];						\
	_Z80_A = res;													\
}
#else
#define ADC(value)												\
{																\
	unsigned val = value;										\
	unsigned res = _Z80_A + val + (_F & CF);						\
	_F = SZ[res & 0xff] | ((res >> 8) & CF) |					\
		((_Z80_A ^ res ^ val) & HF) |								\
		(((val ^ _Z80_A ^ 0x80) & (val ^ res) & 0x80) >> 5);		\
	_Z80_A = res;													\
}
#endif
#endif

/***************************************************************
 * SUB	n
 ***************************************************************/
#ifdef X86_ASM
#if Z80_EXACT
#define SUB(value)												\
 asm (															\
 " subb %2,%0			\n"										\
 " lahf					\n"										\
 " setob %1				\n" /* al = 1 if overflow */			\
 " stc					\n" /* prepare to set N flag */			\
 " adcb %1,%1			\n" /* shift to P/V bit position */		\
 " addb %1,%1			\n"										\
 " andb $0xd1,%%ah		\n" /* sign, zero, half carry, carry */ \
 " orb %%ah,%1			\n" /* combine with P/V */				\
 " movb %0,%%ah			\n" /* get result */					\
 " andb $0x28,%%ah		\n" /* maks flags 5+3 */				\
 " orb %%ah,%1			\n" /* put them into flags */			\
 :"=q" (_Z80_A), "=q" (_F)											\
 :"q" (value), "1" (_F), "0" (_Z80_A)								\
 )
#else
#define SUB(value)												\
 asm (															\
 " subb %2,%0			\n"										\
 " lahf					\n"										\
 " setob %1				\n" /* al = 1 if overflow */			\
 " stc					\n" /* prepare to set N flag */			\
 " adcb %1,%1			\n" /* shift to P/V bit position */		\
 " addb %1,%1			\n"										\
 " andb $0xd1,%%ah		\n" /* sign, zero, half carry, carry */ \
 " orb %%ah,%1			\n" /* combine with P/V */				\
 :"=q" (_Z80_A), "=q" (_F)											\
 :"q" (value), "1" (_F), "0" (_Z80_A)								\
 )
#endif
#else
#if BIG_FLAGS_ARRAY
#define SUB(value)												\
{																\
	UINT32 ah = _AFD & 0xff00;									\
	UINT32 res = (UINT8)((ah >> 8) - value);					\
	_F = SZHVC_sub[ah | res];									\
	_Z80_A = res;													\
}
#else
#define SUB(value)												\
{																\
	unsigned val = value;										\
	unsigned res = _Z80_A - val;									\
	_F = SZ[res & 0xff] | ((res >> 8) & CF) | NF |				\
		((_Z80_A ^ res ^ val) & HF) |								\
		(((val ^ _Z80_A) & (_Z80_A ^ res) & 0x80) >> 5);				\
	_Z80_A = res;													\
}
#endif
#endif

/***************************************************************
 * SBC	A,n
 ***************************************************************/
#ifdef X86_ASM
#if Z80_EXACT
#define SBC(value)												\
 asm (															\
 " shrb $1,%1			\n"										\
 " sbbb %2,%0			\n"										\
 " lahf					\n"										\
 " setob %1				\n" /* al = 1 if overflow */			\
 " stc					\n" /* prepare to set N flag */			\
 " adcb %1,%1			\n" /* shift to P/V bit position */		\
 " addb %1,%1			\n"										\
 " andb $0xd1,%%ah		\n" /* sign, zero, half carry, carry */ \
 " orb %%ah,%1			\n" /* combine with P/V */				\
 " movb %0,%%ah			\n" /* get result */					\
 " andb $0x28,%%ah		\n" /* maks flags 5+3 */				\
 " orb %%ah,%1			\n" /* put them into flags */			\
 :"=q" (_Z80_A), "=q" (_F)											\
 :"q" (value), "1" (_F), "0" (_Z80_A)								\
 )
#else
#define SBC(value)												\
 asm (															\
 " shrb $1,%1			\n"										\
 " sbbb %2,%0			\n"										\
 " lahf					\n"										\
 " setob %1				\n" /* al = 1 if overflow */			\
 " stc					\n" /* prepare to set N flag */			\
 " adcb %1,%1			\n" /* shift to P/V bit position */		\
 " addb %1,%1			\n"										\
 " andb $0xd1,%%ah		\n" /* sign, zero, half carry, carry */ \
 " orb %%ah,%1			\n" /* combine with P/V */				\
 :"=q" (_Z80_A), "=q" (_F)											\
 :"q" (value), "1" (_F), "0" (_Z80_A)								\
 )
#endif
#else
#if BIG_FLAGS_ARRAY
#define SBC(value)												\
{																\
	UINT32 ah = _AFD & 0xff00, c = _AFD & 1;					\
	UINT32 res = (UINT8)((ah >> 8) - value - c);				\
	_F = SZHVC_sub[(c<<16) | ah | res];							\
	_Z80_A = res;													\
}
#else
#define SBC(value)												\
{																\
	unsigned val = value;										\
	unsigned res = _Z80_A - val - (_F & CF);						\
	_F = SZ[res & 0xff] | ((res >> 8) & CF) | NF |				\
		((_Z80_A ^ res ^ val) & HF) |								\
		(((val ^ _Z80_A) & (_Z80_A ^ res) & 0x80) >> 5);				\
	_Z80_A = res;													\
}
#endif
#endif

/***************************************************************
 * NEG
 ***************************************************************/
#define NEG {													\
	UINT8 value = _Z80_A;											\
	_Z80_A = 0;														\
	SUB(value);													\
}

/***************************************************************
 * DAA
 ***************************************************************/
#define DAA {													\
	UINT8 cf, nf, hf, lo, hi, diff;								\
	cf = _F & CF;												\
	nf = _F & NF;												\
	hf = _F & HF;												\
	lo = _Z80_A & 15;												\
	hi = _Z80_A / 16;												\
																\
	if (cf)														\
	{															\
		diff = (lo <= 9 && !hf) ? 0x60 : 0x66;					\
	}															\
	else														\
	{															\
		if (lo >= 10)											\
		{														\
			diff = hi <= 8 ? 0x06 : 0x66;						\
		}														\
		else													\
		{														\
			if (hi >= 10)										\
			{													\
				diff = hf ? 0x66 : 0x60;						\
			}													\
			else												\
			{													\
				diff = hf ? 0x06 : 0x00;						\
			}													\
		}														\
	}															\
	if (nf) _Z80_A -= diff;											\
	else _Z80_A += diff;											\
																\
	_F = SZP[_Z80_A] | (_F & NF);									\
	if (cf || (lo <= 9 ? hi >= 10 : hi >= 9)) _F |= CF;			\
	if (nf ? hf && lo <= 5 : lo >= 10)	_F |= HF;				\
}

/***************************************************************
 * AND	n
 ***************************************************************/
#define AND(value)												\
	_Z80_A &= value;												\
	_F = SZP[_Z80_A] | HF

/***************************************************************
 * OR	n
 ***************************************************************/
#define OR(value)												\
	_Z80_A |= value;												\
	_F = SZP[_Z80_A]

/***************************************************************
 * XOR	n
 ***************************************************************/
#define XOR(value)												\
	_Z80_A ^= value;												\
	_F = SZP[_Z80_A]

/***************************************************************
 * CP	n
 ***************************************************************/
#ifdef X86_ASM
#if Z80_EXACT
#define CP(value)												\
 asm (															\
 " cmpb %2,%0			\n"										\
 " lahf					\n"										\
 " setob %1				\n" /* al = 1 if overflow */			\
 " stc					\n" /* prepare to set N flag */			\
 " adcb %1,%1			\n" /* shift to P/V bit position */		\
 " addb %1,%1			\n"										\
 " andb $0xd1,%%ah		\n" /* sign, zero, half carry, carry */ \
 " orb %%ah,%1			\n" /* combine with P/V */				\
 " movb %2,%%ah			\n" /* get result */					\
 " andb $0x28,%%ah		\n" /* maks flags 5+3 */				\
 " orb %%ah,%1			\n" /* put them into flags */			\
 :"=q" (_Z80_A), "=q" (_F)											\
 :"q" (value), "1" (_F), "0" (_Z80_A)								\
 )
#else
#define CP(value)												\
 asm (															\
 " cmpb %2,%0			\n"										\
 " lahf					\n"										\
 " setob %1				\n" /* al = 1 if overflow */			\
 " stc					\n" /* prepare to set N flag */			\
 " adcb %1,%1			\n" /* shift to P/V bit position */		\
 " addb %1,%1			\n"										\
 " andb $0xd1,%%ah		\n" /* sign, zero, half carry, carry */ \
 " orb %%ah,%1			\n" /* combine with P/V */				\
 :"=q" (_Z80_A), "=q" (_F)											\
 :"q" (value), "1" (_F), "0" (_Z80_A)								\
 )
#endif
#else
#if BIG_FLAGS_ARRAY
#define CP(value)												\
{																\
	unsigned val = value;										\
	UINT32 ah = _AFD & 0xff00;									\
	UINT32 res = (UINT8)((ah >> 8) - val);						\
	_F = (SZHVC_sub[ah | res] & ~(YF | XF)) |					\
		(val & (YF | XF));										\
}
#else
#define CP(value)												\
{																\
	unsigned val = value;										\
	unsigned res = _Z80_A - val;									\
	_F = (SZ[res & 0xff] & (SF | ZF)) |							\
		(val & (YF | XF)) | ((res >> 8) & CF) | NF |			\
		((_Z80_A ^ res ^ val) & HF) |								\
		((((val ^ _Z80_A) & (_Z80_A ^ res)) >> 5) & VF);				\
}
#endif
#endif

/***************************************************************
 * EX	AF,AF'
 ***************************************************************/
#define EX_AF {													\
	PAIR tmp;													\
	tmp = Z80.AF; Z80.AF = Z80.AF2; Z80.AF2 = tmp;				\
}

/***************************************************************
 * EX	DE,HL
 ***************************************************************/
#define EX_DE_HL {												\
	PAIR tmp;													\
	tmp = Z80.DE; Z80.DE = Z80.HL; Z80.HL = tmp;				\
}

/***************************************************************
 * EXX
 ***************************************************************/
#define EXX {													\
	PAIR tmp;													\
	tmp = Z80.BC; Z80.BC = Z80.BC2; Z80.BC2 = tmp;				\
	tmp = Z80.DE; Z80.DE = Z80.DE2; Z80.DE2 = tmp;				\
	tmp = Z80.HL; Z80.HL = Z80.HL2; Z80.HL2 = tmp;				\
}

/***************************************************************
 * EX	(SP),r16
 ***************************************************************/
#define EXSP(DR)												\
{																\
	PAIR tmp = { { 0, 0, 0, 0 } };								\
	RM16( _SPD, &tmp );											\
	WM16( _SPD, &Z80.DR );										\
	Z80.DR = tmp;												\
}


/***************************************************************
 * ADD16
 ***************************************************************/
#ifdef	X86_ASM
#if Z80_EXACT
#define ADD16(DR,SR)											\
 asm (															\
 " andb $0xc4,%1		\n"										\
 " addb %%dl,%%cl		\n"										\
 " adcb %%dh,%%ch		\n"										\
 " lahf					\n"										\
 " andb $0x11,%%ah		\n"										\
 " orb %%ah,%1			\n"										\
 " movb %%ch,%%ah		\n" /* get result MSB */				\
 " andb $0x28,%%ah		\n" /* maks flags 5+3 */				\
 " orb %%ah,%1			\n" /* put them into flags */			\
 :"=c" (Z80.DR.d), "=q" (_F)									\
 :"0" (Z80.DR.d), "1" (_F), "d" (Z80.SR.d)						\
 )
#else
#define ADD16(DR,SR)											\
 asm (															\
 " andb $0xc4,%1		\n"										\
 " addb %%dl,%%cl		\n"										\
 " adcb %%dh,%%ch		\n"										\
 " lahf					\n"										\
 " andb $0x11,%%ah		\n"										\
 " orb %%ah,%1			\n"										\
 :"=c" (Z80.DR.d), "=q" (_F)									\
 :"0" (Z80.DR.d), "1" (_F), "d" (Z80.SR.d)						\
 )
#endif
#else
#define ADD16(DR,SR)											\
{																\
	UINT32 res = Z80.DR.d + Z80.SR.d;							\
	_F = (_F & (SF | ZF | VF)) |								\
		(((Z80.DR.d ^ res ^ Z80.SR.d) >> 8) & HF) |				\
		((res >> 16) & CF) | ((res >> 8) & (YF | XF));			\
	Z80.DR.w.l = (UINT16)res;									\
}
#endif

/***************************************************************
 * ADC	r16,r16
 ***************************************************************/
#ifdef	X86_ASM
#if Z80_EXACT
#define ADC16(Reg)												\
 asm (															\
 " shrb $1,%1			\n"										\
 " adcb %%dl,%%cl		\n"										\
 " lahf					\n"										\
 " movb %%ah,%%dl		\n"										\
 " adcb %%dh,%%ch		\n"										\
 " lahf					\n"										\
 " setob %1				\n"										\
 " orb $0xbf,%%dl		\n" /* set all but zero */				\
 " addb %1,%1			\n"										\
 " andb $0xd1,%%ah		\n" /* sign,zero,half carry and carry */\
 " addb %1,%1			\n"										\
 " orb %%ah,%1			\n" /* overflow into P/V */				\
 " andb %%dl,%1			\n" /* mask zero */						\
 " movb %%ch,%%ah		\n" /* get result MSB */				\
 " andb $0x28,%%ah		\n" /* maks flags 5+3 */				\
 " orb %%ah,%1			\n" /* put them into flags */			\
 :"=c" (_HLD), "=q" (_F)										\
 :"0" (_HLD), "1" (_F), "d" (Z80.Reg.d)							\
 )
#else
#define ADC16(Reg)												\
 asm (															\
 " shrb $1,%1			\n"										\
 " adcb %%dl,%%cl		\n"										\
 " lahf					\n"										\
 " movb %%ah,%%dl		\n"										\
 " adcb %%dh,%%ch		\n"										\
 " lahf					\n"										\
 " setob %1				\n"										\
 " orb $0xbf,%%dl		\n" /* set all but zero */				\
 " addb %1,%1			\n"										\
 " andb $0xd1,%%ah		\n" /* sign,zero,half carry and carry */\
 " addb %1,%1			\n"										\
 " orb %%ah,%1			\n" /* overflow into P/V */				\
 " andb %%dl,%1			\n" /* mask zero */						\
 :"=c" (_HLD), "=q" (_F)										\
 :"0" (_HLD), "1" (_F), "d" (Z80.Reg.d)							\
 )
#endif
#else
#define ADC16(Reg)												\
{																\
	UINT32 res = _HLD + Z80.Reg.d + (_F & CF);					\
	_F = (((_HLD ^ res ^ Z80.Reg.d) >> 8) & HF) |				\
		((res >> 16) & CF) |									\
		((res >> 8) & (SF | YF | XF)) |							\
		((res & 0xffff) ? 0 : ZF) |								\
		(((Z80.Reg.d ^ _HLD ^ 0x8000) & (Z80.Reg.d ^ res) & 0x8000) >> 13); \
	_HL = (UINT16)res;											\
}
#endif

/***************************************************************
 * SBC	r16,r16
 ***************************************************************/
#ifdef	X86_ASM
#if Z80_EXACT
#define SBC16(Reg)												\
asm (															\
 " shrb $1,%1			\n"										\
 " sbbb %%dl,%%cl		\n"										\
 " lahf					\n"										\
 " movb %%ah,%%dl		\n"										\
 " sbbb %%dh,%%ch		\n"										\
 " lahf					\n"										\
 " setob %1				\n"										\
 " orb $0xbf,%%dl		\n" /* set all but zero */				\
 " stc					\n"										\
 " adcb %1,%1			\n"										\
 " andb $0xd1,%%ah		\n" /* sign,zero,half carry and carry */\
 " addb %1,%1			\n"										\
 " orb %%ah,%1			\n" /* overflow into P/V */				\
 " andb %%dl,%1			\n" /* mask zero */						\
 " movb %%ch,%%ah		\n" /* get result MSB */				\
 " andb $0x28,%%ah		\n" /* maks flags 5+3 */				\
 " orb %%ah,%1			\n" /* put them into flags */			\
 :"=c" (_HLD), "=q" (_F)										\
 :"0" (_HLD), "1" (_F), "d" (Z80.Reg.d)							\
 )
#else
#define SBC16(Reg)												\
asm (															\
 " shrb $1,%1			\n"										\
 " sbbb %%dl,%%cl		\n"										\
 " lahf					\n"										\
 " movb %%ah,%%dl		\n"										\
 " sbbb %%dh,%%ch		\n"										\
 " lahf					\n"										\
 " setob %1				\n"										\
 " orb $0xbf,%%dl		\n" /* set all but zero */				\
 " stc					\n"										\
 " adcb %1,%1			\n"										\
 " andb $0xd1,%%ah		\n" /* sign,zero,half carry and carry */\
 " addb %1,%1			\n"										\
 " orb %%ah,%1			\n" /* overflow into P/V */				\
 " andb %%dl,%1			\n" /* mask zero */						\
 :"=c" (_HLD), "=q" (_F)										\
 :"0" (_HLD), "1" (_F), "d" (Z80.Reg.d)							\
 )
#endif
#else
#define SBC16(Reg)												\
{																\
	UINT32 res = _HLD - Z80.Reg.d - (_F & CF);					\
	_F = (((_HLD ^ res ^ Z80.Reg.d) >> 8) & HF) | NF |			\
		((res >> 16) & CF) |									\
		((res >> 8) & (SF | YF | XF)) |							\
		((res & 0xffff) ? 0 : ZF) |								\
		(((Z80.Reg.d ^ _HLD) & (_HLD ^ res) &0x8000) >> 13);	\
	_HL = (UINT16)res;											\
}
#endif

/***************************************************************
 * RLC	r8
 ***************************************************************/
INLINE UINT8 RLC(UINT8 value)
{
	unsigned res = value;
	unsigned c = (res & 0x80) ? CF : 0;
	res = ((res << 1) | (res >> 7)) & 0xff;
	_F = SZP[res] | c;
	return res;
}

/***************************************************************
 * RRC	r8
 ***************************************************************/
INLINE UINT8 RRC(UINT8 value)
{
	unsigned res = value;
	unsigned c = (res & 0x01) ? CF : 0;
	res = ((res >> 1) | (res << 7)) & 0xff;
	_F = SZP[res] | c;
	return res;
}

/***************************************************************
 * RL	r8
 ***************************************************************/
INLINE UINT8 RL(UINT8 value)
{
	unsigned res = value;
	unsigned c = (res & 0x80) ? CF : 0;
	res = ((res << 1) | (_F & CF)) & 0xff;
	_F = SZP[res] | c;
	return res;
}

/***************************************************************
 * RR	r8
 ***************************************************************/
INLINE UINT8 RR(UINT8 value)
{
	unsigned res = value;
	unsigned c = (res & 0x01) ? CF : 0;
	res = ((res >> 1) | (_F << 7)) & 0xff;
	_F = SZP[res] | c;
	return res;
}

/***************************************************************
 * SLA	r8
 ***************************************************************/
INLINE UINT8 SLA(UINT8 value)
{
	unsigned res = value;
	unsigned c = (res & 0x80) ? CF : 0;
	res = (res << 1) & 0xff;
	_F = SZP[res] | c;
	return res;
}

/***************************************************************
 * SRA	r8
 ***************************************************************/
INLINE UINT8 SRA(UINT8 value)
{
	unsigned res = value;
	unsigned c = (res & 0x01) ? CF : 0;
	res = ((res >> 1) | (res & 0x80)) & 0xff;
	_F = SZP[res] | c;
	return res;
}

/***************************************************************
 * SLL	r8
 ***************************************************************/
INLINE UINT8 SLL(UINT8 value)
{
	unsigned res = value;
	unsigned c = (res & 0x80) ? CF : 0;
	res = ((res << 1) | 0x01) & 0xff;
	_F = SZP[res] | c;
	return res;
}

/***************************************************************
 * SRL	r8
 ***************************************************************/
INLINE UINT8 SRL(UINT8 value)
{
	unsigned res = value;
	unsigned c = (res & 0x01) ? CF : 0;
	res = (res >> 1) & 0xff;
	_F = SZP[res] | c;
	return res;
}

/***************************************************************
 * BIT	bit,r8
 ***************************************************************/
#undef BIT
#define BIT(bit,reg)											\
	_F = (_F & CF) | HF | SZ_BIT[reg & (1<<bit)]

/***************************************************************
 * BIT	bit,(IX/Y+o)
 ***************************************************************/
#if Z80_EXACT
#define BIT_XY(bit,reg)											\
	_F = (_F & CF) | HF | (SZ_BIT[reg & (1<<bit)] & ~(YF|XF)) | ((EA>>8) & (YF|XF))
#else
#define BIT_XY	BIT
#endif

/***************************************************************
 * RES	bit,r8
 ***************************************************************/
INLINE UINT8 RES(UINT8 bit, UINT8 value)
{
	return value & ~(1<<bit);
}

/***************************************************************
 * SET	bit,r8
 ***************************************************************/
INLINE UINT8 SET(UINT8 bit, UINT8 value)
{
	return value | (1<<bit);
}

/***************************************************************
 * LDI
 ***************************************************************/
#if Z80_EXACT
#define LDI {													\
	UINT8 io = RM(_HL);											\
	WM( _DE, io );												\
	_F &= SF | ZF | CF;											\
	if( (_Z80_A + io) & 0x02 ) _F |= YF; /* bit 1 -> flag 5 */		\
	if( (_Z80_A + io) & 0x08 ) _F |= XF; /* bit 3 -> flag 3 */		\
	_HL++; _DE++; _BC--;										\
	if( _BC ) _F |= VF;											\
}
#else
#define LDI {													\
	WM( _DE, RM(_HL) );											\
	_F &= SF | ZF | YF | XF | CF;								\
	_HL++; _DE++; _BC--;										\
	if( _BC ) _F |= VF;											\
}
#endif

/***************************************************************
 * CPI
 ***************************************************************/
#if Z80_EXACT
#define CPI {													\
	UINT8 val = RM(_HL);										\
	UINT8 res = _Z80_A - val;										\
	_HL++; _BC--;												\
	_F = (_F & CF) | (SZ[res] & ~(YF|XF)) | ((_Z80_A ^ val ^ res) & HF) | NF;  \
	if( _F & HF ) res -= 1;										\
	if( res & 0x02 ) _F |= YF; /* bit 1 -> flag 5 */			\
	if( res & 0x08 ) _F |= XF; /* bit 3 -> flag 3 */			\
	if( _BC ) _F |= VF;											\
}
#else
#define CPI {													\
	UINT8 val = RM(_HL);										\
	UINT8 res = _Z80_A - val;										\
	_HL++; _BC--;												\
	_F = (_F & CF) | SZ[res] | ((_Z80_A ^ val ^ res) & HF) | NF;	\
	if( _BC ) _F |= VF;											\
}
#endif

/***************************************************************
 * INI
 ***************************************************************/
#if Z80_EXACT
#define INI {													\
	unsigned t;													\
	UINT8 io = IN(_BC);											\
	_Z80_B--;														\
	WM( _HL, io );												\
	_HL++;														\
	_F = SZ[_Z80_B];												\
	t = (unsigned)((_Z80_C + 1) & 0xff) + (unsigned)io;				\
	if( io & SF ) _F |= NF;										\
	if( t & 0x100 ) _F |= HF | CF;								\
	_F |= SZP[(UINT8)(t & 0x07) ^ _Z80_B] & PF;						\
}
#else
#define INI {													\
	_Z80_B--;														\
	WM( _HL, IN(_BC) );											\
	_HL++;														\
	_F = (_Z80_B) ? NF : NF | ZF;									\
}
#endif

/***************************************************************
 * OUTI
 ***************************************************************/
#if Z80_EXACT
#define OUTI {													\
	unsigned t;													\
	UINT8 io = RM(_HL);											\
	_Z80_B--;														\
	OUT( _BC, io );												\
	_HL++;														\
	_F = SZ[_Z80_B];												\
	t = (unsigned)_Z80_L + (unsigned)io;							\
	if( io & SF ) _F |= NF;										\
	if( t & 0x100 ) _F |= HF | CF;								\
	_F |= SZP[(UINT8)(t & 0x07) ^ _Z80_B] & PF;						\
}
#else
#define OUTI {													\
	_Z80_B--;														\
	OUT( _BC, RM(_HL) );										\
	_HL++;														\
	_F = (_Z80_B) ? NF : NF | ZF;									\
}
#endif

/***************************************************************
 * LDD
 ***************************************************************/
#if Z80_EXACT
#define LDD {													\
	UINT8 io = RM(_HL);											\
	WM( _DE, io );												\
	_F &= SF | ZF | CF;											\
	if( (_Z80_A + io) & 0x02 ) _F |= YF; /* bit 1 -> flag 5 */		\
	if( (_Z80_A + io) & 0x08 ) _F |= XF; /* bit 3 -> flag 3 */		\
	_HL--; _DE--; _BC--;										\
	if( _BC ) _F |= VF;											\
}
#else
#define LDD {													\
	WM( _DE, RM(_HL) );											\
	_F &= SF | ZF | YF | XF | CF;								\
	_HL--; _DE--; _BC--;										\
	if( _BC ) _F |= VF;											\
}
#endif

/***************************************************************
 * CPD
 ***************************************************************/
#if Z80_EXACT
#define CPD {													\
	UINT8 val = RM(_HL);										\
	UINT8 res = _Z80_A - val;										\
	_HL--; _BC--;												\
	_F = (_F & CF) | (SZ[res] & ~(YF|XF)) | ((_Z80_A ^ val ^ res) & HF) | NF;  \
	if( _F & HF ) res -= 1;										\
	if( res & 0x02 ) _F |= YF; /* bit 1 -> flag 5 */			\
	if( res & 0x08 ) _F |= XF; /* bit 3 -> flag 3 */			\
	if( _BC ) _F |= VF;											\
}
#else
#define CPD {													\
	UINT8 val = RM(_HL);										\
	UINT8 res = _Z80_A - val;										\
	_HL--; _BC--;												\
	_F = (_F & CF) | SZ[res] | ((_Z80_A ^ val ^ res) & HF) | NF;	\
	if( _BC ) _F |= VF;											\
}
#endif

/***************************************************************
 * IND
 ***************************************************************/
#if Z80_EXACT
#define IND {													\
	unsigned t;													\
	UINT8 io = IN(_BC);											\
	_Z80_B--;														\
	WM( _HL, io );												\
	_HL--;														\
	_F = SZ[_Z80_B];												\
	t = ((unsigned)(_Z80_C - 1) & 0xff) + (unsigned)io;				\
	if( io & SF ) _F |= NF;										\
	if( t & 0x100 ) _F |= HF | CF;								\
	_F |= SZP[(UINT8)(t & 0x07) ^ _Z80_B] & PF;						\
}
#else
#define IND {													\
	_Z80_B--;														\
	WM( _HL, IN(_BC) );											\
	_HL--;														\
	_F = (_Z80_B) ? NF : NF | ZF;									\
}
#endif

/***************************************************************
 * OUTD
 ***************************************************************/
#if Z80_EXACT
#define OUTD {													\
	unsigned t;													\
	UINT8 io = RM(_HL);											\
	_Z80_B--;														\
	OUT( _BC, io );												\
	_HL--;														\
	_F = SZ[_Z80_B];												\
	t = (unsigned)_Z80_L + (unsigned)io;							\
	if( io & SF ) _F |= NF;										\
	if( t & 0x100 ) _F |= HF | CF;								\
	_F |= SZP[(UINT8)(t & 0x07) ^ _Z80_B] & PF;						\
}
#else
#define OUTD {													\
	_Z80_B--;														\
	OUT( _BC, RM(_HL) );										\
	_HL--;														\
	_F = (_Z80_B) ? NF : NF | ZF;									\
}
#endif

/***************************************************************
 * LDIR
 ***************************************************************/
#define LDIR													\
	LDI;														\
	if( _BC )													\
	{															\
		_PC -= 2;												\
		CC(ex,0xb0);											\
	}

/***************************************************************
 * CPIR
 ***************************************************************/
#define CPIR													\
	CPI;														\
	if( _BC && !(_F & ZF) )										\
	{															\
		_PC -= 2;												\
		CC(ex,0xb1);											\
	}

/***************************************************************
 * INIR
 ***************************************************************/
#define INIR													\
	INI;														\
	if( _Z80_B )													\
	{															\
		_PC -= 2;												\
		CC(ex,0xb2);											\
	}

/***************************************************************
 * OTIR
 ***************************************************************/
#define OTIR													\
	OUTI;														\
	if( _Z80_B )													\
	{															\
		_PC -= 2;												\
		CC(ex,0xb3);											\
	}

/***************************************************************
 * LDDR
 ***************************************************************/
#define LDDR													\
	LDD;														\
	if( _BC )													\
	{															\
		_PC -= 2;												\
		CC(ex,0xb8);											\
	}

/***************************************************************
 * CPDR
 ***************************************************************/
#define CPDR													\
	CPD;														\
	if( _BC && !(_F & ZF) )										\
	{															\
		_PC -= 2;												\
		CC(ex,0xb9);											\
	}

/***************************************************************
 * INDR
 ***************************************************************/
#define INDR													\
	IND;														\
	if( _Z80_B )													\
	{															\
		_PC -= 2;												\
		CC(ex,0xba);											\
	}

/***************************************************************
 * OTDR
 ***************************************************************/
#define OTDR													\
	OUTD;														\
	if( _Z80_B )													\
	{															\
		_PC -= 2;												\
		CC(ex,0xbb);											\
	}

/***************************************************************
 * EI
 ***************************************************************/
#define EI {													\
	/* If interrupts were disabled, execute one more			\
	 * instruction and check the IRQ line.						\
	 * If not, simply set interrupt flip-flop 2					\
	 */															\
	if( _IFF1 == 0 )											\
	{															\
		_IFF1 = _IFF2 = 1;										\
		_PPC = _PCD;											\
		CALL_MAME_DEBUG;										\
		_Z80_R++;													\
		while( cpu_readop(_PCD) == 0xfb ) /* more EIs? */		\
		{														\
			CC(op,0xfb);										\
			_PPC =_PCD;											\
			CALL_MAME_DEBUG;									\
			_PC++;												\
			_Z80_R++;												\
		}														\
		if( Z80.irq_state != CLEAR_LINE ||						\
			Z80.request_irq >= 0 )								\
		{														\
			after_EI = 1;	/* avoid cycle skip hacks */		\
			EXEC(op,ROP());										\
			after_EI = 0;										\
			take_interrupt();									\
		} else EXEC(op,ROP());									\
	} else _IFF2 = 1;											\
}

/**********************************************************
 * opcodes with CB prefix
 * rotate, shift and bit operations
 **********************************************************/
OP(cb,00) { _Z80_B = RLC(_Z80_B);											} /* RLC  B			  */
OP(cb,01) { _Z80_C = RLC(_Z80_C);											} /* RLC  C			  */
OP(cb,02) { _Z80_D = RLC(_Z80_D);											} /* RLC  D			  */
OP(cb,03) { _Z80_E = RLC(_Z80_E);											} /* RLC  E			  */
OP(cb,04) { _Z80_H = RLC(_Z80_H);											} /* RLC  H			  */
OP(cb,05) { _Z80_L = RLC(_Z80_L);											} /* RLC  L			  */
OP(cb,06) { WM( _HL, RLC(RM(_HL)) );								} /* RLC  (HL)		  */
OP(cb,07) { _Z80_A = RLC(_Z80_A);											} /* RLC  A			  */

OP(cb,08) { _Z80_B = RRC(_Z80_B);											} /* RRC  B			  */
OP(cb,09) { _Z80_C = RRC(_Z80_C);											} /* RRC  C			  */
OP(cb,0a) { _Z80_D = RRC(_Z80_D);											} /* RRC  D			  */
OP(cb,0b) { _Z80_E = RRC(_Z80_E);											} /* RRC  E			  */
OP(cb,0c) { _Z80_H = RRC(_Z80_H);											} /* RRC  H			  */
OP(cb,0d) { _Z80_L = RRC(_Z80_L);											} /* RRC  L			  */
OP(cb,0e) { WM( _HL, RRC(RM(_HL)) );								} /* RRC  (HL)		  */
OP(cb,0f) { _Z80_A = RRC(_Z80_A);											} /* RRC  A			  */

OP(cb,10) { _Z80_B = RL(_Z80_B);											} /* RL   B			  */
OP(cb,11) { _Z80_C = RL(_Z80_C);											} /* RL   C			  */
OP(cb,12) { _Z80_D = RL(_Z80_D);											} /* RL   D			  */
OP(cb,13) { _Z80_E = RL(_Z80_E);											} /* RL   E			  */
OP(cb,14) { _Z80_H = RL(_Z80_H);											} /* RL   H			  */
OP(cb,15) { _Z80_L = RL(_Z80_L);											} /* RL   L			  */
OP(cb,16) { WM( _HL, RL(RM(_HL)) );									} /* RL   (HL)		  */
OP(cb,17) { _Z80_A = RL(_Z80_A);											} /* RL   A			  */

OP(cb,18) { _Z80_B = RR(_Z80_B);											} /* RR   B			  */
OP(cb,19) { _Z80_C = RR(_Z80_C);											} /* RR   C			  */
OP(cb,1a) { _Z80_D = RR(_Z80_D);											} /* RR   D			  */
OP(cb,1b) { _Z80_E = RR(_Z80_E);											} /* RR   E			  */
OP(cb,1c) { _Z80_H = RR(_Z80_H);											} /* RR   H			  */
OP(cb,1d) { _Z80_L = RR(_Z80_L);											} /* RR   L			  */
OP(cb,1e) { WM( _HL, RR(RM(_HL)) );									} /* RR   (HL)		  */
OP(cb,1f) { _Z80_A = RR(_Z80_A);											} /* RR   A			  */

OP(cb,20) { _Z80_B = SLA(_Z80_B);											} /* SLA  B			  */
OP(cb,21) { _Z80_C = SLA(_Z80_C);											} /* SLA  C			  */
OP(cb,22) { _Z80_D = SLA(_Z80_D);											} /* SLA  D			  */
OP(cb,23) { _Z80_E = SLA(_Z80_E);											} /* SLA  E			  */
OP(cb,24) { _Z80_H = SLA(_Z80_H);											} /* SLA  H			  */
OP(cb,25) { _Z80_L = SLA(_Z80_L);											} /* SLA  L			  */
OP(cb,26) { WM( _HL, SLA(RM(_HL)) );								} /* SLA  (HL)		  */
OP(cb,27) { _Z80_A = SLA(_Z80_A);											} /* SLA  A			  */

OP(cb,28) { _Z80_B = SRA(_Z80_B);											} /* SRA  B			  */
OP(cb,29) { _Z80_C = SRA(_Z80_C);											} /* SRA  C			  */
OP(cb,2a) { _Z80_D = SRA(_Z80_D);											} /* SRA  D			  */
OP(cb,2b) { _Z80_E = SRA(_Z80_E);											} /* SRA  E			  */
OP(cb,2c) { _Z80_H = SRA(_Z80_H);											} /* SRA  H			  */
OP(cb,2d) { _Z80_L = SRA(_Z80_L);											} /* SRA  L			  */
OP(cb,2e) { WM( _HL, SRA(RM(_HL)) );								} /* SRA  (HL)		  */
OP(cb,2f) { _Z80_A = SRA(_Z80_A);											} /* SRA  A			  */

OP(cb,30) { _Z80_B = SLL(_Z80_B);											} /* SLL  B			  */
OP(cb,31) { _Z80_C = SLL(_Z80_C);											} /* SLL  C			  */
OP(cb,32) { _Z80_D = SLL(_Z80_D);											} /* SLL  D			  */
OP(cb,33) { _Z80_E = SLL(_Z80_E);											} /* SLL  E			  */
OP(cb,34) { _Z80_H = SLL(_Z80_H);											} /* SLL  H			  */
OP(cb,35) { _Z80_L = SLL(_Z80_L);											} /* SLL  L			  */
OP(cb,36) { WM( _HL, SLL(RM(_HL)) );								} /* SLL  (HL)		  */
OP(cb,37) { _Z80_A = SLL(_Z80_A);											} /* SLL  A			  */

OP(cb,38) { _Z80_B = SRL(_Z80_B);											} /* SRL  B			  */
OP(cb,39) { _Z80_C = SRL(_Z80_C);											} /* SRL  C			  */
OP(cb,3a) { _Z80_D = SRL(_Z80_D);											} /* SRL  D			  */
OP(cb,3b) { _Z80_E = SRL(_Z80_E);											} /* SRL  E			  */
OP(cb,3c) { _Z80_H = SRL(_Z80_H);											} /* SRL  H			  */
OP(cb,3d) { _Z80_L = SRL(_Z80_L);											} /* SRL  L			  */
OP(cb,3e) { WM( _HL, SRL(RM(_HL)) );								} /* SRL  (HL)		  */
OP(cb,3f) { _Z80_A = SRL(_Z80_A);											} /* SRL  A			  */

OP(cb,40) { BIT(0,_Z80_B);												} /* BIT  0,B		  */
OP(cb,41) { BIT(0,_Z80_C);												} /* BIT  0,C		  */
OP(cb,42) { BIT(0,_Z80_D);												} /* BIT  0,D		  */
OP(cb,43) { BIT(0,_Z80_E);												} /* BIT  0,E		  */
OP(cb,44) { BIT(0,_Z80_H);												} /* BIT  0,H		  */
OP(cb,45) { BIT(0,_Z80_L);												} /* BIT  0,L		  */
OP(cb,46) { BIT(0,RM(_HL));											} /* BIT  0,(HL)	  */
OP(cb,47) { BIT(0,_Z80_A);												} /* BIT  0,A		  */

OP(cb,48) { BIT(1,_Z80_B);												} /* BIT  1,B		  */
OP(cb,49) { BIT(1,_Z80_C);												} /* BIT  1,C		  */
OP(cb,4a) { BIT(1,_Z80_D);												} /* BIT  1,D		  */
OP(cb,4b) { BIT(1,_Z80_E);												} /* BIT  1,E		  */
OP(cb,4c) { BIT(1,_Z80_H);												} /* BIT  1,H		  */
OP(cb,4d) { BIT(1,_Z80_L);												} /* BIT  1,L		  */
OP(cb,4e) { BIT(1,RM(_HL));											} /* BIT  1,(HL)	  */
OP(cb,4f) { BIT(1,_Z80_A);												} /* BIT  1,A		  */

OP(cb,50) { BIT(2,_Z80_B);												} /* BIT  2,B		  */
OP(cb,51) { BIT(2,_Z80_C);												} /* BIT  2,C		  */
OP(cb,52) { BIT(2,_Z80_D);												} /* BIT  2,D		  */
OP(cb,53) { BIT(2,_Z80_E);												} /* BIT  2,E		  */
OP(cb,54) { BIT(2,_Z80_H);												} /* BIT  2,H		  */
OP(cb,55) { BIT(2,_Z80_L);												} /* BIT  2,L		  */
OP(cb,56) { BIT(2,RM(_HL));											} /* BIT  2,(HL)	  */
OP(cb,57) { BIT(2,_Z80_A);												} /* BIT  2,A		  */

OP(cb,58) { BIT(3,_Z80_B);												} /* BIT  3,B		  */
OP(cb,59) { BIT(3,_Z80_C);												} /* BIT  3,C		  */
OP(cb,5a) { BIT(3,_Z80_D);												} /* BIT  3,D		  */
OP(cb,5b) { BIT(3,_Z80_E);												} /* BIT  3,E		  */
OP(cb,5c) { BIT(3,_Z80_H);												} /* BIT  3,H		  */
OP(cb,5d) { BIT(3,_Z80_L);												} /* BIT  3,L		  */
OP(cb,5e) { BIT(3,RM(_HL));											} /* BIT  3,(HL)	  */
OP(cb,5f) { BIT(3,_Z80_A);												} /* BIT  3,A		  */

OP(cb,60) { BIT(4,_Z80_B);												} /* BIT  4,B		  */
OP(cb,61) { BIT(4,_Z80_C);												} /* BIT  4,C		  */
OP(cb,62) { BIT(4,_Z80_D);												} /* BIT  4,D		  */
OP(cb,63) { BIT(4,_Z80_E);												} /* BIT  4,E		  */
OP(cb,64) { BIT(4,_Z80_H);												} /* BIT  4,H		  */
OP(cb,65) { BIT(4,_Z80_L);												} /* BIT  4,L		  */
OP(cb,66) { BIT(4,RM(_HL));											} /* BIT  4,(HL)	  */
OP(cb,67) { BIT(4,_Z80_A);												} /* BIT  4,A		  */

OP(cb,68) { BIT(5,_Z80_B);												} /* BIT  5,B		  */
OP(cb,69) { BIT(5,_Z80_C);												} /* BIT  5,C		  */
OP(cb,6a) { BIT(5,_Z80_D);												} /* BIT  5,D		  */
OP(cb,6b) { BIT(5,_Z80_E);												} /* BIT  5,E		  */
OP(cb,6c) { BIT(5,_Z80_H);												} /* BIT  5,H		  */
OP(cb,6d) { BIT(5,_Z80_L);												} /* BIT  5,L		  */
OP(cb,6e) { BIT(5,RM(_HL));											} /* BIT  5,(HL)	  */
OP(cb,6f) { BIT(5,_Z80_A);												} /* BIT  5,A		  */

OP(cb,70) { BIT(6,_Z80_B);												} /* BIT  6,B		  */
OP(cb,71) { BIT(6,_Z80_C);												} /* BIT  6,C		  */
OP(cb,72) { BIT(6,_Z80_D);												} /* BIT  6,D		  */
OP(cb,73) { BIT(6,_Z80_E);												} /* BIT  6,E		  */
OP(cb,74) { BIT(6,_Z80_H);												} /* BIT  6,H		  */
OP(cb,75) { BIT(6,_Z80_L);												} /* BIT  6,L		  */
OP(cb,76) { BIT(6,RM(_HL));											} /* BIT  6,(HL)	  */
OP(cb,77) { BIT(6,_Z80_A);												} /* BIT  6,A		  */

OP(cb,78) { BIT(7,_Z80_B);												} /* BIT  7,B		  */
OP(cb,79) { BIT(7,_Z80_C);												} /* BIT  7,C		  */
OP(cb,7a) { BIT(7,_Z80_D);												} /* BIT  7,D		  */
OP(cb,7b) { BIT(7,_Z80_E);												} /* BIT  7,E		  */
OP(cb,7c) { BIT(7,_Z80_H);												} /* BIT  7,H		  */
OP(cb,7d) { BIT(7,_Z80_L);												} /* BIT  7,L		  */
OP(cb,7e) { BIT(7,RM(_HL));											} /* BIT  7,(HL)	  */
OP(cb,7f) { BIT(7,_Z80_A);												} /* BIT  7,A		  */

OP(cb,80) { _Z80_B = RES(0,_Z80_B);											} /* RES  0,B		  */
OP(cb,81) { _Z80_C = RES(0,_Z80_C);											} /* RES  0,C		  */
OP(cb,82) { _Z80_D = RES(0,_Z80_D);											} /* RES  0,D		  */
OP(cb,83) { _Z80_E = RES(0,_Z80_E);											} /* RES  0,E		  */
OP(cb,84) { _Z80_H = RES(0,_Z80_H);											} /* RES  0,H		  */
OP(cb,85) { _Z80_L = RES(0,_Z80_L);											} /* RES  0,L		  */
OP(cb,86) { WM( _HL, RES(0,RM(_HL)) );								} /* RES  0,(HL)	  */
OP(cb,87) { _Z80_A = RES(0,_Z80_A);											} /* RES  0,A		  */

OP(cb,88) { _Z80_B = RES(1,_Z80_B);											} /* RES  1,B		  */
OP(cb,89) { _Z80_C = RES(1,_Z80_C);											} /* RES  1,C		  */
OP(cb,8a) { _Z80_D = RES(1,_Z80_D);											} /* RES  1,D		  */
OP(cb,8b) { _Z80_E = RES(1,_Z80_E);											} /* RES  1,E		  */
OP(cb,8c) { _Z80_H = RES(1,_Z80_H);											} /* RES  1,H		  */
OP(cb,8d) { _Z80_L = RES(1,_Z80_L);											} /* RES  1,L		  */
OP(cb,8e) { WM( _HL, RES(1,RM(_HL)) );								} /* RES  1,(HL)	  */
OP(cb,8f) { _Z80_A = RES(1,_Z80_A);											} /* RES  1,A		  */

OP(cb,90) { _Z80_B = RES(2,_Z80_B);											} /* RES  2,B		  */
OP(cb,91) { _Z80_C = RES(2,_Z80_C);											} /* RES  2,C		  */
OP(cb,92) { _Z80_D = RES(2,_Z80_D);											} /* RES  2,D		  */
OP(cb,93) { _Z80_E = RES(2,_Z80_E);											} /* RES  2,E		  */
OP(cb,94) { _Z80_H = RES(2,_Z80_H);											} /* RES  2,H		  */
OP(cb,95) { _Z80_L = RES(2,_Z80_L);											} /* RES  2,L		  */
OP(cb,96) { WM( _HL, RES(2,RM(_HL)) );								} /* RES  2,(HL)	  */
OP(cb,97) { _Z80_A = RES(2,_Z80_A);											} /* RES  2,A		  */

OP(cb,98) { _Z80_B = RES(3,_Z80_B);											} /* RES  3,B		  */
OP(cb,99) { _Z80_C = RES(3,_Z80_C);											} /* RES  3,C		  */
OP(cb,9a) { _Z80_D = RES(3,_Z80_D);											} /* RES  3,D		  */
OP(cb,9b) { _Z80_E = RES(3,_Z80_E);											} /* RES  3,E		  */
OP(cb,9c) { _Z80_H = RES(3,_Z80_H);											} /* RES  3,H		  */
OP(cb,9d) { _Z80_L = RES(3,_Z80_L);											} /* RES  3,L		  */
OP(cb,9e) { WM( _HL, RES(3,RM(_HL)) );								} /* RES  3,(HL)	  */
OP(cb,9f) { _Z80_A = RES(3,_Z80_A);											} /* RES  3,A		  */

OP(cb,a0) { _Z80_B = RES(4,_Z80_B);											} /* RES  4,B		  */
OP(cb,a1) { _Z80_C = RES(4,_Z80_C);											} /* RES  4,C		  */
OP(cb,a2) { _Z80_D = RES(4,_Z80_D);											} /* RES  4,D		  */
OP(cb,a3) { _Z80_E = RES(4,_Z80_E);											} /* RES  4,E		  */
OP(cb,a4) { _Z80_H = RES(4,_Z80_H);											} /* RES  4,H		  */
OP(cb,a5) { _Z80_L = RES(4,_Z80_L);											} /* RES  4,L		  */
OP(cb,a6) { WM( _HL, RES(4,RM(_HL)) );								} /* RES  4,(HL)	  */
OP(cb,a7) { _Z80_A = RES(4,_Z80_A);											} /* RES  4,A		  */

OP(cb,a8) { _Z80_B = RES(5,_Z80_B);											} /* RES  5,B		  */
OP(cb,a9) { _Z80_C = RES(5,_Z80_C);											} /* RES  5,C		  */
OP(cb,aa) { _Z80_D = RES(5,_Z80_D);											} /* RES  5,D		  */
OP(cb,ab) { _Z80_E = RES(5,_Z80_E);											} /* RES  5,E		  */
OP(cb,ac) { _Z80_H = RES(5,_Z80_H);											} /* RES  5,H		  */
OP(cb,ad) { _Z80_L = RES(5,_Z80_L);											} /* RES  5,L		  */
OP(cb,ae) { WM( _HL, RES(5,RM(_HL)) );								} /* RES  5,(HL)	  */
OP(cb,af) { _Z80_A = RES(5,_Z80_A);											} /* RES  5,A		  */

OP(cb,b0) { _Z80_B = RES(6,_Z80_B);											} /* RES  6,B		  */
OP(cb,b1) { _Z80_C = RES(6,_Z80_C);											} /* RES  6,C		  */
OP(cb,b2) { _Z80_D = RES(6,_Z80_D);											} /* RES  6,D		  */
OP(cb,b3) { _Z80_E = RES(6,_Z80_E);											} /* RES  6,E		  */
OP(cb,b4) { _Z80_H = RES(6,_Z80_H);											} /* RES  6,H		  */
OP(cb,b5) { _Z80_L = RES(6,_Z80_L);											} /* RES  6,L		  */
OP(cb,b6) { WM( _HL, RES(6,RM(_HL)) );								} /* RES  6,(HL)	  */
OP(cb,b7) { _Z80_A = RES(6,_Z80_A);											} /* RES  6,A		  */

OP(cb,b8) { _Z80_B = RES(7,_Z80_B);											} /* RES  7,B		  */
OP(cb,b9) { _Z80_C = RES(7,_Z80_C);											} /* RES  7,C		  */
OP(cb,ba) { _Z80_D = RES(7,_Z80_D);											} /* RES  7,D		  */
OP(cb,bb) { _Z80_E = RES(7,_Z80_E);											} /* RES  7,E		  */
OP(cb,bc) { _Z80_H = RES(7,_Z80_H);											} /* RES  7,H		  */
OP(cb,bd) { _Z80_L = RES(7,_Z80_L);											} /* RES  7,L		  */
OP(cb,be) { WM( _HL, RES(7,RM(_HL)) );								} /* RES  7,(HL)	  */
OP(cb,bf) { _Z80_A = RES(7,_Z80_A);											} /* RES  7,A		  */

OP(cb,c0) { _Z80_B = SET(0,_Z80_B);											} /* SET  0,B		  */
OP(cb,c1) { _Z80_C = SET(0,_Z80_C);											} /* SET  0,C		  */
OP(cb,c2) { _Z80_D = SET(0,_Z80_D);											} /* SET  0,D		  */
OP(cb,c3) { _Z80_E = SET(0,_Z80_E);											} /* SET  0,E		  */
OP(cb,c4) { _Z80_H = SET(0,_Z80_H);											} /* SET  0,H		  */
OP(cb,c5) { _Z80_L = SET(0,_Z80_L);											} /* SET  0,L		  */
OP(cb,c6) { WM( _HL, SET(0,RM(_HL)) );								} /* SET  0,(HL)	  */
OP(cb,c7) { _Z80_A = SET(0,_Z80_A);											} /* SET  0,A		  */

OP(cb,c8) { _Z80_B = SET(1,_Z80_B);											} /* SET  1,B		  */
OP(cb,c9) { _Z80_C = SET(1,_Z80_C);											} /* SET  1,C		  */
OP(cb,ca) { _Z80_D = SET(1,_Z80_D);											} /* SET  1,D		  */
OP(cb,cb) { _Z80_E = SET(1,_Z80_E);											} /* SET  1,E		  */
OP(cb,cc) { _Z80_H = SET(1,_Z80_H);											} /* SET  1,H		  */
OP(cb,cd) { _Z80_L = SET(1,_Z80_L);											} /* SET  1,L		  */
OP(cb,ce) { WM( _HL, SET(1,RM(_HL)) );								} /* SET  1,(HL)	  */
OP(cb,cf) { _Z80_A = SET(1,_Z80_A);											} /* SET  1,A		  */

OP(cb,d0) { _Z80_B = SET(2,_Z80_B);											} /* SET  2,B		  */
OP(cb,d1) { _Z80_C = SET(2,_Z80_C);											} /* SET  2,C		  */
OP(cb,d2) { _Z80_D = SET(2,_Z80_D);											} /* SET  2,D		  */
OP(cb,d3) { _Z80_E = SET(2,_Z80_E);											} /* SET  2,E		  */
OP(cb,d4) { _Z80_H = SET(2,_Z80_H);											} /* SET  2,H		  */
OP(cb,d5) { _Z80_L = SET(2,_Z80_L);											} /* SET  2,L		  */
OP(cb,d6) { WM( _HL, SET(2,RM(_HL)) );								}/* SET  2,(HL)		 */
OP(cb,d7) { _Z80_A = SET(2,_Z80_A);											} /* SET  2,A		  */

OP(cb,d8) { _Z80_B = SET(3,_Z80_B);											} /* SET  3,B		  */
OP(cb,d9) { _Z80_C = SET(3,_Z80_C);											} /* SET  3,C		  */
OP(cb,da) { _Z80_D = SET(3,_Z80_D);											} /* SET  3,D		  */
OP(cb,db) { _Z80_E = SET(3,_Z80_E);											} /* SET  3,E		  */
OP(cb,dc) { _Z80_H = SET(3,_Z80_H);											} /* SET  3,H		  */
OP(cb,dd) { _Z80_L = SET(3,_Z80_L);											} /* SET  3,L		  */
OP(cb,de) { WM( _HL, SET(3,RM(_HL)) );								} /* SET  3,(HL)	  */
OP(cb,df) { _Z80_A = SET(3,_Z80_A);											} /* SET  3,A		  */

OP(cb,e0) { _Z80_B = SET(4,_Z80_B);											} /* SET  4,B		  */
OP(cb,e1) { _Z80_C = SET(4,_Z80_C);											} /* SET  4,C		  */
OP(cb,e2) { _Z80_D = SET(4,_Z80_D);											} /* SET  4,D		  */
OP(cb,e3) { _Z80_E = SET(4,_Z80_E);											} /* SET  4,E		  */
OP(cb,e4) { _Z80_H = SET(4,_Z80_H);											} /* SET  4,H		  */
OP(cb,e5) { _Z80_L = SET(4,_Z80_L);											} /* SET  4,L		  */
OP(cb,e6) { WM( _HL, SET(4,RM(_HL)) );								} /* SET  4,(HL)	  */
OP(cb,e7) { _Z80_A = SET(4,_Z80_A);											} /* SET  4,A		  */

OP(cb,e8) { _Z80_B = SET(5,_Z80_B);											} /* SET  5,B		  */
OP(cb,e9) { _Z80_C = SET(5,_Z80_C);											} /* SET  5,C		  */
OP(cb,ea) { _Z80_D = SET(5,_Z80_D);											} /* SET  5,D		  */
OP(cb,eb) { _Z80_E = SET(5,_Z80_E);											} /* SET  5,E		  */
OP(cb,ec) { _Z80_H = SET(5,_Z80_H);											} /* SET  5,H		  */
OP(cb,ed) { _Z80_L = SET(5,_Z80_L);											} /* SET  5,L		  */
OP(cb,ee) { WM( _HL, SET(5,RM(_HL)) );								} /* SET  5,(HL)	  */
OP(cb,ef) { _Z80_A = SET(5,_Z80_A);											} /* SET  5,A		  */

OP(cb,f0) { _Z80_B = SET(6,_Z80_B);											} /* SET  6,B		  */
OP(cb,f1) { _Z80_C = SET(6,_Z80_C);											} /* SET  6,C		  */
OP(cb,f2) { _Z80_D = SET(6,_Z80_D);											} /* SET  6,D		  */
OP(cb,f3) { _Z80_E = SET(6,_Z80_E);											} /* SET  6,E		  */
OP(cb,f4) { _Z80_H = SET(6,_Z80_H);											} /* SET  6,H		  */
OP(cb,f5) { _Z80_L = SET(6,_Z80_L);											} /* SET  6,L		  */
OP(cb,f6) { WM( _HL, SET(6,RM(_HL)) );								} /* SET  6,(HL)	  */
OP(cb,f7) { _Z80_A = SET(6,_Z80_A);											} /* SET  6,A		  */

OP(cb,f8) { _Z80_B = SET(7,_Z80_B);											} /* SET  7,B		  */
OP(cb,f9) { _Z80_C = SET(7,_Z80_C);											} /* SET  7,C		  */
OP(cb,fa) { _Z80_D = SET(7,_Z80_D);											} /* SET  7,D		  */
OP(cb,fb) { _Z80_E = SET(7,_Z80_E);											} /* SET  7,E		  */
OP(cb,fc) { _Z80_H = SET(7,_Z80_H);											} /* SET  7,H		  */
OP(cb,fd) { _Z80_L = SET(7,_Z80_L);											} /* SET  7,L		  */
OP(cb,fe) { WM( _HL, SET(7,RM(_HL)) );								} /* SET  7,(HL)	  */
OP(cb,ff) { _Z80_A = SET(7,_Z80_A);											} /* SET  7,A		  */


/**********************************************************
* opcodes with DD/FD CB prefix
* rotate, shift and bit operations with (IX+o)
**********************************************************/
OP(xycb,00) { _Z80_B = RLC( RM(EA) ); WM( EA,_Z80_B );						} /* RLC  B=(XY+o)	  */
OP(xycb,01) { _Z80_C = RLC( RM(EA) ); WM( EA,_Z80_C );						} /* RLC  C=(XY+o)	  */
OP(xycb,02) { _Z80_D = RLC( RM(EA) ); WM( EA,_Z80_D );						} /* RLC  D=(XY+o)	  */
OP(xycb,03) { _Z80_E = RLC( RM(EA) ); WM( EA,_Z80_E );						} /* RLC  E=(XY+o)	  */
OP(xycb,04) { _Z80_H = RLC( RM(EA) ); WM( EA,_Z80_H );						} /* RLC  H=(XY+o)	  */
OP(xycb,05) { _Z80_L = RLC( RM(EA) ); WM( EA,_Z80_L );						} /* RLC  L=(XY+o)	  */
OP(xycb,06) { WM( EA, RLC( RM(EA) ) );								} /* RLC  (XY+o)	  */
OP(xycb,07) { _Z80_A = RLC( RM(EA) ); WM( EA,_Z80_A );						} /* RLC  A=(XY+o)	  */

OP(xycb,08) { _Z80_B = RRC( RM(EA) ); WM( EA,_Z80_B );						} /* RRC  B=(XY+o)	  */
OP(xycb,09) { _Z80_C = RRC( RM(EA) ); WM( EA,_Z80_C );						} /* RRC  C=(XY+o)	  */
OP(xycb,0a) { _Z80_D = RRC( RM(EA) ); WM( EA,_Z80_D );						} /* RRC  D=(XY+o)	  */
OP(xycb,0b) { _Z80_E = RRC( RM(EA) ); WM( EA,_Z80_E );						} /* RRC  E=(XY+o)	  */
OP(xycb,0c) { _Z80_H = RRC( RM(EA) ); WM( EA,_Z80_H );						} /* RRC  H=(XY+o)	  */
OP(xycb,0d) { _Z80_L = RRC( RM(EA) ); WM( EA,_Z80_L );						} /* RRC  L=(XY+o)	  */
OP(xycb,0e) { WM( EA,RRC( RM(EA) ) );								} /* RRC  (XY+o)	  */
OP(xycb,0f) { _Z80_A = RRC( RM(EA) ); WM( EA,_Z80_A );						} /* RRC  A=(XY+o)	  */

OP(xycb,10) { _Z80_B = RL( RM(EA) ); WM( EA,_Z80_B );						} /* RL   B=(XY+o)	  */
OP(xycb,11) { _Z80_C = RL( RM(EA) ); WM( EA,_Z80_C );						} /* RL   C=(XY+o)	  */
OP(xycb,12) { _Z80_D = RL( RM(EA) ); WM( EA,_Z80_D );						} /* RL   D=(XY+o)	  */
OP(xycb,13) { _Z80_E = RL( RM(EA) ); WM( EA,_Z80_E );						} /* RL   E=(XY+o)	  */
OP(xycb,14) { _Z80_H = RL( RM(EA) ); WM( EA,_Z80_H );						} /* RL   H=(XY+o)	  */
OP(xycb,15) { _Z80_L = RL( RM(EA) ); WM( EA,_Z80_L );						} /* RL   L=(XY+o)	  */
OP(xycb,16) { WM( EA,RL( RM(EA) ) );								} /* RL   (XY+o)	  */
OP(xycb,17) { _Z80_A = RL( RM(EA) ); WM( EA,_Z80_A );						} /* RL   A=(XY+o)	  */

OP(xycb,18) { _Z80_B = RR( RM(EA) ); WM( EA,_Z80_B );						} /* RR   B=(XY+o)	  */
OP(xycb,19) { _Z80_C = RR( RM(EA) ); WM( EA,_Z80_C );						} /* RR   C=(XY+o)	  */
OP(xycb,1a) { _Z80_D = RR( RM(EA) ); WM( EA,_Z80_D );						} /* RR   D=(XY+o)	  */
OP(xycb,1b) { _Z80_E = RR( RM(EA) ); WM( EA,_Z80_E );						} /* RR   E=(XY+o)	  */
OP(xycb,1c) { _Z80_H = RR( RM(EA) ); WM( EA,_Z80_H );						} /* RR   H=(XY+o)	  */
OP(xycb,1d) { _Z80_L = RR( RM(EA) ); WM( EA,_Z80_L );						} /* RR   L=(XY+o)	  */
OP(xycb,1e) { WM( EA,RR( RM(EA) ) );								} /* RR   (XY+o)	  */
OP(xycb,1f) { _Z80_A = RR( RM(EA) ); WM( EA,_Z80_A );						} /* RR   A=(XY+o)	  */

OP(xycb,20) { _Z80_B = SLA( RM(EA) ); WM( EA,_Z80_B );						} /* SLA  B=(XY+o)	  */
OP(xycb,21) { _Z80_C = SLA( RM(EA) ); WM( EA,_Z80_C );						} /* SLA  C=(XY+o)	  */
OP(xycb,22) { _Z80_D = SLA( RM(EA) ); WM( EA,_Z80_D );						} /* SLA  D=(XY+o)	  */
OP(xycb,23) { _Z80_E = SLA( RM(EA) ); WM( EA,_Z80_E );						} /* SLA  E=(XY+o)	  */
OP(xycb,24) { _Z80_H = SLA( RM(EA) ); WM( EA,_Z80_H );						} /* SLA  H=(XY+o)	  */
OP(xycb,25) { _Z80_L = SLA( RM(EA) ); WM( EA,_Z80_L );						} /* SLA  L=(XY+o)	  */
OP(xycb,26) { WM( EA,SLA( RM(EA) ) );								} /* SLA  (XY+o)	  */
OP(xycb,27) { _Z80_A = SLA( RM(EA) ); WM( EA,_Z80_A );						} /* SLA  A=(XY+o)	  */

OP(xycb,28) { _Z80_B = SRA( RM(EA) ); WM( EA,_Z80_B );						} /* SRA  B=(XY+o)	  */
OP(xycb,29) { _Z80_C = SRA( RM(EA) ); WM( EA,_Z80_C );						} /* SRA  C=(XY+o)	  */
OP(xycb,2a) { _Z80_D = SRA( RM(EA) ); WM( EA,_Z80_D );						} /* SRA  D=(XY+o)	  */
OP(xycb,2b) { _Z80_E = SRA( RM(EA) ); WM( EA,_Z80_E );						} /* SRA  E=(XY+o)	  */
OP(xycb,2c) { _Z80_H = SRA( RM(EA) ); WM( EA,_Z80_H );						} /* SRA  H=(XY+o)	  */
OP(xycb,2d) { _Z80_L = SRA( RM(EA) ); WM( EA,_Z80_L );						} /* SRA  L=(XY+o)	  */
OP(xycb,2e) { WM( EA,SRA( RM(EA) ) );								} /* SRA  (XY+o)	  */
OP(xycb,2f) { _Z80_A = SRA( RM(EA) ); WM( EA,_Z80_A );						} /* SRA  A=(XY+o)	  */

OP(xycb,30) { _Z80_B = SLL( RM(EA) ); WM( EA,_Z80_B );						} /* SLL  B=(XY+o)	  */
OP(xycb,31) { _Z80_C = SLL( RM(EA) ); WM( EA,_Z80_C );						} /* SLL  C=(XY+o)	  */
OP(xycb,32) { _Z80_D = SLL( RM(EA) ); WM( EA,_Z80_D );						} /* SLL  D=(XY+o)	  */
OP(xycb,33) { _Z80_E = SLL( RM(EA) ); WM( EA,_Z80_E );						} /* SLL  E=(XY+o)	  */
OP(xycb,34) { _Z80_H = SLL( RM(EA) ); WM( EA,_Z80_H );						} /* SLL  H=(XY+o)	  */
OP(xycb,35) { _Z80_L = SLL( RM(EA) ); WM( EA,_Z80_L );						} /* SLL  L=(XY+o)	  */
OP(xycb,36) { WM( EA,SLL( RM(EA) ) );								} /* SLL  (XY+o)	  */
OP(xycb,37) { _Z80_A = SLL( RM(EA) ); WM( EA,_Z80_A );						} /* SLL  A=(XY+o)	  */

OP(xycb,38) { _Z80_B = SRL( RM(EA) ); WM( EA,_Z80_B );						} /* SRL  B=(XY+o)	  */
OP(xycb,39) { _Z80_C = SRL( RM(EA) ); WM( EA,_Z80_C );						} /* SRL  C=(XY+o)	  */
OP(xycb,3a) { _Z80_D = SRL( RM(EA) ); WM( EA,_Z80_D );						} /* SRL  D=(XY+o)	  */
OP(xycb,3b) { _Z80_E = SRL( RM(EA) ); WM( EA,_Z80_E );						} /* SRL  E=(XY+o)	  */
OP(xycb,3c) { _Z80_H = SRL( RM(EA) ); WM( EA,_Z80_H );						} /* SRL  H=(XY+o)	  */
OP(xycb,3d) { _Z80_L = SRL( RM(EA) ); WM( EA,_Z80_L );						} /* SRL  L=(XY+o)	  */
OP(xycb,3e) { WM( EA,SRL( RM(EA) ) );								} /* SRL  (XY+o)	  */
OP(xycb,3f) { _Z80_A = SRL( RM(EA) ); WM( EA,_Z80_A );						} /* SRL  A=(XY+o)	  */

OP(xycb,40) { xycb_46();											} /* BIT  0,B=(XY+o)  */
OP(xycb,41) { xycb_46();													  } /* BIT	0,C=(XY+o)	*/
OP(xycb,42) { xycb_46();											} /* BIT  0,D=(XY+o)  */
OP(xycb,43) { xycb_46();											} /* BIT  0,E=(XY+o)  */
OP(xycb,44) { xycb_46();											} /* BIT  0,H=(XY+o)  */
OP(xycb,45) { xycb_46();											} /* BIT  0,L=(XY+o)  */
OP(xycb,46) { BIT_XY(0,RM(EA));										} /* BIT  0,(XY+o)	  */
OP(xycb,47) { xycb_46();											} /* BIT  0,A=(XY+o)  */

OP(xycb,48) { xycb_4e();											} /* BIT  1,B=(XY+o)  */
OP(xycb,49) { xycb_4e();													  } /* BIT	1,C=(XY+o)	*/
OP(xycb,4a) { xycb_4e();											} /* BIT  1,D=(XY+o)  */
OP(xycb,4b) { xycb_4e();											} /* BIT  1,E=(XY+o)  */
OP(xycb,4c) { xycb_4e();											} /* BIT  1,H=(XY+o)  */
OP(xycb,4d) { xycb_4e();											} /* BIT  1,L=(XY+o)  */
OP(xycb,4e) { BIT_XY(1,RM(EA));										} /* BIT  1,(XY+o)	  */
OP(xycb,4f) { xycb_4e();											} /* BIT  1,A=(XY+o)  */

OP(xycb,50) { xycb_56();											} /* BIT  2,B=(XY+o)  */
OP(xycb,51) { xycb_56();													  } /* BIT	2,C=(XY+o)	*/
OP(xycb,52) { xycb_56();											} /* BIT  2,D=(XY+o)  */
OP(xycb,53) { xycb_56();											} /* BIT  2,E=(XY+o)  */
OP(xycb,54) { xycb_56();											} /* BIT  2,H=(XY+o)  */
OP(xycb,55) { xycb_56();											} /* BIT  2,L=(XY+o)  */
OP(xycb,56) { BIT_XY(2,RM(EA));										} /* BIT  2,(XY+o)	  */
OP(xycb,57) { xycb_56();											} /* BIT  2,A=(XY+o)  */

OP(xycb,58) { xycb_5e();											} /* BIT  3,B=(XY+o)  */
OP(xycb,59) { xycb_5e();													  } /* BIT	3,C=(XY+o)	*/
OP(xycb,5a) { xycb_5e();											} /* BIT  3,D=(XY+o)  */
OP(xycb,5b) { xycb_5e();											} /* BIT  3,E=(XY+o)  */
OP(xycb,5c) { xycb_5e();											} /* BIT  3,H=(XY+o)  */
OP(xycb,5d) { xycb_5e();											} /* BIT  3,L=(XY+o)  */
OP(xycb,5e) { BIT_XY(3,RM(EA));										} /* BIT  3,(XY+o)	  */
OP(xycb,5f) { xycb_5e();											} /* BIT  3,A=(XY+o)  */

OP(xycb,60) { xycb_66();											} /* BIT  4,B=(XY+o)  */
OP(xycb,61) { xycb_66();													  } /* BIT	4,C=(XY+o)	*/
OP(xycb,62) { xycb_66();											} /* BIT  4,D=(XY+o)  */
OP(xycb,63) { xycb_66();											} /* BIT  4,E=(XY+o)  */
OP(xycb,64) { xycb_66();											} /* BIT  4,H=(XY+o)  */
OP(xycb,65) { xycb_66();											} /* BIT  4,L=(XY+o)  */
OP(xycb,66) { BIT_XY(4,RM(EA));										} /* BIT  4,(XY+o)	  */
OP(xycb,67) { xycb_66();											} /* BIT  4,A=(XY+o)  */

OP(xycb,68) { xycb_6e();											} /* BIT  5,B=(XY+o)  */
OP(xycb,69) { xycb_6e();													  } /* BIT	5,C=(XY+o)	*/
OP(xycb,6a) { xycb_6e();											} /* BIT  5,D=(XY+o)  */
OP(xycb,6b) { xycb_6e();											} /* BIT  5,E=(XY+o)  */
OP(xycb,6c) { xycb_6e();											} /* BIT  5,H=(XY+o)  */
OP(xycb,6d) { xycb_6e();											} /* BIT  5,L=(XY+o)  */
OP(xycb,6e) { BIT_XY(5,RM(EA));										} /* BIT  5,(XY+o)	  */
OP(xycb,6f) { xycb_6e();											} /* BIT  5,A=(XY+o)  */

OP(xycb,70) { xycb_76();											} /* BIT  6,B=(XY+o)  */
OP(xycb,71) { xycb_76();													  } /* BIT	6,C=(XY+o)	*/
OP(xycb,72) { xycb_76();											} /* BIT  6,D=(XY+o)  */
OP(xycb,73) { xycb_76();											} /* BIT  6,E=(XY+o)  */
OP(xycb,74) { xycb_76();											} /* BIT  6,H=(XY+o)  */
OP(xycb,75) { xycb_76();											} /* BIT  6,L=(XY+o)  */
OP(xycb,76) { BIT_XY(6,RM(EA));										} /* BIT  6,(XY+o)	  */
OP(xycb,77) { xycb_76();											} /* BIT  6,A=(XY+o)  */

OP(xycb,78) { xycb_7e();											} /* BIT  7,B=(XY+o)  */
OP(xycb,79) { xycb_7e();													  } /* BIT	7,C=(XY+o)	*/
OP(xycb,7a) { xycb_7e();											} /* BIT  7,D=(XY+o)  */
OP(xycb,7b) { xycb_7e();											} /* BIT  7,E=(XY+o)  */
OP(xycb,7c) { xycb_7e();											} /* BIT  7,H=(XY+o)  */
OP(xycb,7d) { xycb_7e();											} /* BIT  7,L=(XY+o)  */
OP(xycb,7e) { BIT_XY(7,RM(EA));										} /* BIT  7,(XY+o)	  */
OP(xycb,7f) { xycb_7e();											} /* BIT  7,A=(XY+o)  */

OP(xycb,80) { _Z80_B = RES(0, RM(EA) ); WM( EA,_Z80_B );					} /* RES  0,B=(XY+o)  */
OP(xycb,81) { _Z80_C = RES(0, RM(EA) ); WM( EA,_Z80_C );					} /* RES  0,C=(XY+o)  */
OP(xycb,82) { _Z80_D = RES(0, RM(EA) ); WM( EA,_Z80_D );					} /* RES  0,D=(XY+o)  */
OP(xycb,83) { _Z80_E = RES(0, RM(EA) ); WM( EA,_Z80_E );					} /* RES  0,E=(XY+o)  */
OP(xycb,84) { _Z80_H = RES(0, RM(EA) ); WM( EA,_Z80_H );					} /* RES  0,H=(XY+o)  */
OP(xycb,85) { _Z80_L = RES(0, RM(EA) ); WM( EA,_Z80_L );					} /* RES  0,L=(XY+o)  */
OP(xycb,86) { WM( EA, RES(0,RM(EA)) );								} /* RES  0,(XY+o)	  */
OP(xycb,87) { _Z80_A = RES(0, RM(EA) ); WM( EA,_Z80_A );					} /* RES  0,A=(XY+o)  */

OP(xycb,88) { _Z80_B = RES(1, RM(EA) ); WM( EA,_Z80_B );					} /* RES  1,B=(XY+o)  */
OP(xycb,89) { _Z80_C = RES(1, RM(EA) ); WM( EA,_Z80_C );					} /* RES  1,C=(XY+o)  */
OP(xycb,8a) { _Z80_D = RES(1, RM(EA) ); WM( EA,_Z80_D );					} /* RES  1,D=(XY+o)  */
OP(xycb,8b) { _Z80_E = RES(1, RM(EA) ); WM( EA,_Z80_E );					} /* RES  1,E=(XY+o)  */
OP(xycb,8c) { _Z80_H = RES(1, RM(EA) ); WM( EA,_Z80_H );					} /* RES  1,H=(XY+o)  */
OP(xycb,8d) { _Z80_L = RES(1, RM(EA) ); WM( EA,_Z80_L );					} /* RES  1,L=(XY+o)  */
OP(xycb,8e) { WM( EA, RES(1,RM(EA)) );								} /* RES  1,(XY+o)	  */
OP(xycb,8f) { _Z80_A = RES(1, RM(EA) ); WM( EA,_Z80_A );					} /* RES  1,A=(XY+o)  */

OP(xycb,90) { _Z80_B = RES(2, RM(EA) ); WM( EA,_Z80_B );					} /* RES  2,B=(XY+o)  */
OP(xycb,91) { _Z80_C = RES(2, RM(EA) ); WM( EA,_Z80_C );					} /* RES  2,C=(XY+o)  */
OP(xycb,92) { _Z80_D = RES(2, RM(EA) ); WM( EA,_Z80_D );					} /* RES  2,D=(XY+o)  */
OP(xycb,93) { _Z80_E = RES(2, RM(EA) ); WM( EA,_Z80_E );					} /* RES  2,E=(XY+o)  */
OP(xycb,94) { _Z80_H = RES(2, RM(EA) ); WM( EA,_Z80_H );					} /* RES  2,H=(XY+o)  */
OP(xycb,95) { _Z80_L = RES(2, RM(EA) ); WM( EA,_Z80_L );					} /* RES  2,L=(XY+o)  */
OP(xycb,96) { WM( EA, RES(2,RM(EA)) );								} /* RES  2,(XY+o)	  */
OP(xycb,97) { _Z80_A = RES(2, RM(EA) ); WM( EA,_Z80_A );					} /* RES  2,A=(XY+o)  */

OP(xycb,98) { _Z80_B = RES(3, RM(EA) ); WM( EA,_Z80_B );					} /* RES  3,B=(XY+o)  */
OP(xycb,99) { _Z80_C = RES(3, RM(EA) ); WM( EA,_Z80_C );					} /* RES  3,C=(XY+o)  */
OP(xycb,9a) { _Z80_D = RES(3, RM(EA) ); WM( EA,_Z80_D );					} /* RES  3,D=(XY+o)  */
OP(xycb,9b) { _Z80_E = RES(3, RM(EA) ); WM( EA,_Z80_E );					} /* RES  3,E=(XY+o)  */
OP(xycb,9c) { _Z80_H = RES(3, RM(EA) ); WM( EA,_Z80_H );					} /* RES  3,H=(XY+o)  */
OP(xycb,9d) { _Z80_L = RES(3, RM(EA) ); WM( EA,_Z80_L );					} /* RES  3,L=(XY+o)  */
OP(xycb,9e) { WM( EA, RES(3,RM(EA)) );								} /* RES  3,(XY+o)	  */
OP(xycb,9f) { _Z80_A = RES(3, RM(EA) ); WM( EA,_Z80_A );					} /* RES  3,A=(XY+o)  */

OP(xycb,a0) { _Z80_B = RES(4, RM(EA) ); WM( EA,_Z80_B );					} /* RES  4,B=(XY+o)  */
OP(xycb,a1) { _Z80_C = RES(4, RM(EA) ); WM( EA,_Z80_C );					} /* RES  4,C=(XY+o)  */
OP(xycb,a2) { _Z80_D = RES(4, RM(EA) ); WM( EA,_Z80_D );					} /* RES  4,D=(XY+o)  */
OP(xycb,a3) { _Z80_E = RES(4, RM(EA) ); WM( EA,_Z80_E );					} /* RES  4,E=(XY+o)  */
OP(xycb,a4) { _Z80_H = RES(4, RM(EA) ); WM( EA,_Z80_H );					} /* RES  4,H=(XY+o)  */
OP(xycb,a5) { _Z80_L = RES(4, RM(EA) ); WM( EA,_Z80_L );					} /* RES  4,L=(XY+o)  */
OP(xycb,a6) { WM( EA, RES(4,RM(EA)) );								} /* RES  4,(XY+o)	  */
OP(xycb,a7) { _Z80_A = RES(4, RM(EA) ); WM( EA,_Z80_A );					} /* RES  4,A=(XY+o)  */

OP(xycb,a8) { _Z80_B = RES(5, RM(EA) ); WM( EA,_Z80_B );					} /* RES  5,B=(XY+o)  */
OP(xycb,a9) { _Z80_C = RES(5, RM(EA) ); WM( EA,_Z80_C );					} /* RES  5,C=(XY+o)  */
OP(xycb,aa) { _Z80_D = RES(5, RM(EA) ); WM( EA,_Z80_D );					} /* RES  5,D=(XY+o)  */
OP(xycb,ab) { _Z80_E = RES(5, RM(EA) ); WM( EA,_Z80_E );					} /* RES  5,E=(XY+o)  */
OP(xycb,ac) { _Z80_H = RES(5, RM(EA) ); WM( EA,_Z80_H );					} /* RES  5,H=(XY+o)  */
OP(xycb,ad) { _Z80_L = RES(5, RM(EA) ); WM( EA,_Z80_L );					} /* RES  5,L=(XY+o)  */
OP(xycb,ae) { WM( EA, RES(5,RM(EA)) );								} /* RES  5,(XY+o)	  */
OP(xycb,af) { _Z80_A = RES(5, RM(EA) ); WM( EA,_Z80_A );					} /* RES  5,A=(XY+o)  */

OP(xycb,b0) { _Z80_B = RES(6, RM(EA) ); WM( EA,_Z80_B );					} /* RES  6,B=(XY+o)  */
OP(xycb,b1) { _Z80_C = RES(6, RM(EA) ); WM( EA,_Z80_C );					} /* RES  6,C=(XY+o)  */
OP(xycb,b2) { _Z80_D = RES(6, RM(EA) ); WM( EA,_Z80_D );					} /* RES  6,D=(XY+o)  */
OP(xycb,b3) { _Z80_E = RES(6, RM(EA) ); WM( EA,_Z80_E );					} /* RES  6,E=(XY+o)  */
OP(xycb,b4) { _Z80_H = RES(6, RM(EA) ); WM( EA,_Z80_H );					} /* RES  6,H=(XY+o)  */
OP(xycb,b5) { _Z80_L = RES(6, RM(EA) ); WM( EA,_Z80_L );					} /* RES  6,L=(XY+o)  */
OP(xycb,b6) { WM( EA, RES(6,RM(EA)) );								} /* RES  6,(XY+o)	  */
OP(xycb,b7) { _Z80_A = RES(6, RM(EA) ); WM( EA,_Z80_A );					} /* RES  6,A=(XY+o)  */

OP(xycb,b8) { _Z80_B = RES(7, RM(EA) ); WM( EA,_Z80_B );					} /* RES  7,B=(XY+o)  */
OP(xycb,b9) { _Z80_C = RES(7, RM(EA) ); WM( EA,_Z80_C );					} /* RES  7,C=(XY+o)  */
OP(xycb,ba) { _Z80_D = RES(7, RM(EA) ); WM( EA,_Z80_D );					} /* RES  7,D=(XY+o)  */
OP(xycb,bb) { _Z80_E = RES(7, RM(EA) ); WM( EA,_Z80_E );					} /* RES  7,E=(XY+o)  */
OP(xycb,bc) { _Z80_H = RES(7, RM(EA) ); WM( EA,_Z80_H );					} /* RES  7,H=(XY+o)  */
OP(xycb,bd) { _Z80_L = RES(7, RM(EA) ); WM( EA,_Z80_L );					} /* RES  7,L=(XY+o)  */
OP(xycb,be) { WM( EA, RES(7,RM(EA)) );								} /* RES  7,(XY+o)	  */
OP(xycb,bf) { _Z80_A = RES(7, RM(EA) ); WM( EA,_Z80_A );					} /* RES  7,A=(XY+o)  */

OP(xycb,c0) { _Z80_B = SET(0, RM(EA) ); WM( EA,_Z80_B );					} /* SET  0,B=(XY+o)  */
OP(xycb,c1) { _Z80_C = SET(0, RM(EA) ); WM( EA,_Z80_C );					} /* SET  0,C=(XY+o)  */
OP(xycb,c2) { _Z80_D = SET(0, RM(EA) ); WM( EA,_Z80_D );					} /* SET  0,D=(XY+o)  */
OP(xycb,c3) { _Z80_E = SET(0, RM(EA) ); WM( EA,_Z80_E );					} /* SET  0,E=(XY+o)  */
OP(xycb,c4) { _Z80_H = SET(0, RM(EA) ); WM( EA,_Z80_H );					} /* SET  0,H=(XY+o)  */
OP(xycb,c5) { _Z80_L = SET(0, RM(EA) ); WM( EA,_Z80_L );					} /* SET  0,L=(XY+o)  */
OP(xycb,c6) { WM( EA, SET(0,RM(EA)) );								} /* SET  0,(XY+o)	  */
OP(xycb,c7) { _Z80_A = SET(0, RM(EA) ); WM( EA,_Z80_A );					} /* SET  0,A=(XY+o)  */

OP(xycb,c8) { _Z80_B = SET(1, RM(EA) ); WM( EA,_Z80_B );					} /* SET  1,B=(XY+o)  */
OP(xycb,c9) { _Z80_C = SET(1, RM(EA) ); WM( EA,_Z80_C );					} /* SET  1,C=(XY+o)  */
OP(xycb,ca) { _Z80_D = SET(1, RM(EA) ); WM( EA,_Z80_D );					} /* SET  1,D=(XY+o)  */
OP(xycb,cb) { _Z80_E = SET(1, RM(EA) ); WM( EA,_Z80_E );					} /* SET  1,E=(XY+o)  */
OP(xycb,cc) { _Z80_H = SET(1, RM(EA) ); WM( EA,_Z80_H );					} /* SET  1,H=(XY+o)  */
OP(xycb,cd) { _Z80_L = SET(1, RM(EA) ); WM( EA,_Z80_L );					} /* SET  1,L=(XY+o)  */
OP(xycb,ce) { WM( EA, SET(1,RM(EA)) );								} /* SET  1,(XY+o)	  */
OP(xycb,cf) { _Z80_A = SET(1, RM(EA) ); WM( EA,_Z80_A );					} /* SET  1,A=(XY+o)  */

OP(xycb,d0) { _Z80_B = SET(2, RM(EA) ); WM( EA,_Z80_B );					} /* SET  2,B=(XY+o)  */
OP(xycb,d1) { _Z80_C = SET(2, RM(EA) ); WM( EA,_Z80_C );					} /* SET  2,C=(XY+o)  */
OP(xycb,d2) { _Z80_D = SET(2, RM(EA) ); WM( EA,_Z80_D );					} /* SET  2,D=(XY+o)  */
OP(xycb,d3) { _Z80_E = SET(2, RM(EA) ); WM( EA,_Z80_E );					} /* SET  2,E=(XY+o)  */
OP(xycb,d4) { _Z80_H = SET(2, RM(EA) ); WM( EA,_Z80_H );					} /* SET  2,H=(XY+o)  */
OP(xycb,d5) { _Z80_L = SET(2, RM(EA) ); WM( EA,_Z80_L );					} /* SET  2,L=(XY+o)  */
OP(xycb,d6) { WM( EA, SET(2,RM(EA)) );								} /* SET  2,(XY+o)	  */
OP(xycb,d7) { _Z80_A = SET(2, RM(EA) ); WM( EA,_Z80_A );					} /* SET  2,A=(XY+o)  */

OP(xycb,d8) { _Z80_B = SET(3, RM(EA) ); WM( EA,_Z80_B );					} /* SET  3,B=(XY+o)  */
OP(xycb,d9) { _Z80_C = SET(3, RM(EA) ); WM( EA,_Z80_C );					} /* SET  3,C=(XY+o)  */
OP(xycb,da) { _Z80_D = SET(3, RM(EA) ); WM( EA,_Z80_D );					} /* SET  3,D=(XY+o)  */
OP(xycb,db) { _Z80_E = SET(3, RM(EA) ); WM( EA,_Z80_E );					} /* SET  3,E=(XY+o)  */
OP(xycb,dc) { _Z80_H = SET(3, RM(EA) ); WM( EA,_Z80_H );					} /* SET  3,H=(XY+o)  */
OP(xycb,dd) { _Z80_L = SET(3, RM(EA) ); WM( EA,_Z80_L );					} /* SET  3,L=(XY+o)  */
OP(xycb,de) { WM( EA, SET(3,RM(EA)) );								} /* SET  3,(XY+o)	  */
OP(xycb,df) { _Z80_A = SET(3, RM(EA) ); WM( EA,_Z80_A );					} /* SET  3,A=(XY+o)  */

OP(xycb,e0) { _Z80_B = SET(4, RM(EA) ); WM( EA,_Z80_B );					} /* SET  4,B=(XY+o)  */
OP(xycb,e1) { _Z80_C = SET(4, RM(EA) ); WM( EA,_Z80_C );					} /* SET  4,C=(XY+o)  */
OP(xycb,e2) { _Z80_D = SET(4, RM(EA) ); WM( EA,_Z80_D );					} /* SET  4,D=(XY+o)  */
OP(xycb,e3) { _Z80_E = SET(4, RM(EA) ); WM( EA,_Z80_E );					} /* SET  4,E=(XY+o)  */
OP(xycb,e4) { _Z80_H = SET(4, RM(EA) ); WM( EA,_Z80_H );					} /* SET  4,H=(XY+o)  */
OP(xycb,e5) { _Z80_L = SET(4, RM(EA) ); WM( EA,_Z80_L );					} /* SET  4,L=(XY+o)  */
OP(xycb,e6) { WM( EA, SET(4,RM(EA)) );								} /* SET  4,(XY+o)	  */
OP(xycb,e7) { _Z80_A = SET(4, RM(EA) ); WM( EA,_Z80_A );					} /* SET  4,A=(XY+o)  */

OP(xycb,e8) { _Z80_B = SET(5, RM(EA) ); WM( EA,_Z80_B );					} /* SET  5,B=(XY+o)  */
OP(xycb,e9) { _Z80_C = SET(5, RM(EA) ); WM( EA,_Z80_C );					} /* SET  5,C=(XY+o)  */
OP(xycb,ea) { _Z80_D = SET(5, RM(EA) ); WM( EA,_Z80_D );					} /* SET  5,D=(XY+o)  */
OP(xycb,eb) { _Z80_E = SET(5, RM(EA) ); WM( EA,_Z80_E );					} /* SET  5,E=(XY+o)  */
OP(xycb,ec) { _Z80_H = SET(5, RM(EA) ); WM( EA,_Z80_H );					} /* SET  5,H=(XY+o)  */
OP(xycb,ed) { _Z80_L = SET(5, RM(EA) ); WM( EA,_Z80_L );					} /* SET  5,L=(XY+o)  */
OP(xycb,ee) { WM( EA, SET(5,RM(EA)) );								} /* SET  5,(XY+o)	  */
OP(xycb,ef) { _Z80_A = SET(5, RM(EA) ); WM( EA,_Z80_A );					} /* SET  5,A=(XY+o)  */

OP(xycb,f0) { _Z80_B = SET(6, RM(EA) ); WM( EA,_Z80_B );					} /* SET  6,B=(XY+o)  */
OP(xycb,f1) { _Z80_C = SET(6, RM(EA) ); WM( EA,_Z80_C );					} /* SET  6,C=(XY+o)  */
OP(xycb,f2) { _Z80_D = SET(6, RM(EA) ); WM( EA,_Z80_D );					} /* SET  6,D=(XY+o)  */
OP(xycb,f3) { _Z80_E = SET(6, RM(EA) ); WM( EA,_Z80_E );					} /* SET  6,E=(XY+o)  */
OP(xycb,f4) { _Z80_H = SET(6, RM(EA) ); WM( EA,_Z80_H );					} /* SET  6,H=(XY+o)  */
OP(xycb,f5) { _Z80_L = SET(6, RM(EA) ); WM( EA,_Z80_L );					} /* SET  6,L=(XY+o)  */
OP(xycb,f6) { WM( EA, SET(6,RM(EA)) );								} /* SET  6,(XY+o)	  */
OP(xycb,f7) { _Z80_A = SET(6, RM(EA) ); WM( EA,_Z80_A );					} /* SET  6,A=(XY+o)  */

OP(xycb,f8) { _Z80_B = SET(7, RM(EA) ); WM( EA,_Z80_B );					} /* SET  7,B=(XY+o)  */
OP(xycb,f9) { _Z80_C = SET(7, RM(EA) ); WM( EA,_Z80_C );					} /* SET  7,C=(XY+o)  */
OP(xycb,fa) { _Z80_D = SET(7, RM(EA) ); WM( EA,_Z80_D );					} /* SET  7,D=(XY+o)  */
OP(xycb,fb) { _Z80_E = SET(7, RM(EA) ); WM( EA,_Z80_E );					} /* SET  7,E=(XY+o)  */
OP(xycb,fc) { _Z80_H = SET(7, RM(EA) ); WM( EA,_Z80_H );					} /* SET  7,H=(XY+o)  */
OP(xycb,fd) { _Z80_L = SET(7, RM(EA) ); WM( EA,_Z80_L );					} /* SET  7,L=(XY+o)  */
OP(xycb,fe) { WM( EA, SET(7,RM(EA)) );								} /* SET  7,(XY+o)	  */
OP(xycb,ff) { _Z80_A = SET(7, RM(EA) ); WM( EA,_Z80_A );					} /* SET  7,A=(XY+o)  */

OP(illegal,1) {
	logerror("Z80 #%d ill. opcode $%02x $%02x\n",
			cpu_getactivecpu(), cpu_readop((_PCD-1)&0xffff), cpu_readop(_PCD));
}

/**********************************************************
 * IX register related opcodes (DD prefix)
 **********************************************************/
OP(dd,00) { illegal_1(); op_00();									} /* DB   DD		  */
OP(dd,01) { illegal_1(); op_01();									} /* DB   DD		  */
OP(dd,02) { illegal_1(); op_02();									} /* DB   DD		  */
OP(dd,03) { illegal_1(); op_03();									} /* DB   DD		  */
OP(dd,04) { illegal_1(); op_04();									} /* DB   DD		  */
OP(dd,05) { illegal_1(); op_05();									} /* DB   DD		  */
OP(dd,06) { illegal_1(); op_06();									} /* DB   DD		  */
OP(dd,07) { illegal_1(); op_07();									} /* DB   DD		  */

OP(dd,08) { illegal_1(); op_08();									} /* DB   DD		  */
OP(dd,09) { _Z80_R++; ADD16(IX,BC);										} /* ADD  IX,BC		  */
OP(dd,0a) { illegal_1(); op_0a();									} /* DB   DD		  */
OP(dd,0b) { illegal_1(); op_0b();									} /* DB   DD		  */
OP(dd,0c) { illegal_1(); op_0c();									} /* DB   DD		  */
OP(dd,0d) { illegal_1(); op_0d();									} /* DB   DD		  */
OP(dd,0e) { illegal_1(); op_0e();									} /* DB   DD		  */
OP(dd,0f) { illegal_1(); op_0f();									} /* DB   DD		  */

OP(dd,10) { illegal_1(); op_10();									} /* DB   DD		  */
OP(dd,11) { illegal_1(); op_11();									} /* DB   DD		  */
OP(dd,12) { illegal_1(); op_12();									} /* DB   DD		  */
OP(dd,13) { illegal_1(); op_13();									} /* DB   DD		  */
OP(dd,14) { illegal_1(); op_14();									} /* DB   DD		  */
OP(dd,15) { illegal_1(); op_15();									} /* DB   DD		  */
OP(dd,16) { illegal_1(); op_16();									} /* DB   DD		  */
OP(dd,17) { illegal_1(); op_17();									} /* DB   DD		  */

OP(dd,18) { illegal_1(); op_18();									} /* DB   DD		  */
OP(dd,19) { _Z80_R++; ADD16(IX,DE);										} /* ADD  IX,DE		  */
OP(dd,1a) { illegal_1(); op_1a();									} /* DB   DD		  */
OP(dd,1b) { illegal_1(); op_1b();									} /* DB   DD		  */
OP(dd,1c) { illegal_1(); op_1c();									} /* DB   DD		  */
OP(dd,1d) { illegal_1(); op_1d();									} /* DB   DD		  */
OP(dd,1e) { illegal_1(); op_1e();									} /* DB   DD		  */
OP(dd,1f) { illegal_1(); op_1f();									} /* DB   DD		  */

OP(dd,20) { illegal_1(); op_20();									} /* DB   DD		  */
OP(dd,21) { _Z80_R++; _IX = ARG16();									} /* LD   IX,w		  */
OP(dd,22) { _Z80_R++; EA = ARG16(); WM16( EA, &Z80.IX );				} /* LD   (w),IX	  */
OP(dd,23) { _Z80_R++; _IX++;											} /* INC  IX		  */
OP(dd,24) { _Z80_R++; _HX = INC(_HX);									} /* INC  HX		  */
OP(dd,25) { _Z80_R++; _HX = DEC(_HX);									} /* DEC  HX		  */
OP(dd,26) { _Z80_R++; _HX = ARG();										} /* LD   HX,n		  */
OP(dd,27) { illegal_1(); op_27();									} /* DB   DD		  */

OP(dd,28) { illegal_1(); op_28();									} /* DB   DD		  */
OP(dd,29) { _Z80_R++; ADD16(IX,IX);										} /* ADD  IX,IX		  */
OP(dd,2a) { _Z80_R++; EA = ARG16(); RM16( EA, &Z80.IX );				} /* LD   IX,(w)	  */
OP(dd,2b) { _Z80_R++; _IX--;											} /* DEC  IX		  */
OP(dd,2c) { _Z80_R++; _LX = INC(_LX);									} /* INC  LX		  */
OP(dd,2d) { _Z80_R++; _LX = DEC(_LX);									} /* DEC  LX		  */
OP(dd,2e) { _Z80_R++; _LX = ARG();										} /* LD   LX,n		  */
OP(dd,2f) { illegal_1(); op_2f();									} /* DB   DD		  */

OP(dd,30) { illegal_1(); op_30();									} /* DB   DD		  */
OP(dd,31) { illegal_1(); op_31();									} /* DB   DD		  */
OP(dd,32) { illegal_1(); op_32();									} /* DB   DD		  */
OP(dd,33) { illegal_1(); op_33();									} /* DB   DD		  */
OP(dd,34) { _Z80_R++; EAX; WM( EA, INC(RM(EA)) );						} /* INC  (IX+o)	  */
OP(dd,35) { _Z80_R++; EAX; WM( EA, DEC(RM(EA)) );						} /* DEC  (IX+o)	  */
OP(dd,36) { _Z80_R++; EAX; WM( EA, ARG() );								} /* LD   (IX+o),n	  */
OP(dd,37) { illegal_1(); op_37();									} /* DB   DD		  */

OP(dd,38) { illegal_1(); op_38();									} /* DB   DD		  */
OP(dd,39) { _Z80_R++; ADD16(IX,SP);										} /* ADD  IX,SP		  */
OP(dd,3a) { illegal_1(); op_3a();									} /* DB   DD		  */
OP(dd,3b) { illegal_1(); op_3b();									} /* DB   DD		  */
OP(dd,3c) { illegal_1(); op_3c();									} /* DB   DD		  */
OP(dd,3d) { illegal_1(); op_3d();									} /* DB   DD		  */
OP(dd,3e) { illegal_1(); op_3e();									} /* DB   DD		  */
OP(dd,3f) { illegal_1(); op_3f();									} /* DB   DD		  */

OP(dd,40) { illegal_1(); op_40();									} /* DB   DD		  */
OP(dd,41) { illegal_1(); op_41();									} /* DB   DD		  */
OP(dd,42) { illegal_1(); op_42();									} /* DB   DD		  */
OP(dd,43) { illegal_1(); op_43();									} /* DB   DD		  */
OP(dd,44) { _Z80_R++; _Z80_B = _HX;											} /* LD   B,HX		  */
OP(dd,45) { _Z80_R++; _Z80_B = _LX;											} /* LD   B,LX		  */
OP(dd,46) { _Z80_R++; EAX; _Z80_B = RM(EA);									} /* LD   B,(IX+o)	  */
OP(dd,47) { illegal_1(); op_47();									} /* DB   DD		  */

OP(dd,48) { illegal_1(); op_48();									} /* DB   DD		  */
OP(dd,49) { illegal_1(); op_49();									} /* DB   DD		  */
OP(dd,4a) { illegal_1(); op_4a();									} /* DB   DD		  */
OP(dd,4b) { illegal_1(); op_4b();									} /* DB   DD		  */
OP(dd,4c) { _Z80_R++; _Z80_C = _HX;											} /* LD   C,HX		  */
OP(dd,4d) { _Z80_R++; _Z80_C = _LX;											} /* LD   C,LX		  */
OP(dd,4e) { _Z80_R++; EAX; _Z80_C = RM(EA);									} /* LD   C,(IX+o)	  */
OP(dd,4f) { illegal_1(); op_4f();									} /* DB   DD		  */

OP(dd,50) { illegal_1(); op_50();									} /* DB   DD		  */
OP(dd,51) { illegal_1(); op_51();									} /* DB   DD		  */
OP(dd,52) { illegal_1(); op_52();									} /* DB   DD		  */
OP(dd,53) { illegal_1(); op_53();									} /* DB   DD		  */
OP(dd,54) { _Z80_R++; _Z80_D = _HX;											} /* LD   D,HX		  */
OP(dd,55) { _Z80_R++; _Z80_D = _LX;											} /* LD   D,LX		  */
OP(dd,56) { _Z80_R++; EAX; _Z80_D = RM(EA);									} /* LD   D,(IX+o)	  */
OP(dd,57) { illegal_1(); op_57();									} /* DB   DD		  */

OP(dd,58) { illegal_1(); op_58();									} /* DB   DD		  */
OP(dd,59) { illegal_1(); op_59();									} /* DB   DD		  */
OP(dd,5a) { illegal_1(); op_5a();									} /* DB   DD		  */
OP(dd,5b) { illegal_1(); op_5b();									} /* DB   DD		  */
OP(dd,5c) { _Z80_R++; _Z80_E = _HX;											} /* LD   E,HX		  */
OP(dd,5d) { _Z80_R++; _Z80_E = _LX;											} /* LD   E,LX		  */
OP(dd,5e) { _Z80_R++; EAX; _Z80_E = RM(EA);									} /* LD   E,(IX+o)	  */
OP(dd,5f) { illegal_1(); op_5f();									} /* DB   DD		  */

OP(dd,60) { _Z80_R++; _HX = _Z80_B;											} /* LD   HX,B		  */
OP(dd,61) { _Z80_R++; _HX = _Z80_C;											} /* LD   HX,C		  */
OP(dd,62) { _Z80_R++; _HX = _Z80_D;											} /* LD   HX,D		  */
OP(dd,63) { _Z80_R++; _HX = _Z80_E;											} /* LD   HX,E		  */
OP(dd,64) {															} /* LD   HX,HX		  */
OP(dd,65) { _Z80_R++; _HX = _LX;										} /* LD   HX,LX		  */
OP(dd,66) { _Z80_R++; EAX; _Z80_H = RM(EA);									} /* LD   H,(IX+o)	  */
OP(dd,67) { _Z80_R++; _HX = _Z80_A;											} /* LD   HX,A		  */

OP(dd,68) { _Z80_R++; _LX = _Z80_B;											} /* LD   LX,B		  */
OP(dd,69) { _Z80_R++; _LX = _Z80_C;											} /* LD   LX,C		  */
OP(dd,6a) { _Z80_R++; _LX = _Z80_D;											} /* LD   LX,D		  */
OP(dd,6b) { _Z80_R++; _LX = _Z80_E;											} /* LD   LX,E		  */
OP(dd,6c) { _Z80_R++; _LX = _HX;										} /* LD   LX,HX		  */
OP(dd,6d) {															} /* LD   LX,LX		  */
OP(dd,6e) { _Z80_R++; EAX; _Z80_L = RM(EA);									} /* LD   L,(IX+o)	  */
OP(dd,6f) { _Z80_R++; _LX = _Z80_A;											} /* LD   LX,A		  */

OP(dd,70) { _Z80_R++; EAX; WM( EA, _Z80_B );								} /* LD   (IX+o),B	  */
OP(dd,71) { _Z80_R++; EAX; WM( EA, _Z80_C );								} /* LD   (IX+o),C	  */
OP(dd,72) { _Z80_R++; EAX; WM( EA, _Z80_D );								} /* LD   (IX+o),D	  */
OP(dd,73) { _Z80_R++; EAX; WM( EA, _Z80_E );								} /* LD   (IX+o),E	  */
OP(dd,74) { _Z80_R++; EAX; WM( EA, _Z80_H );								} /* LD   (IX+o),H	  */
OP(dd,75) { _Z80_R++; EAX; WM( EA, _Z80_L );								} /* LD   (IX+o),L	  */
OP(dd,76) { illegal_1(); op_76();									}		  /* DB   DD		  */
OP(dd,77) { _Z80_R++; EAX; WM( EA, _Z80_A );								} /* LD   (IX+o),A	  */

OP(dd,78) { illegal_1(); op_78();									} /* DB   DD		  */
OP(dd,79) { illegal_1(); op_79();									} /* DB   DD		  */
OP(dd,7a) { illegal_1(); op_7a();									} /* DB   DD		  */
OP(dd,7b) { illegal_1(); op_7b();									} /* DB   DD		  */
OP(dd,7c) { _Z80_R++; _Z80_A = _HX;											} /* LD   A,HX		  */
OP(dd,7d) { _Z80_R++; _Z80_A = _LX;											} /* LD   A,LX		  */
OP(dd,7e) { _Z80_R++; EAX; _Z80_A = RM(EA);									} /* LD   A,(IX+o)	  */
OP(dd,7f) { illegal_1(); op_7f();									} /* DB   DD		  */

OP(dd,80) { illegal_1(); op_80();									} /* DB   DD		  */
OP(dd,81) { illegal_1(); op_81();									} /* DB   DD		  */
OP(dd,82) { illegal_1(); op_82();									} /* DB   DD		  */
OP(dd,83) { illegal_1(); op_83();									} /* DB   DD		  */
OP(dd,84) { _Z80_R++; ADD(_HX);											} /* ADD  A,HX		  */
OP(dd,85) { _Z80_R++; ADD(_LX);											} /* ADD  A,LX		  */
OP(dd,86) { _Z80_R++; EAX; ADD(RM(EA));									} /* ADD  A,(IX+o)	  */
OP(dd,87) { illegal_1(); op_87();									} /* DB   DD		  */

OP(dd,88) { illegal_1(); op_88();									} /* DB   DD		  */
OP(dd,89) { illegal_1(); op_89();									} /* DB   DD		  */
OP(dd,8a) { illegal_1(); op_8a();									} /* DB   DD		  */
OP(dd,8b) { illegal_1(); op_8b();									} /* DB   DD		  */
OP(dd,8c) { _Z80_R++; ADC(_HX);											} /* ADC  A,HX		  */
OP(dd,8d) { _Z80_R++; ADC(_LX);											} /* ADC  A,LX		  */
OP(dd,8e) { _Z80_R++; EAX; ADC(RM(EA));									} /* ADC  A,(IX+o)	  */
OP(dd,8f) { illegal_1(); op_8f();									} /* DB   DD		  */

OP(dd,90) { illegal_1(); op_90();									} /* DB   DD		  */
OP(dd,91) { illegal_1(); op_91();									} /* DB   DD		  */
OP(dd,92) { illegal_1(); op_92();									} /* DB   DD		  */
OP(dd,93) { illegal_1(); op_93();									} /* DB   DD		  */
OP(dd,94) { _Z80_R++; SUB(_HX);											} /* SUB  HX		  */
OP(dd,95) { _Z80_R++; SUB(_LX);											} /* SUB  LX		  */
OP(dd,96) { _Z80_R++; EAX; SUB(RM(EA));									} /* SUB  (IX+o)	  */
OP(dd,97) { illegal_1(); op_97();									} /* DB   DD		  */

OP(dd,98) { illegal_1(); op_98();									} /* DB   DD		  */
OP(dd,99) { illegal_1(); op_99();									} /* DB   DD		  */
OP(dd,9a) { illegal_1(); op_9a();									} /* DB   DD		  */
OP(dd,9b) { illegal_1(); op_9b();									} /* DB   DD		  */
OP(dd,9c) { _Z80_R++; SBC(_HX);											} /* SBC  A,HX		  */
OP(dd,9d) { _Z80_R++; SBC(_LX);											} /* SBC  A,LX		  */
OP(dd,9e) { _Z80_R++; EAX; SBC(RM(EA));									} /* SBC  A,(IX+o)	  */
OP(dd,9f) { illegal_1(); op_9f();									} /* DB   DD		  */

OP(dd,a0) { illegal_1(); op_a0();									} /* DB   DD		  */
OP(dd,a1) { illegal_1(); op_a1();									} /* DB   DD		  */
OP(dd,a2) { illegal_1(); op_a2();									} /* DB   DD		  */
OP(dd,a3) { illegal_1(); op_a3();									} /* DB   DD		  */
OP(dd,a4) { _Z80_R++; AND(_HX);											} /* AND  HX		  */
OP(dd,a5) { _Z80_R++; AND(_LX);											} /* AND  LX		  */
OP(dd,a6) { _Z80_R++; EAX; AND(RM(EA));									} /* AND  (IX+o)	  */
OP(dd,a7) { illegal_1(); op_a7();									} /* DB   DD		  */

OP(dd,a8) { illegal_1(); op_a8();									} /* DB   DD		  */
OP(dd,a9) { illegal_1(); op_a9();									} /* DB   DD		  */
OP(dd,aa) { illegal_1(); op_aa();									} /* DB   DD		  */
OP(dd,ab) { illegal_1(); op_ab();									} /* DB   DD		  */
OP(dd,ac) { _Z80_R++; XOR(_HX);											} /* XOR  HX		  */
OP(dd,ad) { _Z80_R++; XOR(_LX);											} /* XOR  LX		  */
OP(dd,ae) { _Z80_R++; EAX; XOR(RM(EA));									} /* XOR  (IX+o)	  */
OP(dd,af) { illegal_1(); op_af();									} /* DB   DD		  */

OP(dd,b0) { illegal_1(); op_b0();									} /* DB   DD		  */
OP(dd,b1) { illegal_1(); op_b1();									} /* DB   DD		  */
OP(dd,b2) { illegal_1(); op_b2();									} /* DB   DD		  */
OP(dd,b3) { illegal_1(); op_b3();									} /* DB   DD		  */
OP(dd,b4) { _Z80_R++; OR(_HX);											} /* OR   HX		  */
OP(dd,b5) { _Z80_R++; OR(_LX);											} /* OR   LX		  */
OP(dd,b6) { _Z80_R++; EAX; OR(RM(EA));									} /* OR   (IX+o)	  */
OP(dd,b7) { illegal_1(); op_b7();									} /* DB   DD		  */

OP(dd,b8) { illegal_1(); op_b8();									} /* DB   DD		  */
OP(dd,b9) { illegal_1(); op_b9();									} /* DB   DD		  */
OP(dd,ba) { illegal_1(); op_ba();									} /* DB   DD		  */
OP(dd,bb) { illegal_1(); op_bb();									} /* DB   DD		  */
OP(dd,bc) { _Z80_R++; CP(_HX);											} /* CP   HX		  */
OP(dd,bd) { _Z80_R++; CP(_LX);											} /* CP   LX		  */
OP(dd,be) { _Z80_R++; EAX; CP(RM(EA));									} /* CP   (IX+o)	  */
OP(dd,bf) { illegal_1(); op_bf();									} /* DB   DD		  */

OP(dd,c0) { illegal_1(); op_c0();									} /* DB   DD		  */
OP(dd,c1) { illegal_1(); op_c1();									} /* DB   DD		  */
OP(dd,c2) { illegal_1(); op_c2();									} /* DB   DD		  */
OP(dd,c3) { illegal_1(); op_c3();									} /* DB   DD		  */
OP(dd,c4) { illegal_1(); op_c4();									} /* DB   DD		  */
OP(dd,c5) { illegal_1(); op_c5();									} /* DB   DD		  */
OP(dd,c6) { illegal_1(); op_c6();									} /* DB   DD		  */
OP(dd,c7) { illegal_1(); op_c7();									}		  /* DB   DD		  */

OP(dd,c8) { illegal_1(); op_c8();									} /* DB   DD		  */
OP(dd,c9) { illegal_1(); op_c9();									} /* DB   DD		  */
OP(dd,ca) { illegal_1(); op_ca();									} /* DB   DD		  */
OP(dd,cb) { _Z80_R++; EAX; EXEC(xycb,ARG());							} /* **   DD CB xx	  */
OP(dd,cc) { illegal_1(); op_cc();									} /* DB   DD		  */
OP(dd,cd) { illegal_1(); op_cd();									} /* DB   DD		  */
OP(dd,ce) { illegal_1(); op_ce();									} /* DB   DD		  */
OP(dd,cf) { illegal_1(); op_cf();									} /* DB   DD		  */

OP(dd,d0) { illegal_1(); op_d0();									} /* DB   DD		  */
OP(dd,d1) { illegal_1(); op_d1();									} /* DB   DD		  */
OP(dd,d2) { illegal_1(); op_d2();									} /* DB   DD		  */
OP(dd,d3) { illegal_1(); op_d3();									} /* DB   DD		  */
OP(dd,d4) { illegal_1(); op_d4();									} /* DB   DD		  */
OP(dd,d5) { illegal_1(); op_d5();									} /* DB   DD		  */
OP(dd,d6) { illegal_1(); op_d6();									} /* DB   DD		  */
OP(dd,d7) { illegal_1(); op_d7();									} /* DB   DD		  */

OP(dd,d8) { illegal_1(); op_d8();									} /* DB   DD		  */
OP(dd,d9) { illegal_1(); op_d9();									} /* DB   DD		  */
OP(dd,da) { illegal_1(); op_da();									} /* DB   DD		  */
OP(dd,db) { illegal_1(); op_db();									} /* DB   DD		  */
OP(dd,dc) { illegal_1(); op_dc();									} /* DB   DD		  */
OP(dd,dd) { illegal_1(); op_dd();									} /* DB   DD		  */
OP(dd,de) { illegal_1(); op_de();									} /* DB   DD		  */
OP(dd,df) { illegal_1(); op_df();									} /* DB   DD		  */

OP(dd,e0) { illegal_1(); op_e0();									} /* DB   DD		  */
OP(dd,e1) { _Z80_R++; POP(IX);											} /* POP  IX		  */
OP(dd,e2) { illegal_1(); op_e2();									} /* DB   DD		  */
OP(dd,e3) { _Z80_R++; EXSP(IX);											} /* EX   (SP),IX	  */
OP(dd,e4) { illegal_1(); op_e4();									} /* DB   DD		  */
OP(dd,e5) { _Z80_R++; PUSH( IX );										} /* PUSH IX		  */
OP(dd,e6) { illegal_1(); op_e6();									} /* DB   DD		  */
OP(dd,e7) { illegal_1(); op_e7();									} /* DB   DD		  */

OP(dd,e8) { illegal_1(); op_e8();									} /* DB   DD		  */
OP(dd,e9) { _Z80_R++; _PC = _IX; change_pc16(_PCD);						} /* JP   (IX)		  */
OP(dd,ea) { illegal_1(); op_ea();									} /* DB   DD		  */
OP(dd,eb) { illegal_1(); op_eb();									} /* DB   DD		  */
OP(dd,ec) { illegal_1(); op_ec();									} /* DB   DD		  */
OP(dd,ed) { illegal_1(); op_ed();									} /* DB   DD		  */
OP(dd,ee) { illegal_1(); op_ee();									} /* DB   DD		  */
OP(dd,ef) { illegal_1(); op_ef();									} /* DB   DD		  */

OP(dd,f0) { illegal_1(); op_f0();									} /* DB   DD		  */
OP(dd,f1) { illegal_1(); op_f1();									} /* DB   DD		  */
OP(dd,f2) { illegal_1(); op_f2();									} /* DB   DD		  */
OP(dd,f3) { illegal_1(); op_f3();									} /* DB   DD		  */
OP(dd,f4) { illegal_1(); op_f4();									} /* DB   DD		  */
OP(dd,f5) { illegal_1(); op_f5();									} /* DB   DD		  */
OP(dd,f6) { illegal_1(); op_f6();									} /* DB   DD		  */
OP(dd,f7) { illegal_1(); op_f7();									} /* DB   DD		  */

OP(dd,f8) { illegal_1(); op_f8();									} /* DB   DD		  */
OP(dd,f9) { _Z80_R++; _SP = _IX;										} /* LD   SP,IX		  */
OP(dd,fa) { illegal_1(); op_fa();									} /* DB   DD		  */
OP(dd,fb) { illegal_1(); op_fb();									} /* DB   DD		  */
OP(dd,fc) { illegal_1(); op_fc();									} /* DB   DD		  */
OP(dd,fd) { illegal_1(); op_fd();									} /* DB   DD		  */
OP(dd,fe) { illegal_1(); op_fe();									} /* DB   DD		  */
OP(dd,ff) { illegal_1(); op_ff();									} /* DB   DD		  */

/**********************************************************
 * IY register related opcodes (FD prefix)
 **********************************************************/
OP(fd,00) { illegal_1(); op_00();									} /* DB   FD		  */
OP(fd,01) { illegal_1(); op_01();									} /* DB   FD		  */
OP(fd,02) { illegal_1(); op_02();									} /* DB   FD		  */
OP(fd,03) { illegal_1(); op_03();									} /* DB   FD		  */
OP(fd,04) { illegal_1(); op_04();									} /* DB   FD		  */
OP(fd,05) { illegal_1(); op_05();									} /* DB   FD		  */
OP(fd,06) { illegal_1(); op_06();									} /* DB   FD		  */
OP(fd,07) { illegal_1(); op_07();									} /* DB   FD		  */

OP(fd,08) { illegal_1(); op_08();									} /* DB   FD		  */
OP(fd,09) { _Z80_R++; ADD16(IY,BC);										} /* ADD  IY,BC		  */
OP(fd,0a) { illegal_1(); op_0a();									} /* DB   FD		  */
OP(fd,0b) { illegal_1(); op_0b();									} /* DB   FD		  */
OP(fd,0c) { illegal_1(); op_0c();									} /* DB   FD		  */
OP(fd,0d) { illegal_1(); op_0d();									} /* DB   FD		  */
OP(fd,0e) { illegal_1(); op_0e();									} /* DB   FD		  */
OP(fd,0f) { illegal_1(); op_0f();									} /* DB   FD		  */

OP(fd,10) { illegal_1(); op_10();									} /* DB   FD		  */
OP(fd,11) { illegal_1(); op_11();									} /* DB   FD		  */
OP(fd,12) { illegal_1(); op_12();									} /* DB   FD		  */
OP(fd,13) { illegal_1(); op_13();									} /* DB   FD		  */
OP(fd,14) { illegal_1(); op_14();									} /* DB   FD		  */
OP(fd,15) { illegal_1(); op_15();									} /* DB   FD		  */
OP(fd,16) { illegal_1(); op_16();									} /* DB   FD		  */
OP(fd,17) { illegal_1(); op_17();									} /* DB   FD		  */

OP(fd,18) { illegal_1(); op_18();									} /* DB   FD		  */
OP(fd,19) { _Z80_R++; ADD16(IY,DE);										} /* ADD  IY,DE		  */
OP(fd,1a) { illegal_1(); op_1a();									} /* DB   FD		  */
OP(fd,1b) { illegal_1(); op_1b();									} /* DB   FD		  */
OP(fd,1c) { illegal_1(); op_1c();									} /* DB   FD		  */
OP(fd,1d) { illegal_1(); op_1d();									} /* DB   FD		  */
OP(fd,1e) { illegal_1(); op_1e();									} /* DB   FD		  */
OP(fd,1f) { illegal_1(); op_1f();									} /* DB   FD		  */

OP(fd,20) { illegal_1(); op_20();									} /* DB   FD		  */
OP(fd,21) { _Z80_R++; _IY = ARG16();									} /* LD   IY,w		  */
OP(fd,22) { _Z80_R++; EA = ARG16(); WM16( EA, &Z80.IY );				} /* LD   (w),IY	  */
OP(fd,23) { _Z80_R++; _IY++;											} /* INC  IY		  */
OP(fd,24) { _Z80_R++; _HY = INC(_HY);									} /* INC  HY		  */
OP(fd,25) { _Z80_R++; _HY = DEC(_HY);									} /* DEC  HY		  */
OP(fd,26) { _Z80_R++; _HY = ARG();										} /* LD   HY,n		  */
OP(fd,27) { illegal_1(); op_27();									} /* DB   FD		  */

OP(fd,28) { illegal_1(); op_28();									} /* DB   FD		  */
OP(fd,29) { _Z80_R++; ADD16(IY,IY);										} /* ADD  IY,IY		  */
OP(fd,2a) { _Z80_R++; EA = ARG16(); RM16( EA, &Z80.IY );				} /* LD   IY,(w)	  */
OP(fd,2b) { _Z80_R++; _IY--;											} /* DEC  IY		  */
OP(fd,2c) { _Z80_R++; _LY = INC(_LY);									} /* INC  LY		  */
OP(fd,2d) { _Z80_R++; _LY = DEC(_LY);									} /* DEC  LY		  */
OP(fd,2e) { _Z80_R++; _LY = ARG();										} /* LD   LY,n		  */
OP(fd,2f) { illegal_1(); op_2f();									} /* DB   FD		  */

OP(fd,30) { illegal_1(); op_30();									} /* DB   FD		  */
OP(fd,31) { illegal_1(); op_31();									} /* DB   FD		  */
OP(fd,32) { illegal_1(); op_32();									} /* DB   FD		  */
OP(fd,33) { illegal_1(); op_33();									} /* DB   FD		  */
OP(fd,34) { _Z80_R++; EAY; WM( EA, INC(RM(EA)) );						} /* INC  (IY+o)	  */
OP(fd,35) { _Z80_R++; EAY; WM( EA, DEC(RM(EA)) );						} /* DEC  (IY+o)	  */
OP(fd,36) { _Z80_R++; EAY; WM( EA, ARG() );								} /* LD   (IY+o),n	  */
OP(fd,37) { illegal_1(); op_37();									} /* DB   FD		  */

OP(fd,38) { illegal_1(); op_38();									} /* DB   FD		  */
OP(fd,39) { _Z80_R++; ADD16(IY,SP);										} /* ADD  IY,SP		  */
OP(fd,3a) { illegal_1(); op_3a();									} /* DB   FD		  */
OP(fd,3b) { illegal_1(); op_3b();									} /* DB   FD		  */
OP(fd,3c) { illegal_1(); op_3c();									} /* DB   FD		  */
OP(fd,3d) { illegal_1(); op_3d();									} /* DB   FD		  */
OP(fd,3e) { illegal_1(); op_3e();									} /* DB   FD		  */
OP(fd,3f) { illegal_1(); op_3f();									} /* DB   FD		  */

OP(fd,40) { illegal_1(); op_40();									} /* DB   FD		  */
OP(fd,41) { illegal_1(); op_41();									} /* DB   FD		  */
OP(fd,42) { illegal_1(); op_42();									} /* DB   FD		  */
OP(fd,43) { illegal_1(); op_43();									} /* DB   FD		  */
OP(fd,44) { _Z80_R++; _Z80_B = _HY;											} /* LD   B,HY		  */
OP(fd,45) { _Z80_R++; _Z80_B = _LY;											} /* LD   B,LY		  */
OP(fd,46) { _Z80_R++; EAY; _Z80_B = RM(EA);									} /* LD   B,(IY+o)	  */
OP(fd,47) { illegal_1(); op_47();									} /* DB   FD		  */

OP(fd,48) { illegal_1(); op_48();									} /* DB   FD		  */
OP(fd,49) { illegal_1(); op_49();									} /* DB   FD		  */
OP(fd,4a) { illegal_1(); op_4a();									} /* DB   FD		  */
OP(fd,4b) { illegal_1(); op_4b();									} /* DB   FD		  */
OP(fd,4c) { _Z80_R++; _Z80_C = _HY;											} /* LD   C,HY		  */
OP(fd,4d) { _Z80_R++; _Z80_C = _LY;											} /* LD   C,LY		  */
OP(fd,4e) { _Z80_R++; EAY; _Z80_C = RM(EA);									} /* LD   C,(IY+o)	  */
OP(fd,4f) { illegal_1(); op_4f();									} /* DB   FD		  */

OP(fd,50) { illegal_1(); op_50();									} /* DB   FD		  */
OP(fd,51) { illegal_1(); op_51();									} /* DB   FD		  */
OP(fd,52) { illegal_1(); op_52();									} /* DB   FD		  */
OP(fd,53) { illegal_1(); op_53();									} /* DB   FD		  */
OP(fd,54) { _Z80_R++; _Z80_D = _HY;											} /* LD   D,HY		  */
OP(fd,55) { _Z80_R++; _Z80_D = _LY;											} /* LD   D,LY		  */
OP(fd,56) { _Z80_R++; EAY; _Z80_D = RM(EA);									} /* LD   D,(IY+o)	  */
OP(fd,57) { illegal_1(); op_57();									} /* DB   FD		  */

OP(fd,58) { illegal_1(); op_58();									} /* DB   FD		  */
OP(fd,59) { illegal_1(); op_59();									} /* DB   FD		  */
OP(fd,5a) { illegal_1(); op_5a();									} /* DB   FD		  */
OP(fd,5b) { illegal_1(); op_5b();									} /* DB   FD		  */
OP(fd,5c) { _Z80_R++; _Z80_E = _HY;											} /* LD   E,HY		  */
OP(fd,5d) { _Z80_R++; _Z80_E = _LY;											} /* LD   E,LY		  */
OP(fd,5e) { _Z80_R++; EAY; _Z80_E = RM(EA);									} /* LD   E,(IY+o)	  */
OP(fd,5f) { illegal_1(); op_5f();									} /* DB   FD		  */

OP(fd,60) { _Z80_R++; _HY = _Z80_B;											} /* LD   HY,B		  */
OP(fd,61) { _Z80_R++; _HY = _Z80_C;											} /* LD   HY,C		  */
OP(fd,62) { _Z80_R++; _HY = _Z80_D;											} /* LD   HY,D		  */
OP(fd,63) { _Z80_R++; _HY = _Z80_E;											} /* LD   HY,E		  */
OP(fd,64) { _Z80_R++;													} /* LD   HY,HY		  */
OP(fd,65) { _Z80_R++; _HY = _LY;										} /* LD   HY,LY		  */
OP(fd,66) { _Z80_R++; EAY; _Z80_H = RM(EA);									} /* LD   H,(IY+o)	  */
OP(fd,67) { _Z80_R++; _HY = _Z80_A;											} /* LD   HY,A		  */

OP(fd,68) { _Z80_R++; _LY = _Z80_B;											} /* LD   LY,B		  */
OP(fd,69) { _Z80_R++; _LY = _Z80_C;											} /* LD   LY,C		  */
OP(fd,6a) { _Z80_R++; _LY = _Z80_D;											} /* LD   LY,D		  */
OP(fd,6b) { _Z80_R++; _LY = _Z80_E;											} /* LD   LY,E		  */
OP(fd,6c) { _Z80_R++; _LY = _HY;										} /* LD   LY,HY		  */
OP(fd,6d) { _Z80_R++;													} /* LD   LY,LY		  */
OP(fd,6e) { _Z80_R++; EAY; _Z80_L = RM(EA);									} /* LD   L,(IY+o)	  */
OP(fd,6f) { _Z80_R++; _LY = _Z80_A;											} /* LD   LY,A		  */

OP(fd,70) { _Z80_R++; EAY; WM( EA, _Z80_B );								} /* LD   (IY+o),B	  */
OP(fd,71) { _Z80_R++; EAY; WM( EA, _Z80_C );								} /* LD   (IY+o),C	  */
OP(fd,72) { _Z80_R++; EAY; WM( EA, _Z80_D );								} /* LD   (IY+o),D	  */
OP(fd,73) { _Z80_R++; EAY; WM( EA, _Z80_E );								} /* LD   (IY+o),E	  */
OP(fd,74) { _Z80_R++; EAY; WM( EA, _Z80_H );								} /* LD   (IY+o),H	  */
OP(fd,75) { _Z80_R++; EAY; WM( EA, _Z80_L );								} /* LD   (IY+o),L	  */
OP(fd,76) { illegal_1(); op_76();									}		  /* DB   FD		  */
OP(fd,77) { _Z80_R++; EAY; WM( EA, _Z80_A );								} /* LD   (IY+o),A	  */

OP(fd,78) { illegal_1(); op_78();									} /* DB   FD		  */
OP(fd,79) { illegal_1(); op_79();									} /* DB   FD		  */
OP(fd,7a) { illegal_1(); op_7a();									} /* DB   FD		  */
OP(fd,7b) { illegal_1(); op_7b();									} /* DB   FD		  */
OP(fd,7c) { _Z80_R++; _Z80_A = _HY;											} /* LD   A,HY		  */
OP(fd,7d) { _Z80_R++; _Z80_A = _LY;											} /* LD   A,LY		  */
OP(fd,7e) { _Z80_R++; EAY; _Z80_A = RM(EA);									} /* LD   A,(IY+o)	  */
OP(fd,7f) { illegal_1(); op_7f();									} /* DB   FD		  */

OP(fd,80) { illegal_1(); op_80();									} /* DB   FD		  */
OP(fd,81) { illegal_1(); op_81();									} /* DB   FD		  */
OP(fd,82) { illegal_1(); op_82();									} /* DB   FD		  */
OP(fd,83) { illegal_1(); op_83();									} /* DB   FD		  */
OP(fd,84) { _Z80_R++; ADD(_HY);											} /* ADD  A,HY		  */
OP(fd,85) { _Z80_R++; ADD(_LY);											} /* ADD  A,LY		  */
OP(fd,86) { _Z80_R++; EAY; ADD(RM(EA));									} /* ADD  A,(IY+o)	  */
OP(fd,87) { illegal_1(); op_87();									} /* DB   FD		  */

OP(fd,88) { illegal_1(); op_88();									} /* DB   FD		  */
OP(fd,89) { illegal_1(); op_89();									} /* DB   FD		  */
OP(fd,8a) { illegal_1(); op_8a();									} /* DB   FD		  */
OP(fd,8b) { illegal_1(); op_8b();									} /* DB   FD		  */
OP(fd,8c) { _Z80_R++; ADC(_HY);											} /* ADC  A,HY		  */
OP(fd,8d) { _Z80_R++; ADC(_LY);											} /* ADC  A,LY		  */
OP(fd,8e) { _Z80_R++; EAY; ADC(RM(EA));									} /* ADC  A,(IY+o)	  */
OP(fd,8f) { illegal_1(); op_8f();									} /* DB   FD		  */

OP(fd,90) { illegal_1(); op_90();									} /* DB   FD		  */
OP(fd,91) { illegal_1(); op_91();									} /* DB   FD		  */
OP(fd,92) { illegal_1(); op_92();									} /* DB   FD		  */
OP(fd,93) { illegal_1(); op_93();									} /* DB   FD		  */
OP(fd,94) { _Z80_R++; SUB(_HY);											} /* SUB  HY		  */
OP(fd,95) { _Z80_R++; SUB(_LY);											} /* SUB  LY		  */
OP(fd,96) { _Z80_R++; EAY; SUB(RM(EA));									} /* SUB  (IY+o)	  */
OP(fd,97) { illegal_1(); op_97();									} /* DB   FD		  */

OP(fd,98) { illegal_1(); op_98();									} /* DB   FD		  */
OP(fd,99) { illegal_1(); op_99();									} /* DB   FD		  */
OP(fd,9a) { illegal_1(); op_9a();									} /* DB   FD		  */
OP(fd,9b) { illegal_1(); op_9b();									} /* DB   FD		  */
OP(fd,9c) { _Z80_R++; SBC(_HY);											} /* SBC  A,HY		  */
OP(fd,9d) { _Z80_R++; SBC(_LY);											} /* SBC  A,LY		  */
OP(fd,9e) { _Z80_R++; EAY; SBC(RM(EA));									} /* SBC  A,(IY+o)	  */
OP(fd,9f) { illegal_1(); op_9f();									} /* DB   FD		  */

OP(fd,a0) { illegal_1(); op_a0();									} /* DB   FD		  */
OP(fd,a1) { illegal_1(); op_a1();									} /* DB   FD		  */
OP(fd,a2) { illegal_1(); op_a2();									} /* DB   FD		  */
OP(fd,a3) { illegal_1(); op_a3();									} /* DB   FD		  */
OP(fd,a4) { _Z80_R++; AND(_HY);											} /* AND  HY		  */
OP(fd,a5) { _Z80_R++; AND(_LY);											} /* AND  LY		  */
OP(fd,a6) { _Z80_R++; EAY; AND(RM(EA));									} /* AND  (IY+o)	  */
OP(fd,a7) { illegal_1(); op_a7();									} /* DB   FD		  */

OP(fd,a8) { illegal_1(); op_a8();									} /* DB   FD		  */
OP(fd,a9) { illegal_1(); op_a9();									} /* DB   FD		  */
OP(fd,aa) { illegal_1(); op_aa();									} /* DB   FD		  */
OP(fd,ab) { illegal_1(); op_ab();									} /* DB   FD		  */
OP(fd,ac) { _Z80_R++; XOR(_HY);											} /* XOR  HY		  */
OP(fd,ad) { _Z80_R++; XOR(_LY);											} /* XOR  LY		  */
OP(fd,ae) { _Z80_R++; EAY; XOR(RM(EA));									} /* XOR  (IY+o)	  */
OP(fd,af) { illegal_1(); op_af();									} /* DB   FD		  */

OP(fd,b0) { illegal_1(); op_b0();									} /* DB   FD		  */
OP(fd,b1) { illegal_1(); op_b1();									} /* DB   FD		  */
OP(fd,b2) { illegal_1(); op_b2();									} /* DB   FD		  */
OP(fd,b3) { illegal_1(); op_b3();									} /* DB   FD		  */
OP(fd,b4) { _Z80_R++; OR(_HY);											} /* OR   HY		  */
OP(fd,b5) { _Z80_R++; OR(_LY);											} /* OR   LY		  */
OP(fd,b6) { _Z80_R++; EAY; OR(RM(EA));									} /* OR   (IY+o)	  */
OP(fd,b7) { illegal_1(); op_b7();									} /* DB   FD		  */

OP(fd,b8) { illegal_1(); op_b8();									} /* DB   FD		  */
OP(fd,b9) { illegal_1(); op_b9();									} /* DB   FD		  */
OP(fd,ba) { illegal_1(); op_ba();									} /* DB   FD		  */
OP(fd,bb) { illegal_1(); op_bb();									} /* DB   FD		  */
OP(fd,bc) { _Z80_R++; CP(_HY);											} /* CP   HY		  */
OP(fd,bd) { _Z80_R++; CP(_LY);											} /* CP   LY		  */
OP(fd,be) { _Z80_R++; EAY; CP(RM(EA));									} /* CP   (IY+o)	  */
OP(fd,bf) { illegal_1(); op_bf();									} /* DB   FD		  */

OP(fd,c0) { illegal_1(); op_c0();									} /* DB   FD		  */
OP(fd,c1) { illegal_1(); op_c1();									} /* DB   FD		  */
OP(fd,c2) { illegal_1(); op_c2();									} /* DB   FD		  */
OP(fd,c3) { illegal_1(); op_c3();									} /* DB   FD		  */
OP(fd,c4) { illegal_1(); op_c4();									} /* DB   FD		  */
OP(fd,c5) { illegal_1(); op_c5();									} /* DB   FD		  */
OP(fd,c6) { illegal_1(); op_c6();									} /* DB   FD		  */
OP(fd,c7) { illegal_1(); op_c7();									} /* DB   FD		  */

OP(fd,c8) { illegal_1(); op_c8();									} /* DB   FD		  */
OP(fd,c9) { illegal_1(); op_c9();									} /* DB   FD		  */
OP(fd,ca) { illegal_1(); op_ca();									} /* DB   FD		  */
OP(fd,cb) { _Z80_R++; EAY; EXEC(xycb,ARG());							} /* **   FD CB xx	  */
OP(fd,cc) { illegal_1(); op_cc();									} /* DB   FD		  */
OP(fd,cd) { illegal_1(); op_cd();									} /* DB   FD		  */
OP(fd,ce) { illegal_1(); op_ce();									} /* DB   FD		  */
OP(fd,cf) { illegal_1(); op_cf();									} /* DB   FD		  */

OP(fd,d0) { illegal_1(); op_d0();									} /* DB   FD		  */
OP(fd,d1) { illegal_1(); op_d1();									} /* DB   FD		  */
OP(fd,d2) { illegal_1(); op_d2();									} /* DB   FD		  */
OP(fd,d3) { illegal_1(); op_d3();									} /* DB   FD		  */
OP(fd,d4) { illegal_1(); op_d4();									} /* DB   FD		  */
OP(fd,d5) { illegal_1(); op_d5();									} /* DB   FD		  */
OP(fd,d6) { illegal_1(); op_d6();									} /* DB   FD		  */
OP(fd,d7) { illegal_1(); op_d7();									} /* DB   FD		  */

OP(fd,d8) { illegal_1(); op_d8();									} /* DB   FD		  */
OP(fd,d9) { illegal_1(); op_d9();									} /* DB   FD		  */
OP(fd,da) { illegal_1(); op_da();									} /* DB   FD		  */
OP(fd,db) { illegal_1(); op_db();									} /* DB   FD		  */
OP(fd,dc) { illegal_1(); op_dc();									} /* DB   FD		  */
OP(fd,dd) { illegal_1(); op_dd();									} /* DB   FD		  */
OP(fd,de) { illegal_1(); op_de();									} /* DB   FD		  */
OP(fd,df) { illegal_1(); op_df();									} /* DB   FD		  */

OP(fd,e0) { illegal_1(); op_e0();									} /* DB   FD		  */
OP(fd,e1) { _Z80_R++; POP(IY);											} /* POP  IY		  */
OP(fd,e2) { illegal_1(); op_e2();									} /* DB   FD		  */
OP(fd,e3) { _Z80_R++; EXSP(IY);											} /* EX   (SP),IY	  */
OP(fd,e4) { illegal_1(); op_e4();									} /* DB   FD		  */
OP(fd,e5) { _Z80_R++; PUSH( IY );										} /* PUSH IY		  */
OP(fd,e6) { illegal_1(); op_e6();									} /* DB   FD		  */
OP(fd,e7) { illegal_1(); op_e7();									} /* DB   FD		  */

OP(fd,e8) { illegal_1(); op_e8();									} /* DB   FD		  */
OP(fd,e9) { _Z80_R++; _PC = _IY; change_pc16(_PCD);						} /* JP   (IY)		  */
OP(fd,ea) { illegal_1(); op_ea();									} /* DB   FD		  */
OP(fd,eb) { illegal_1(); op_eb();									} /* DB   FD		  */
OP(fd,ec) { illegal_1(); op_ec();									} /* DB   FD		  */
OP(fd,ed) { illegal_1(); op_ed();									} /* DB   FD		  */
OP(fd,ee) { illegal_1(); op_ee();									} /* DB   FD		  */
OP(fd,ef) { illegal_1(); op_ef();									} /* DB   FD		  */

OP(fd,f0) { illegal_1(); op_f0();									} /* DB   FD		  */
OP(fd,f1) { illegal_1(); op_f1();									} /* DB   FD		  */
OP(fd,f2) { illegal_1(); op_f2();									} /* DB   FD		  */
OP(fd,f3) { illegal_1(); op_f3();									} /* DB   FD		  */
OP(fd,f4) { illegal_1(); op_f4();									} /* DB   FD		  */
OP(fd,f5) { illegal_1(); op_f5();									} /* DB   FD		  */
OP(fd,f6) { illegal_1(); op_f6();									} /* DB   FD		  */
OP(fd,f7) { illegal_1(); op_f7();									} /* DB   FD		  */

OP(fd,f8) { illegal_1(); op_f8();									} /* DB   FD		  */
OP(fd,f9) { _Z80_R++; _SP = _IY;										} /* LD   SP,IY		  */
OP(fd,fa) { illegal_1(); op_fa();									} /* DB   FD		  */
OP(fd,fb) { illegal_1(); op_fb();									} /* DB   FD		  */
OP(fd,fc) { illegal_1(); op_fc();									} /* DB   FD		  */
OP(fd,fd) { illegal_1(); op_fd();									} /* DB   FD		  */
OP(fd,fe) { illegal_1(); op_fe();									} /* DB   FD		  */
OP(fd,ff) { illegal_1(); op_ff();									} /* DB   FD		  */

OP(illegal,2)
{
	logerror("Z80 #%d ill. opcode $ed $%02x\n",
			cpu_getactivecpu(), cpu_readop((_PCD-1)&0xffff));
}

/**********************************************************
 * special opcodes (ED prefix)
 **********************************************************/
OP(ed,00) { illegal_2();											} /* DB   ED		  */
OP(ed,01) { illegal_2();											} /* DB   ED		  */
OP(ed,02) { illegal_2();											} /* DB   ED		  */
OP(ed,03) { illegal_2();											} /* DB   ED		  */
OP(ed,04) { illegal_2();											} /* DB   ED		  */
OP(ed,05) { illegal_2();											} /* DB   ED		  */
OP(ed,06) { illegal_2();											} /* DB   ED		  */
OP(ed,07) { illegal_2();											} /* DB   ED		  */

OP(ed,08) { illegal_2();											} /* DB   ED		  */
OP(ed,09) { illegal_2();											} /* DB   ED		  */
OP(ed,0a) { illegal_2();											} /* DB   ED		  */
OP(ed,0b) { illegal_2();											} /* DB   ED		  */
OP(ed,0c) { illegal_2();											} /* DB   ED		  */
OP(ed,0d) { illegal_2();											} /* DB   ED		  */
OP(ed,0e) { illegal_2();											} /* DB   ED		  */
OP(ed,0f) { illegal_2();											} /* DB   ED		  */

OP(ed,10) { illegal_2();											} /* DB   ED		  */
OP(ed,11) { illegal_2();											} /* DB   ED		  */
OP(ed,12) { illegal_2();											} /* DB   ED		  */
OP(ed,13) { illegal_2();											} /* DB   ED		  */
OP(ed,14) { illegal_2();											} /* DB   ED		  */
OP(ed,15) { illegal_2();											} /* DB   ED		  */
OP(ed,16) { illegal_2();											} /* DB   ED		  */
OP(ed,17) { illegal_2();											} /* DB   ED		  */

OP(ed,18) { illegal_2();											} /* DB   ED		  */
OP(ed,19) { illegal_2();											} /* DB   ED		  */
OP(ed,1a) { illegal_2();											} /* DB   ED		  */
OP(ed,1b) { illegal_2();											} /* DB   ED		  */
OP(ed,1c) { illegal_2();											} /* DB   ED		  */
OP(ed,1d) { illegal_2();											} /* DB   ED		  */
OP(ed,1e) { illegal_2();											} /* DB   ED		  */
OP(ed,1f) { illegal_2();											} /* DB   ED		  */

OP(ed,20) { illegal_2();											} /* DB   ED		  */
OP(ed,21) { illegal_2();											} /* DB   ED		  */
OP(ed,22) { illegal_2();											} /* DB   ED		  */
OP(ed,23) { illegal_2();											} /* DB   ED		  */
OP(ed,24) { illegal_2();											} /* DB   ED		  */
OP(ed,25) { illegal_2();											} /* DB   ED		  */
OP(ed,26) { illegal_2();											} /* DB   ED		  */
OP(ed,27) { illegal_2();											} /* DB   ED		  */

OP(ed,28) { illegal_2();											} /* DB   ED		  */
OP(ed,29) { illegal_2();											} /* DB   ED		  */
OP(ed,2a) { illegal_2();											} /* DB   ED		  */
OP(ed,2b) { illegal_2();											} /* DB   ED		  */
OP(ed,2c) { illegal_2();											} /* DB   ED		  */
OP(ed,2d) { illegal_2();											} /* DB   ED		  */
OP(ed,2e) { illegal_2();											} /* DB   ED		  */
OP(ed,2f) { illegal_2();											} /* DB   ED		  */

OP(ed,30) { illegal_2();											} /* DB   ED		  */
OP(ed,31) { illegal_2();											} /* DB   ED		  */
OP(ed,32) { illegal_2();											} /* DB   ED		  */
OP(ed,33) { illegal_2();											} /* DB   ED		  */
OP(ed,34) { illegal_2();											} /* DB   ED		  */
OP(ed,35) { illegal_2();											} /* DB   ED		  */
OP(ed,36) { illegal_2();											} /* DB   ED		  */
OP(ed,37) { illegal_2();											} /* DB   ED		  */

OP(ed,38) { illegal_2();											} /* DB   ED		  */
OP(ed,39) { illegal_2();											} /* DB   ED		  */
OP(ed,3a) { illegal_2();											} /* DB   ED		  */
OP(ed,3b) { illegal_2();											} /* DB   ED		  */
OP(ed,3c) { illegal_2();											} /* DB   ED		  */
OP(ed,3d) { illegal_2();											} /* DB   ED		  */
OP(ed,3e) { illegal_2();											} /* DB   ED		  */
OP(ed,3f) { illegal_2();											} /* DB   ED		  */

OP(ed,40) { _Z80_B = IN(_BC); _F = (_F & CF) | SZP[_Z80_B];					} /* IN   B,(C)		  */
OP(ed,41) { OUT(_BC,_Z80_B);											} /* OUT  (C),B		  */
OP(ed,42) { SBC16( BC );											} /* SBC  HL,BC		  */
OP(ed,43) { EA = ARG16(); WM16( EA, &Z80.BC );						} /* LD   (w),BC	  */
OP(ed,44) { NEG;													} /* NEG			  */
OP(ed,45) { RETN;													} /* RETN;			  */
OP(ed,46) { _IM = 0;												} /* IM   0			  */
OP(ed,47) { LD_I_A;													} /* LD   I,A		  */

OP(ed,48) { _Z80_C = IN(_BC); _F = (_F & CF) | SZP[_Z80_C];					} /* IN   C,(C)		  */
OP(ed,49) { OUT(_BC,_Z80_C);											} /* OUT  (C),C		  */
OP(ed,4a) { ADC16( BC );											} /* ADC  HL,BC		  */
OP(ed,4b) { EA = ARG16(); RM16( EA, &Z80.BC );						} /* LD   BC,(w)	  */
OP(ed,4c) { NEG;													} /* NEG			  */
OP(ed,4d) { RETI;													} /* RETI			  */
OP(ed,4e) { _IM = 0;												} /* IM   0			  */
OP(ed,4f) { LD_R_A;													} /* LD   R,A		  */

OP(ed,50) { _Z80_D = IN(_BC); _F = (_F & CF) | SZP[_Z80_D];					} /* IN   D,(C)		  */
OP(ed,51) { OUT(_BC,_Z80_D);											} /* OUT  (C),D		  */
OP(ed,52) { SBC16( DE );											} /* SBC  HL,DE		  */
OP(ed,53) { EA = ARG16(); WM16( EA, &Z80.DE );						} /* LD   (w),DE	  */
OP(ed,54) { NEG;													} /* NEG			  */
OP(ed,55) { RETN;													} /* RETN;			  */
OP(ed,56) { _IM = 1;												} /* IM   1			  */
OP(ed,57) { LD_A_I;													} /* LD   A,I		  */

OP(ed,58) { _Z80_E = IN(_BC); _F = (_F & CF) | SZP[_Z80_E];					} /* IN   E,(C)		  */
OP(ed,59) { OUT(_BC,_Z80_E);											} /* OUT  (C),E		  */
OP(ed,5a) { ADC16( DE );											} /* ADC  HL,DE		  */
OP(ed,5b) { EA = ARG16(); RM16( EA, &Z80.DE );						} /* LD   DE,(w)	  */
OP(ed,5c) { NEG;													} /* NEG			  */
OP(ed,5d) { RETI;													} /* RETI			  */
OP(ed,5e) { _IM = 2;												} /* IM   2			  */
OP(ed,5f) { LD_A_R;													} /* LD   A,R		  */

OP(ed,60) { _Z80_H = IN(_BC); _F = (_F & CF) | SZP[_Z80_H];					} /* IN   H,(C)		  */
OP(ed,61) { OUT(_BC,_Z80_H);											} /* OUT  (C),H		  */
OP(ed,62) { SBC16( HL );											} /* SBC  HL,HL		  */
OP(ed,63) { EA = ARG16(); WM16( EA, &Z80.HL );						} /* LD   (w),HL	  */
OP(ed,64) { NEG;													} /* NEG			  */
OP(ed,65) { RETN;													} /* RETN;			  */
OP(ed,66) { _IM = 0;												} /* IM   0			  */
OP(ed,67) { RRD;													} /* RRD  (HL)		  */

OP(ed,68) { _Z80_L = IN(_BC); _F = (_F & CF) | SZP[_Z80_L];					} /* IN   L,(C)		  */
OP(ed,69) { OUT(_BC,_Z80_L);											} /* OUT  (C),L		  */
OP(ed,6a) { ADC16( HL );											} /* ADC  HL,HL		  */
OP(ed,6b) { EA = ARG16(); RM16( EA, &Z80.HL );						} /* LD   HL,(w)	  */
OP(ed,6c) { NEG;													} /* NEG			  */
OP(ed,6d) { RETI;													} /* RETI			  */
OP(ed,6e) { _IM = 0;												} /* IM   0			  */
OP(ed,6f) { RLD;													} /* RLD  (HL)		  */

OP(ed,70) { UINT8 res = IN(_BC); _F = (_F & CF) | SZP[res];			} /* IN   0,(C)		  */
OP(ed,71) { OUT(_BC,0);												} /* OUT  (C),0		  */
OP(ed,72) { SBC16( SP );											} /* SBC  HL,SP		  */
OP(ed,73) { EA = ARG16(); WM16( EA, &Z80.SP );						} /* LD   (w),SP	  */
OP(ed,74) { NEG;													} /* NEG			  */
OP(ed,75) { RETN;													} /* RETN;			  */
OP(ed,76) { _IM = 1;												} /* IM   1			  */
OP(ed,77) { illegal_2();											} /* DB   ED,77		  */

OP(ed,78) { _Z80_A = IN(_BC); _F = (_F & CF) | SZP[_Z80_A];					} /* IN   E,(C)		  */
OP(ed,79) { OUT(_BC,_Z80_A);											} /* OUT  (C),E		  */
OP(ed,7a) { ADC16( SP );											} /* ADC  HL,SP		  */
OP(ed,7b) { EA = ARG16(); RM16( EA, &Z80.SP );						} /* LD   SP,(w)	  */
OP(ed,7c) { NEG;													} /* NEG			  */
OP(ed,7d) { RETI;													} /* RETI			  */
OP(ed,7e) { _IM = 2;												} /* IM   2			  */
OP(ed,7f) { illegal_2();											} /* DB   ED,7F		  */

OP(ed,80) { illegal_2();											} /* DB   ED		  */
OP(ed,81) { illegal_2();											} /* DB   ED		  */
OP(ed,82) { illegal_2();											} /* DB   ED		  */
OP(ed,83) { illegal_2();											} /* DB   ED		  */
OP(ed,84) { illegal_2();											} /* DB   ED		  */
OP(ed,85) { illegal_2();											} /* DB   ED		  */
OP(ed,86) { illegal_2();											} /* DB   ED		  */
OP(ed,87) { illegal_2();											} /* DB   ED		  */

OP(ed,88) { illegal_2();											} /* DB   ED		  */
OP(ed,89) { illegal_2();											} /* DB   ED		  */
OP(ed,8a) { illegal_2();											} /* DB   ED		  */
OP(ed,8b) { illegal_2();											} /* DB   ED		  */
OP(ed,8c) { illegal_2();											} /* DB   ED		  */
OP(ed,8d) { illegal_2();											} /* DB   ED		  */
OP(ed,8e) { illegal_2();											} /* DB   ED		  */
OP(ed,8f) { illegal_2();											} /* DB   ED		  */

OP(ed,90) { illegal_2();											} /* DB   ED		  */
OP(ed,91) { illegal_2();											} /* DB   ED		  */
OP(ed,92) { illegal_2();											} /* DB   ED		  */
OP(ed,93) { illegal_2();											} /* DB   ED		  */
OP(ed,94) { illegal_2();											} /* DB   ED		  */
OP(ed,95) { illegal_2();											} /* DB   ED		  */
OP(ed,96) { illegal_2();											} /* DB   ED		  */
OP(ed,97) { illegal_2();											} /* DB   ED		  */

OP(ed,98) { illegal_2();											} /* DB   ED		  */
OP(ed,99) { illegal_2();											} /* DB   ED		  */
OP(ed,9a) { illegal_2();											} /* DB   ED		  */
OP(ed,9b) { illegal_2();											} /* DB   ED		  */
OP(ed,9c) { illegal_2();											} /* DB   ED		  */
OP(ed,9d) { illegal_2();											} /* DB   ED		  */
OP(ed,9e) { illegal_2();											} /* DB   ED		  */
OP(ed,9f) { illegal_2();											} /* DB   ED		  */

OP(ed,a0) { LDI;													} /* LDI			  */
OP(ed,a1) { CPI;													} /* CPI			  */
OP(ed,a2) { INI;													} /* INI			  */
OP(ed,a3) { OUTI;													} /* OUTI			  */
OP(ed,a4) { illegal_2();											} /* DB   ED		  */
OP(ed,a5) { illegal_2();											} /* DB   ED		  */
OP(ed,a6) { illegal_2();											} /* DB   ED		  */
OP(ed,a7) { illegal_2();											} /* DB   ED		  */

OP(ed,a8) { LDD;													} /* LDD			  */
OP(ed,a9) { CPD;													} /* CPD			  */
OP(ed,aa) { IND;													} /* IND			  */
OP(ed,ab) { OUTD;													} /* OUTD			  */
OP(ed,ac) { illegal_2();											} /* DB   ED		  */
OP(ed,ad) { illegal_2();											} /* DB   ED		  */
OP(ed,ae) { illegal_2();											} /* DB   ED		  */
OP(ed,af) { illegal_2();											} /* DB   ED		  */

OP(ed,b0) { LDIR;													} /* LDIR			  */
OP(ed,b1) { CPIR;													} /* CPIR			  */
OP(ed,b2) { INIR;													} /* INIR			  */
OP(ed,b3) { OTIR;													} /* OTIR			  */
OP(ed,b4) { illegal_2();											} /* DB   ED		  */
OP(ed,b5) { illegal_2();											} /* DB   ED		  */
OP(ed,b6) { illegal_2();											} /* DB   ED		  */
OP(ed,b7) { illegal_2();											} /* DB   ED		  */

OP(ed,b8) { LDDR;													} /* LDDR			  */
OP(ed,b9) { CPDR;													} /* CPDR			  */
OP(ed,ba) { INDR;													} /* INDR			  */
OP(ed,bb) { OTDR;													} /* OTDR			  */
OP(ed,bc) { illegal_2();											} /* DB   ED		  */
OP(ed,bd) { illegal_2();											} /* DB   ED		  */
OP(ed,be) { illegal_2();											} /* DB   ED		  */
OP(ed,bf) { illegal_2();											} /* DB   ED		  */

OP(ed,c0) { illegal_2();											} /* DB   ED		  */
OP(ed,c1) { illegal_2();											} /* DB   ED		  */
OP(ed,c2) { illegal_2();											} /* DB   ED		  */
OP(ed,c3) { illegal_2();											} /* DB   ED		  */
OP(ed,c4) { illegal_2();											} /* DB   ED		  */
OP(ed,c5) { illegal_2();											} /* DB   ED		  */
OP(ed,c6) { illegal_2();											} /* DB   ED		  */
OP(ed,c7) { illegal_2();											} /* DB   ED		  */

OP(ed,c8) { illegal_2();											} /* DB   ED		  */
OP(ed,c9) { illegal_2();											} /* DB   ED		  */
OP(ed,ca) { illegal_2();											} /* DB   ED		  */
OP(ed,cb) { illegal_2();											} /* DB   ED		  */
OP(ed,cc) { illegal_2();											} /* DB   ED		  */
OP(ed,cd) { illegal_2();											} /* DB   ED		  */
OP(ed,ce) { illegal_2();											} /* DB   ED		  */
OP(ed,cf) { illegal_2();											} /* DB   ED		  */

OP(ed,d0) { illegal_2();											} /* DB   ED		  */
OP(ed,d1) { illegal_2();											} /* DB   ED		  */
OP(ed,d2) { illegal_2();											} /* DB   ED		  */
OP(ed,d3) { illegal_2();											} /* DB   ED		  */
OP(ed,d4) { illegal_2();											} /* DB   ED		  */
OP(ed,d5) { illegal_2();											} /* DB   ED		  */
OP(ed,d6) { illegal_2();											} /* DB   ED		  */
OP(ed,d7) { illegal_2();											} /* DB   ED		  */

OP(ed,d8) { illegal_2();											} /* DB   ED		  */
OP(ed,d9) { illegal_2();											} /* DB   ED		  */
OP(ed,da) { illegal_2();											} /* DB   ED		  */
OP(ed,db) { illegal_2();											} /* DB   ED		  */
OP(ed,dc) { illegal_2();											} /* DB   ED		  */
OP(ed,dd) { illegal_2();											} /* DB   ED		  */
OP(ed,de) { illegal_2();											} /* DB   ED		  */
OP(ed,df) { illegal_2();											} /* DB   ED		  */

OP(ed,e0) { illegal_2();											} /* DB   ED		  */
OP(ed,e1) { illegal_2();											} /* DB   ED		  */
OP(ed,e2) { illegal_2();											} /* DB   ED		  */
OP(ed,e3) { illegal_2();											} /* DB   ED		  */
OP(ed,e4) { illegal_2();											} /* DB   ED		  */
OP(ed,e5) { illegal_2();											} /* DB   ED		  */
OP(ed,e6) { illegal_2();											} /* DB   ED		  */
OP(ed,e7) { illegal_2();											} /* DB   ED		  */

OP(ed,e8) { illegal_2();											} /* DB   ED		  */
OP(ed,e9) { illegal_2();											} /* DB   ED		  */
OP(ed,ea) { illegal_2();											} /* DB   ED		  */
OP(ed,eb) { illegal_2();											} /* DB   ED		  */
OP(ed,ec) { illegal_2();											} /* DB   ED		  */
OP(ed,ed) { illegal_2();											} /* DB   ED		  */
OP(ed,ee) { illegal_2();											} /* DB   ED		  */
OP(ed,ef) { illegal_2();											} /* DB   ED		  */

OP(ed,f0) { illegal_2();											} /* DB   ED		  */
OP(ed,f1) { illegal_2();											} /* DB   ED		  */
OP(ed,f2) { illegal_2();											} /* DB   ED		  */
OP(ed,f3) { illegal_2();											} /* DB   ED		  */
OP(ed,f4) { illegal_2();											} /* DB   ED		  */
OP(ed,f5) { illegal_2();											} /* DB   ED		  */
OP(ed,f6) { illegal_2();											} /* DB   ED		  */
OP(ed,f7) { illegal_2();											} /* DB   ED		  */

OP(ed,f8) { illegal_2();											} /* DB   ED		  */
OP(ed,f9) { illegal_2();											} /* DB   ED		  */
OP(ed,fa) { illegal_2();											} /* DB   ED		  */
OP(ed,fb) { illegal_2();											} /* DB   ED		  */
OP(ed,fc) { illegal_2();											} /* DB   ED		  */
OP(ed,fd) { illegal_2();											} /* DB   ED		  */
OP(ed,fe) { illegal_2();											} /* DB   ED		  */
OP(ed,ff) { illegal_2();											} /* DB   ED		  */

#if TIME_LOOP_HACKS

#define CHECK_BC_LOOP												\
if( _BC > 1 && _PCD < 0xfffc ) {									\
	UINT8 op1 = cpu_readop(_PCD);									\
	UINT8 op2 = cpu_readop(_PCD+1);									\
	if( (op1==0x78 && op2==0xb1) || (op1==0x79 && op2==0xb0) )		\
	{																\
		UINT8 op3 = cpu_readop(_PCD+2);								\
		UINT8 op4 = cpu_readop(_PCD+3);								\
		if( op3==0x20 && op4==0xfb )								\
		{															\
			int cnt =												\
				cc[Z80_TABLE_op][0x78] +							\
				cc[Z80_TABLE_op][0xb1] +							\
				cc[Z80_TABLE_op][0x20] +							\
				cc[Z80_TABLE_ex][0x20];								\
			while( _BC > 0 && z80_ICount > cnt )					\
			{														\
				BURNODD( cnt, 4, cnt );								\
				_BC--;												\
			}														\
		}															\
		else														\
		if( op3 == 0xc2 )											\
		{															\
			UINT8 ad1 = cpu_readop_arg(_PCD+3);						\
			UINT8 ad2 = cpu_readop_arg(_PCD+4);						\
			if( (ad1 + 256 * ad2) == (_PCD - 1) )					\
			{														\
				int cnt =											\
					cc[Z80_TABLE_op][0x78] +						\
					cc[Z80_TABLE_op][0xb1] +						\
					cc[Z80_TABLE_op][0xc2] +						\
					cc[Z80_TABLE_ex][0xc2];							\
				while( _BC > 0 && z80_ICount > cnt )				\
				{													\
					BURNODD( cnt, 4, cnt );							\
					_BC--;											\
				}													\
			}														\
		}															\
	}																\
}

#define CHECK_DE_LOOP												\
if( _DE > 1 && _PCD < 0xfffc ) {									\
	UINT8 op1 = cpu_readop(_PCD);									\
	UINT8 op2 = cpu_readop(_PCD+1);									\
	if( (op1==0x7a && op2==0xb3) || (op1==0x7b && op2==0xb2) )		\
	{																\
		UINT8 op3 = cpu_readop(_PCD+2);								\
		UINT8 op4 = cpu_readop(_PCD+3);								\
		if( op3==0x20 && op4==0xfb )								\
		{															\
			int cnt =												\
				cc[Z80_TABLE_op][0x7a] +							\
				cc[Z80_TABLE_op][0xb3] +							\
				cc[Z80_TABLE_op][0x20] +							\
				cc[Z80_TABLE_ex][0x20];								\
			while( _DE > 0 && z80_ICount > cnt )					\
			{														\
				BURNODD( cnt, 4, cnt );								\
				_DE--;												\
			}														\
		}															\
		else														\
		if( op3==0xc2 )												\
		{															\
			UINT8 ad1 = cpu_readop_arg(_PCD+3);						\
			UINT8 ad2 = cpu_readop_arg(_PCD+4);						\
			if( (ad1 + 256 * ad2) == (_PCD - 1) )					\
			{														\
				int cnt =											\
					cc[Z80_TABLE_op][0x7a] +						\
					cc[Z80_TABLE_op][0xb3] +						\
					cc[Z80_TABLE_op][0xc2] +						\
					cc[Z80_TABLE_ex][0xc2];							\
				while( _DE > 0 && z80_ICount > cnt )				\
				{													\
					BURNODD( cnt, 4, cnt );							\
					_DE--;											\
				}													\
			}														\
		}															\
	}																\
}

#define CHECK_HL_LOOP												\
if( _HL > 1 && _PCD < 0xfffc ) {									\
	UINT8 op1 = cpu_readop(_PCD);									\
	UINT8 op2 = cpu_readop(_PCD+1);									\
	if( (op1==0x7c && op2==0xb5) || (op1==0x7d && op2==0xb4) )		\
	{																\
		UINT8 op3 = cpu_readop(_PCD+2);								\
		UINT8 op4 = cpu_readop(_PCD+3);								\
		if( op3==0x20 && op4==0xfb )								\
		{															\
			int cnt =												\
				cc[Z80_TABLE_op][0x7c] +							\
				cc[Z80_TABLE_op][0xb5] +							\
				cc[Z80_TABLE_op][0x20] +							\
				cc[Z80_TABLE_ex][0x20];								\
			while( _HL > 0 && z80_ICount > cnt )					\
			{														\
				BURNODD( cnt, 4, cnt );								\
				_HL--;												\
			}														\
		}															\
		else														\
		if( op3==0xc2 )												\
		{															\
			UINT8 ad1 = cpu_readop_arg(_PCD+3);						\
			UINT8 ad2 = cpu_readop_arg(_PCD+4);						\
			if( (ad1 + 256 * ad2) == (_PCD - 1) )					\
			{														\
				int cnt =											\
					cc[Z80_TABLE_op][0x7c] +						\
					cc[Z80_TABLE_op][0xb5] +						\
					cc[Z80_TABLE_op][0xc2] +						\
					cc[Z80_TABLE_ex][0xc2];							\
				while( _HL > 0 && z80_ICount > cnt )				\
				{													\
					BURNODD( cnt, 4, cnt );							\
					_HL--;											\
				}													\
			}														\
		}															\
	}																\
}

#else

#define CHECK_BC_LOOP
#define CHECK_DE_LOOP
#define CHECK_HL_LOOP

#endif

/**********************************************************
 * main opcodes
 **********************************************************/
OP(op,00) {															} /* NOP			  */
OP(op,01) { _BC = ARG16();											} /* LD   BC,w		  */
OP(op,02) { WM( _BC, _Z80_A );											} /* LD   (BC),A	  */
OP(op,03) { _BC++;													} /* INC  BC		  */
OP(op,04) { _Z80_B = INC(_Z80_B);											} /* INC  B			  */
OP(op,05) { _Z80_B = DEC(_Z80_B);											} /* DEC  B			  */
OP(op,06) { _Z80_B = ARG();												} /* LD   B,n		  */
OP(op,07) { RLCA;													} /* RLCA			  */

OP(op,08) { EX_AF;													} /* EX   AF,AF'	  */
OP(op,09) { ADD16(HL,BC);											} /* ADD  HL,BC		  */
OP(op,0a) { _Z80_A = RM(_BC);											} /* LD   A,(BC)	  */
OP(op,0b) { _BC--; CHECK_BC_LOOP;									} /* DEC  BC		  */
OP(op,0c) { _Z80_C = INC(_Z80_C);											} /* INC  C			  */
OP(op,0d) { _Z80_C = DEC(_Z80_C);											} /* DEC  C			  */
OP(op,0e) { _Z80_C = ARG();												} /* LD   C,n		  */
OP(op,0f) { RRCA;													} /* RRCA			  */

OP(op,10) { _Z80_B--; JR_COND( _Z80_B, 0x10 );								} /* DJNZ o			  */
OP(op,11) { _DE = ARG16();											} /* LD   DE,w		  */
OP(op,12) { WM( _DE, _Z80_A );											} /* LD   (DE),A	  */
OP(op,13) { _DE++;													} /* INC  DE		  */
OP(op,14) { _Z80_D = INC(_Z80_D);											} /* INC  D			  */
OP(op,15) { _Z80_D = DEC(_Z80_D);											} /* DEC  D			  */
OP(op,16) { _Z80_D = ARG();												} /* LD   D,n		  */
OP(op,17) { RLA;													} /* RLA			  */

OP(op,18) { JR();													} /* JR   o			  */
OP(op,19) { ADD16(HL,DE);											} /* ADD  HL,DE		  */
OP(op,1a) { _Z80_A = RM(_DE);											} /* LD   A,(DE)	  */
OP(op,1b) { _DE--; CHECK_DE_LOOP;									} /* DEC  DE		  */
OP(op,1c) { _Z80_E = INC(_Z80_E);											} /* INC  E			  */
OP(op,1d) { _Z80_E = DEC(_Z80_E);											} /* DEC  E			  */
OP(op,1e) { _Z80_E = ARG();												} /* LD   E,n		  */
OP(op,1f) { RRA;													} /* RRA			  */

OP(op,20) { JR_COND( !(_F & ZF), 0x20 );							} /* JR   NZ,o		  */
OP(op,21) { _HL = ARG16();											} /* LD   HL,w		  */
OP(op,22) { EA = ARG16(); WM16( EA, &Z80.HL );						} /* LD   (w),HL	  */
OP(op,23) { _HL++;													} /* INC  HL		  */
OP(op,24) { _Z80_H = INC(_Z80_H);											} /* INC  H			  */
OP(op,25) { _Z80_H = DEC(_Z80_H);											} /* DEC  H			  */
OP(op,26) { _Z80_H = ARG();												} /* LD   H,n		  */
OP(op,27) { DAA;													} /* DAA			  */

OP(op,28) { JR_COND( _F & ZF, 0x28 );								} /* JR   Z,o		  */
OP(op,29) { ADD16(HL,HL);											} /* ADD  HL,HL		  */
OP(op,2a) { EA = ARG16(); RM16( EA, &Z80.HL );						} /* LD   HL,(w)	  */
OP(op,2b) { _HL--; CHECK_HL_LOOP;									} /* DEC  HL		  */
OP(op,2c) { _Z80_L = INC(_Z80_L);											} /* INC  L			  */
OP(op,2d) { _Z80_L = DEC(_Z80_L);											} /* DEC  L			  */
OP(op,2e) { _Z80_L = ARG();												} /* LD   L,n		  */
OP(op,2f) { _Z80_A ^= 0xff; _F = (_F&(SF|ZF|PF|CF))|HF|NF|(_Z80_A&(YF|XF)); } /* CPL			  */

OP(op,30) { JR_COND( !(_F & CF), 0x30 );							} /* JR   NC,o		  */
OP(op,31) { _SP = ARG16();											} /* LD   SP,w		  */
OP(op,32) { EA = ARG16(); WM( EA, _Z80_A );								} /* LD   (w),A		  */
OP(op,33) { _SP++;													} /* INC  SP		  */
OP(op,34) { WM( _HL, INC(RM(_HL)) );								} /* INC  (HL)		  */
OP(op,35) { WM( _HL, DEC(RM(_HL)) );								} /* DEC  (HL)		  */
OP(op,36) { WM( _HL, ARG() );										} /* LD   (HL),n	  */
OP(op,37) { _F = (_F & (SF|ZF|PF)) | CF | (_Z80_A & (YF|XF));			} /* SCF			  */

OP(op,38) { JR_COND( _F & CF, 0x38 );								} /* JR   C,o		  */
OP(op,39) { ADD16(HL,SP);											} /* ADD  HL,SP		  */
OP(op,3a) { EA = ARG16(); _Z80_A = RM( EA );							} /* LD   A,(w)		  */
OP(op,3b) { _SP--;													} /* DEC  SP		  */
OP(op,3c) { _Z80_A = INC(_Z80_A);											} /* INC  A			  */
OP(op,3d) { _Z80_A = DEC(_Z80_A);											} /* DEC  A			  */
OP(op,3e) { _Z80_A = ARG();												} /* LD   A,n		  */
OP(op,3f) { _F = ((_F&(SF|ZF|PF|CF))|((_F&CF)<<4)|(_Z80_A&(YF|XF)))^CF; } /* CCF			  */
/*OP(op,3f) { _F = ((_F & ~(HF|NF)) | ((_F & CF)<<4)) ^ CF;			  }    CCF				   */

OP(op,40) {															} /* LD   B,B		  */
OP(op,41) { _Z80_B = _Z80_C;												} /* LD   B,C		  */
OP(op,42) { _Z80_B = _Z80_D;												} /* LD   B,D		  */
OP(op,43) { _Z80_B = _Z80_E;												} /* LD   B,E		  */
OP(op,44) { _Z80_B = _Z80_H;												} /* LD   B,H		  */
OP(op,45) { _Z80_B = _Z80_L;												} /* LD   B,L		  */
OP(op,46) { _Z80_B = RM(_HL);											} /* LD   B,(HL)	  */
OP(op,47) { _Z80_B = _Z80_A;												} /* LD   B,A		  */

OP(op,48) { _Z80_C = _Z80_B;												} /* LD   C,B		  */
OP(op,49) {															} /* LD   C,C		  */
OP(op,4a) { _Z80_C = _Z80_D;												} /* LD   C,D		  */
OP(op,4b) { _Z80_C = _Z80_E;												} /* LD   C,E		  */
OP(op,4c) { _Z80_C = _Z80_H;												} /* LD   C,H		  */
OP(op,4d) { _Z80_C = _Z80_L;												} /* LD   C,L		  */
OP(op,4e) { _Z80_C = RM(_HL);											} /* LD   C,(HL)	  */
OP(op,4f) { _Z80_C = _Z80_A;												} /* LD   C,A		  */

OP(op,50) { _Z80_D = _Z80_B;												} /* LD   D,B		  */
OP(op,51) { _Z80_D = _Z80_C;												} /* LD   D,C		  */
OP(op,52) {															} /* LD   D,D		  */
OP(op,53) { _Z80_D = _Z80_E;												} /* LD   D,E		  */
OP(op,54) { _Z80_D = _Z80_H;												} /* LD   D,H		  */
OP(op,55) { _Z80_D = _Z80_L;												} /* LD   D,L		  */
OP(op,56) { _Z80_D = RM(_HL);											} /* LD   D,(HL)	  */
OP(op,57) { _Z80_D = _Z80_A;												} /* LD   D,A		  */

OP(op,58) { _Z80_E = _Z80_B;												} /* LD   E,B		  */
OP(op,59) { _Z80_E = _Z80_C;												} /* LD   E,C		  */
OP(op,5a) { _Z80_E = _Z80_D;												} /* LD   E,D		  */
OP(op,5b) {															} /* LD   E,E		  */
OP(op,5c) { _Z80_E = _Z80_H;												} /* LD   E,H		  */
OP(op,5d) { _Z80_E = _Z80_L;												} /* LD   E,L		  */
OP(op,5e) { _Z80_E = RM(_HL);											} /* LD   E,(HL)	  */
OP(op,5f) { _Z80_E = _Z80_A;												} /* LD   E,A		  */

OP(op,60) { _Z80_H = _Z80_B;												} /* LD   H,B		  */
OP(op,61) { _Z80_H = _Z80_C;												} /* LD   H,C		  */
OP(op,62) { _Z80_H = _Z80_D;												} /* LD   H,D		  */
OP(op,63) { _Z80_H = _Z80_E;												} /* LD   H,E		  */
OP(op,64) {															} /* LD   H,H		  */
OP(op,65) { _Z80_H = _Z80_L;												} /* LD   H,L		  */
OP(op,66) { _Z80_H = RM(_HL);											} /* LD   H,(HL)	  */
OP(op,67) { _Z80_H = _Z80_A;												} /* LD   H,A		  */

OP(op,68) { _Z80_L = _Z80_B;												} /* LD   L,B		  */
OP(op,69) { _Z80_L = _Z80_C;												} /* LD   L,C		  */
OP(op,6a) { _Z80_L = _Z80_D;												} /* LD   L,D		  */
OP(op,6b) { _Z80_L = _Z80_E;												} /* LD   L,E		  */
OP(op,6c) { _Z80_L = _Z80_H;												} /* LD   L,H		  */
OP(op,6d) {															} /* LD   L,L		  */
OP(op,6e) { _Z80_L = RM(_HL);											} /* LD   L,(HL)	  */
OP(op,6f) { _Z80_L = _Z80_A;												} /* LD   L,A		  */

OP(op,70) { WM( _HL, _Z80_B );											} /* LD   (HL),B	  */
OP(op,71) { WM( _HL, _Z80_C );											} /* LD   (HL),C	  */
OP(op,72) { WM( _HL, _Z80_D );											} /* LD   (HL),D	  */
OP(op,73) { WM( _HL, _Z80_E );											} /* LD   (HL),E	  */
OP(op,74) { WM( _HL, _Z80_H );											} /* LD   (HL),H	  */
OP(op,75) { WM( _HL, _Z80_L );											} /* LD   (HL),L	  */
OP(op,76) { ENTER_HALT;												} /* HALT			  */
OP(op,77) { WM( _HL, _Z80_A );											} /* LD   (HL),A	  */

OP(op,78) { _Z80_A = _Z80_B;												} /* LD   A,B		  */
OP(op,79) { _Z80_A = _Z80_C;												} /* LD   A,C		  */
OP(op,7a) { _Z80_A = _Z80_D;												} /* LD   A,D		  */
OP(op,7b) { _Z80_A = _Z80_E;												} /* LD   A,E		  */
OP(op,7c) { _Z80_A = _Z80_H;												} /* LD   A,H		  */
OP(op,7d) { _Z80_A = _Z80_L;												} /* LD   A,L		  */
OP(op,7e) { _Z80_A = RM(_HL);											} /* LD   A,(HL)	  */
OP(op,7f) {															} /* LD   A,A		  */

OP(op,80) { ADD(_Z80_B);												} /* ADD  A,B		  */
OP(op,81) { ADD(_Z80_C);												} /* ADD  A,C		  */
OP(op,82) { ADD(_Z80_D);												} /* ADD  A,D		  */
OP(op,83) { ADD(_Z80_E);												} /* ADD  A,E		  */
OP(op,84) { ADD(_Z80_H);												} /* ADD  A,H		  */
OP(op,85) { ADD(_Z80_L);												} /* ADD  A,L		  */
OP(op,86) { ADD(RM(_HL));											} /* ADD  A,(HL)	  */
OP(op,87) { ADD(_Z80_A);												} /* ADD  A,A		  */

OP(op,88) { ADC(_Z80_B);												} /* ADC  A,B		  */
OP(op,89) { ADC(_Z80_C);												} /* ADC  A,C		  */
OP(op,8a) { ADC(_Z80_D);												} /* ADC  A,D		  */
OP(op,8b) { ADC(_Z80_E);												} /* ADC  A,E		  */
OP(op,8c) { ADC(_Z80_H);												} /* ADC  A,H		  */
OP(op,8d) { ADC(_Z80_L);												} /* ADC  A,L		  */
OP(op,8e) { ADC(RM(_HL));											} /* ADC  A,(HL)	  */
OP(op,8f) { ADC(_Z80_A);												} /* ADC  A,A		  */

OP(op,90) { SUB(_Z80_B);												} /* SUB  B			  */
OP(op,91) { SUB(_Z80_C);												} /* SUB  C			  */
OP(op,92) { SUB(_Z80_D);												} /* SUB  D			  */
OP(op,93) { SUB(_Z80_E);												} /* SUB  E			  */
OP(op,94) { SUB(_Z80_H);												} /* SUB  H			  */
OP(op,95) { SUB(_Z80_L);												} /* SUB  L			  */
OP(op,96) { SUB(RM(_HL));											} /* SUB  (HL)		  */
OP(op,97) { SUB(_Z80_A);												} /* SUB  A			  */

OP(op,98) { SBC(_Z80_B);												} /* SBC  A,B		  */
OP(op,99) { SBC(_Z80_C);												} /* SBC  A,C		  */
OP(op,9a) { SBC(_Z80_D);												} /* SBC  A,D		  */
OP(op,9b) { SBC(_Z80_E);												} /* SBC  A,E		  */
OP(op,9c) { SBC(_Z80_H);												} /* SBC  A,H		  */
OP(op,9d) { SBC(_Z80_L);												} /* SBC  A,L		  */
OP(op,9e) { SBC(RM(_HL));											} /* SBC  A,(HL)	  */
OP(op,9f) { SBC(_Z80_A);												} /* SBC  A,A		  */

OP(op,a0) { AND(_Z80_B);												} /* AND  B			  */
OP(op,a1) { AND(_Z80_C);												} /* AND  C			  */
OP(op,a2) { AND(_Z80_D);												} /* AND  D			  */
OP(op,a3) { AND(_Z80_E);												} /* AND  E			  */
OP(op,a4) { AND(_Z80_H);												} /* AND  H			  */
OP(op,a5) { AND(_Z80_L);												} /* AND  L			  */
OP(op,a6) { AND(RM(_HL));											} /* AND  (HL)		  */
OP(op,a7) { AND(_Z80_A);												} /* AND  A			  */

OP(op,a8) { XOR(_Z80_B);												} /* XOR  B			  */
OP(op,a9) { XOR(_Z80_C);												} /* XOR  C			  */
OP(op,aa) { XOR(_Z80_D);												} /* XOR  D			  */
OP(op,ab) { XOR(_Z80_E);												} /* XOR  E			  */
OP(op,ac) { XOR(_Z80_H);												} /* XOR  H			  */
OP(op,ad) { XOR(_Z80_L);												} /* XOR  L			  */
OP(op,ae) { XOR(RM(_HL));											} /* XOR  (HL)		  */
OP(op,af) { XOR(_Z80_A);												} /* XOR  A			  */

OP(op,b0) { OR(_Z80_B);													} /* OR   B			  */
OP(op,b1) { OR(_Z80_C);													} /* OR   C			  */
OP(op,b2) { OR(_Z80_D);													} /* OR   D			  */
OP(op,b3) { OR(_Z80_E);													} /* OR   E			  */
OP(op,b4) { OR(_Z80_H);													} /* OR   H			  */
OP(op,b5) { OR(_Z80_L);													} /* OR   L			  */
OP(op,b6) { OR(RM(_HL));											} /* OR   (HL)		  */
OP(op,b7) { OR(_Z80_A);													} /* OR   A			  */

OP(op,b8) { CP(_Z80_B);													} /* CP   B			  */
OP(op,b9) { CP(_Z80_C);													} /* CP   C			  */
OP(op,ba) { CP(_Z80_D);													} /* CP   D			  */
OP(op,bb) { CP(_Z80_E);													} /* CP   E			  */
OP(op,bc) { CP(_Z80_H);													} /* CP   H			  */
OP(op,bd) { CP(_Z80_L);													} /* CP   L			  */
OP(op,be) { CP(RM(_HL));											} /* CP   (HL)		  */
OP(op,bf) { CP(_Z80_A);													} /* CP   A			  */

OP(op,c0) { RET_COND( !(_F & ZF), 0xc0 );							} /* RET  NZ		  */
OP(op,c1) { POP(BC);												} /* POP  BC		  */
OP(op,c2) { JP_COND( !(_F & ZF) );									} /* JP   NZ,a		  */
OP(op,c3) { JP;														} /* JP   a			  */
OP(op,c4) { CALL_COND( !(_F & ZF), 0xc4 );							} /* CALL NZ,a		  */
OP(op,c5) { PUSH( BC );												} /* PUSH BC		  */
OP(op,c6) { ADD(ARG());												} /* ADD  A,n		  */
OP(op,c7) { RST(0x00);												} /* RST  0			  */

OP(op,c8) { RET_COND( _F & ZF, 0xc8 );								} /* RET  Z			  */
OP(op,c9) { POP(PC); change_pc16(_PCD);								} /* RET			  */
OP(op,ca) { JP_COND( _F & ZF );										} /* JP   Z,a		  */
OP(op,cb) { _Z80_R++; EXEC(cb,ROP());									} /* **** CB xx		  */
OP(op,cc) { CALL_COND( _F & ZF, 0xcc );								} /* CALL Z,a		  */
OP(op,cd) { CALL();													} /* CALL a			  */
OP(op,ce) { ADC(ARG());												} /* ADC  A,n		  */
OP(op,cf) { RST(0x08);												} /* RST  1			  */

OP(op,d0) { RET_COND( !(_F & CF), 0xd0 );							} /* RET  NC		  */
OP(op,d1) { POP(DE);												} /* POP  DE		  */
OP(op,d2) { JP_COND( !(_F & CF) );									} /* JP   NC,a		  */
OP(op,d3) { unsigned n = ARG() | (_Z80_A << 8); OUT( n, _Z80_A );			} /* OUT  (n),A		  */
OP(op,d4) { CALL_COND( !(_F & CF), 0xd4 );							} /* CALL NC,a		  */
OP(op,d5) { PUSH( DE );												} /* PUSH DE		  */
OP(op,d6) { SUB(ARG());												} /* SUB  n			  */
OP(op,d7) { RST(0x10);												} /* RST  2			  */

OP(op,d8) { RET_COND( _F & CF, 0xd8 );								} /* RET  C			  */
OP(op,d9) { EXX;													} /* EXX			  */
OP(op,da) { JP_COND( _F & CF );										} /* JP   C,a		  */
OP(op,db) { unsigned n = ARG() | (_Z80_A << 8); _Z80_A = IN( n );			} /* IN   A,(n)		  */
OP(op,dc) { CALL_COND( _F & CF, 0xdc );								} /* CALL C,a		  */
OP(op,dd) { _Z80_R++; EXEC(dd,ROP());									} /* **** DD xx		  */
OP(op,de) { SBC(ARG());												} /* SBC  A,n		  */
OP(op,df) { RST(0x18);												} /* RST  3			  */

OP(op,e0) { RET_COND( !(_F & PF), 0xe0 );							} /* RET  PO		  */
OP(op,e1) { POP(HL);												} /* POP  HL		  */
OP(op,e2) { JP_COND( !(_F & PF) );									} /* JP   PO,a		  */
OP(op,e3) { EXSP(HL);												} /* EX   HL,(SP)	  */
OP(op,e4) { CALL_COND( !(_F & PF), 0xe4 );							} /* CALL PO,a		  */
OP(op,e5) { PUSH( HL );												} /* PUSH HL		  */
OP(op,e6) { AND(ARG());												} /* AND  n			  */
OP(op,e7) { RST(0x20);												} /* RST  4			  */

OP(op,e8) { RET_COND( _F & PF, 0xe8 );								} /* RET  PE		  */
OP(op,e9) { _PC = _HL; change_pc16(_PCD);							} /* JP   (HL)		  */
OP(op,ea) { JP_COND( _F & PF );										} /* JP   PE,a		  */
OP(op,eb) { EX_DE_HL;												} /* EX   DE,HL		  */
OP(op,ec) { CALL_COND( _F & PF, 0xec );								} /* CALL PE,a		  */
OP(op,ed) { _Z80_R++; EXEC(ed,ROP());									} /* **** ED xx		  */
OP(op,ee) { XOR(ARG());												} /* XOR  n			  */
OP(op,ef) { RST(0x28);												} /* RST  5			  */

OP(op,f0) { RET_COND( !(_F & SF), 0xf0 );							} /* RET  P			  */
OP(op,f1) { POP(AF);												} /* POP  AF		  */
OP(op,f2) { JP_COND( !(_F & SF) );									} /* JP   P,a		  */
OP(op,f3) { _IFF1 = _IFF2 = 0;										} /* DI				  */
OP(op,f4) { CALL_COND( !(_F & SF), 0xf4 );							} /* CALL P,a		  */
OP(op,f5) { PUSH( AF );												} /* PUSH AF		  */
OP(op,f6) { OR(ARG());												} /* OR   n			  */
OP(op,f7) { RST(0x30);												} /* RST  6			  */

OP(op,f8) { RET_COND( _F & SF, 0xf8 );								} /* RET  M			  */
OP(op,f9) { _SP = _HL;												} /* LD   SP,HL		  */
OP(op,fa) { JP_COND(_F & SF);										} /* JP   M,a		  */
OP(op,fb) { EI;														} /* EI				  */
OP(op,fc) { CALL_COND( _F & SF, 0xfc );								} /* CALL M,a		  */
OP(op,fd) { _Z80_R++; EXEC(fd,ROP());									} /* **** FD xx		  */
OP(op,fe) { CP(ARG());												} /* CP   n			  */
OP(op,ff) { RST(0x38);												} /* RST  7			  */


static void take_interrupt(void)
{
	if( _IFF1 )
	{
		int irq_vector;

		/* there isn't a valid previous program counter */
		_PPC = -1;

		/* Check if processor was halted */
		LEAVE_HALT;

		if( Z80.irq_max )			/* daisy chain mode */
		{
			if( Z80.request_irq >= 0 )
			{
				/* Clear both interrupt flip flops */
				_IFF1 = _IFF2 = 0;
				irq_vector = Z80.irq[Z80.request_irq].interrupt_entry(Z80.irq[Z80.request_irq].irq_param);
				LOG(("Z80 #%d daisy chain irq_vector $%02x\n", cpu_getactivecpu(), irq_vector));
				Z80.request_irq = -1;
			} else return;
		}
		else
		{
			/* Clear both interrupt flip flops */
			_IFF1 = _IFF2 = 0;
			/* call back the cpu interface to retrieve the vector */
			irq_vector = (*Z80.irq_callback)(0);
			LOG(("Z80 #%d single int. irq_vector $%02x\n", cpu_getactivecpu(), irq_vector));
		}

		/* Interrupt mode 2. Call [Z80.I:databyte] */
		if( _IM == 2 )
		{
			irq_vector = (irq_vector & 0xff) | (_Z80_I << 8);
			PUSH( PC );
			RM16( irq_vector, &Z80.PC );
			LOG(("Z80 #%d IM2 [$%04x] = $%04x\n",cpu_getactivecpu() , irq_vector, _PCD));
			/* CALL opcode timing */
			Z80.extra_cycles += cc[Z80_TABLE_op][0xcd];
		}
		else
		/* Interrupt mode 1. RST 38h */
		if( _IM == 1 )
		{
			LOG(("Z80 #%d IM1 $0038\n",cpu_getactivecpu() ));
			PUSH( PC );
			_PCD = 0x0038;
			/* RST $38 + 'interrupt latency' cycles */
			Z80.extra_cycles += cc[Z80_TABLE_op][0xff] + cc[Z80_TABLE_ex][0xff];
		}
		else
		{
			/* Interrupt mode 0. We check for CALL and JP instructions, */
			/* if neither of these were found we assume a 1 byte opcode */
			/* was placed on the databus								*/
			LOG(("Z80 #%d IM0 $%04x\n",cpu_getactivecpu() , irq_vector));
			switch (irq_vector & 0xff0000)
			{
				case 0xcd0000:	/* call */
					PUSH( PC );
					_PCD = irq_vector & 0xffff;
					 /* CALL $xxxx + 'interrupt latency' cycles */
					Z80.extra_cycles += cc[Z80_TABLE_op][0xcd] + cc[Z80_TABLE_ex][0xff];
					break;
				case 0xc30000:	/* jump */
					_PCD = irq_vector & 0xffff;
					/* JP $xxxx + 2 cycles */
					Z80.extra_cycles += cc[Z80_TABLE_op][0xc3] + cc[Z80_TABLE_ex][0xff];
					break;
				default:		/* rst (or other opcodes?) */
					PUSH( PC );
					_PCD = irq_vector & 0x0038;
					/* RST $xx + 2 cycles */
					Z80.extra_cycles += cc[Z80_TABLE_op][_PCD] + cc[Z80_TABLE_ex][_PCD];
					break;
			}
		}
		change_pc16(_PCD);
	}
}

/****************************************************************************
 * Processor initialization
 ****************************************************************************/
void z80_init(void)
{
	int cpu = 0;//cpu_getactivecpu();
	int i, p;
#if BIG_FLAGS_ARRAY
	if( !SZHVC_add || !SZHVC_sub )
	{
		int oldval, newval, val;
		UINT8 *padd, *padc, *psub, *psbc;
		/* allocate big flag arrays once */
		SZHVC_add = (UINT8 *)malloc(2*256*256);
		SZHVC_sub = (UINT8 *)malloc(2*256*256);
		if( !SZHVC_add || !SZHVC_sub )
		{
			LOG(("Z80: failed to allocate 2 * 128K flags arrays!!!\n"));
			exit(1);
		}
		padd = &SZHVC_add[	0*256];
		padc = &SZHVC_add[256*256];
		psub = &SZHVC_sub[	0*256];
		psbc = &SZHVC_sub[256*256];
		for (oldval = 0; oldval < 256; oldval++)
		{
			for (newval = 0; newval < 256; newval++)
			{
				/* add or adc w/o carry set */
				val = newval - oldval;
				*padd = (newval) ? ((newval & 0x80) ? SF : 0) : ZF;
#if Z80_EXACT
				*padd |= (newval & (YF | XF));	/* undocumented flag bits 5+3 */
#endif
				if( (newval & 0x0f) < (oldval & 0x0f) ) *padd |= HF;
				if( newval < oldval ) *padd |= CF;
				if( (val^oldval^0x80) & (val^newval) & 0x80 ) *padd |= VF;
				padd++;

				/* adc with carry set */
				val = newval - oldval - 1;
				*padc = (newval) ? ((newval & 0x80) ? SF : 0) : ZF;
#if Z80_EXACT
				*padc |= (newval & (YF | XF));	/* undocumented flag bits 5+3 */
#endif
				if( (newval & 0x0f) <= (oldval & 0x0f) ) *padc |= HF;
				if( newval <= oldval ) *padc |= CF;
				if( (val^oldval^0x80) & (val^newval) & 0x80 ) *padc |= VF;
				padc++;

				/* cp, sub or sbc w/o carry set */
				val = oldval - newval;
				*psub = NF | ((newval) ? ((newval & 0x80) ? SF : 0) : ZF);
#if Z80_EXACT
				*psub |= (newval & (YF | XF));	/* undocumented flag bits 5+3 */
#endif
				if( (newval & 0x0f) > (oldval & 0x0f) ) *psub |= HF;
				if( newval > oldval ) *psub |= CF;
				if( (val^oldval) & (oldval^newval) & 0x80 ) *psub |= VF;
				psub++;

				/* sbc with carry set */
				val = oldval - newval - 1;
				*psbc = NF | ((newval) ? ((newval & 0x80) ? SF : 0) : ZF);
#if Z80_EXACT
				*psbc |= (newval & (YF | XF));	/* undocumented flag bits 5+3 */
#endif
				if( (newval & 0x0f) >= (oldval & 0x0f) ) *psbc |= HF;
				if( newval >= oldval ) *psbc |= CF;
				if( (val^oldval) & (oldval^newval) & 0x80 ) *psbc |= VF;
				psbc++;
			}
		}
	}
#endif
	for (i = 0; i < 256; i++)
	{
		p = 0;
		if( i&0x01 ) ++p;
		if( i&0x02 ) ++p;
		if( i&0x04 ) ++p;
		if( i&0x08 ) ++p;
		if( i&0x10 ) ++p;
		if( i&0x20 ) ++p;
		if( i&0x40 ) ++p;
		if( i&0x80 ) ++p;
		SZ[i] = i ? i & SF : ZF;
#if Z80_EXACT
		SZ[i] |= (i & (YF | XF));		/* undocumented flag bits 5+3 */
#endif
		SZ_BIT[i] = i ? i & SF : ZF | PF;
#if Z80_EXACT
		SZ_BIT[i] |= (i & (YF | XF));	/* undocumented flag bits 5+3 */
#endif
		SZP[i] = SZ[i] | ((p & 1) ? 0 : PF);
		SZHV_inc[i] = SZ[i];
		if( i == 0x80 ) SZHV_inc[i] |= VF;
		if( (i & 0x0f) == 0x00 ) SZHV_inc[i] |= HF;
		SZHV_dec[i] = SZ[i] | NF;
		if( i == 0x7f ) SZHV_dec[i] |= VF;
		if( (i & 0x0f) == 0x0f ) SZHV_dec[i] |= HF;
	}

/*
	state_save_register_UINT16("z80", cpu, "AF", &Z80.AF.w.l, 1);
	state_save_register_UINT16("z80", cpu, "BC", &Z80.BC.w.l, 1);
	state_save_register_UINT16("z80", cpu, "DE", &Z80.DE.w.l, 1);
	state_save_register_UINT16("z80", cpu, "HL", &Z80.HL.w.l, 1);
	state_save_register_UINT16("z80", cpu, "IX", &Z80.IX.w.l, 1);
	state_save_register_UINT16("z80", cpu, "IY", &Z80.IY.w.l, 1);
	state_save_register_UINT16("z80", cpu, "PC", &Z80.PC.w.l, 1);
	state_save_register_UINT16("z80", cpu, "SP", &Z80.SP.w.l, 1);
	state_save_register_UINT16("z80", cpu, "AF2", &Z80.AF2.w.l, 1);
	state_save_register_UINT16("z80", cpu, "BC2", &Z80.BC2.w.l, 1);
	state_save_register_UINT16("z80", cpu, "DE2", &Z80.DE2.w.l, 1);
	state_save_register_UINT16("z80", cpu, "HL2", &Z80.HL2.w.l, 1);
	state_save_register_UINT8("z80", cpu, "R", &Z80.R, 1);
	state_save_register_UINT8("z80", cpu, "R2", &Z80.R2, 1);
	state_save_register_UINT8("z80", cpu, "IFF1", &Z80.IFF1, 1);
	state_save_register_UINT8("z80", cpu, "IFF2", &Z80.IFF2, 1);
	state_save_register_UINT8("z80", cpu, "HALT", &Z80.HALT, 1);
	state_save_register_UINT8("z80", cpu, "IM", &Z80.IM, 1);
	state_save_register_UINT8("z80", cpu, "I", &Z80.I, 1);
	state_save_register_UINT8("z80", cpu, "irq_max", &Z80.irq_max, 1);
	state_save_register_INT8("z80", cpu, "request_irq", &Z80.request_irq, 1);
	state_save_register_INT8("z80", cpu, "service_irq", &Z80.service_irq, 1);
	state_save_register_UINT8("z80", cpu, "int_state", Z80.int_state, 4);
	state_save_register_UINT8("z80", cpu, "nmi_state", &Z80.nmi_state, 1);
	state_save_register_UINT8("z80", cpu, "irq_state", &Z80.irq_state, 1);
*/
	/* daisy chain needs to be saved by z80ctc.c somehow */
}

/****************************************************************************
 * Reset registers to their initial values
 ****************************************************************************/
void z80_reset(void *param)
{
	Z80_DaisyChain *daisy_chain = (Z80_DaisyChain *)param;
	memset(&Z80, 0, sizeof(Z80));
	_IX = _IY = 0xffff; /* IX and IY are FFFF after a reset! */
	_F = ZF;			/* Zero flag is set */
	Z80.request_irq = -1;
	Z80.service_irq = -1;
	Z80.nmi_state = CLEAR_LINE;
	Z80.irq_state = CLEAR_LINE;

	if( daisy_chain )
	{
		while( daisy_chain->irq_param != -1 && Z80.irq_max < Z80_MAXDAISY )
		{
			/* set callbackhandler after reti */
			Z80.irq[Z80.irq_max] = *daisy_chain;
			/* device reset */
			if( Z80.irq[Z80.irq_max].reset )
				Z80.irq[Z80.irq_max].reset(Z80.irq[Z80.irq_max].irq_param);
			Z80.irq_max++;
			daisy_chain++;
		}
	}

	change_pc16(_PCD);
}

void z80_exit(void)
{
#if BIG_FLAGS_ARRAY
	if (SZHVC_add) free(SZHVC_add);
	SZHVC_add = NULL;
	if (SZHVC_sub) free(SZHVC_sub);
	SZHVC_sub = NULL;
#endif
}

/****************************************************************************
 * Execute 'cycles' T-states. Return number of T-states really executed
 ****************************************************************************/
int z80_execute(int cycles)
{
	z80_ICount = cycles - Z80.extra_cycles;
	Z80.extra_cycles = 0;

	do
	{
		_PPC = _PCD;
		CALL_MAME_DEBUG;
		_Z80_R++;
		EXEC_INLINE(op,ROP());
	} while( z80_ICount > 0 );

	z80_ICount -= Z80.extra_cycles;
	Z80.extra_cycles = 0;

	return cycles - z80_ICount;
}

/****************************************************************************
 * Burn 'cycles' T-states. Adjust R register for the lost time
 ****************************************************************************/
void z80_burn(int cycles)
{
	if( cycles > 0 )
	{
		/* NOP takes 4 cycles per instruction */
		int n = (cycles + 3) / 4;
		_Z80_R += n;
		z80_ICount -= 4 * n;
	}
}

/****************************************************************************
 * Get all registers in given buffer
 ****************************************************************************/
unsigned z80_get_context (void *dst)
{
	if( dst )
		*(Z80_Regs*)dst = Z80;
	return sizeof(Z80_Regs);
}

/****************************************************************************
 * Set all registers to given values
 ****************************************************************************/
void z80_set_context (void *src)
{
	if( src )
		Z80 = *(Z80_Regs*)src;
	change_pc16(_PCD);
}

/****************************************************************************
 * Get a pointer to a cycle count table
 ****************************************************************************/
const void *z80_get_cycle_table (int which)
{
	if (which >= 0 && which <= Z80_TABLE_xycb)
		return cc[which];
	return NULL;
}

/****************************************************************************
 * Set a new cycle count table
 ****************************************************************************/
void z80_set_cycle_table (int which, void *new_table)
{
	if (which >= 0 && which <= Z80_TABLE_ex)
		cc[which] = new_table;
}

/****************************************************************************
 * Return a specific register
 ****************************************************************************/
unsigned z80_get_reg (int regnum)
{
	switch( regnum )
	{
		//case REG_PC: return _PCD;
		case Z80_PC: return Z80.PC.w.l;
		//case REG_SP: return _SPD;
		case Z80_SP: return Z80.SP.w.l;
		case Z80_AF: return Z80.AF.w.l;
		case Z80_BC: return Z80.BC.w.l;
		case Z80_DE: return Z80.DE.w.l;
		case Z80_HL: return Z80.HL.w.l;
		case Z80_IX: return Z80.IX.w.l;
		case Z80_IY: return Z80.IY.w.l;
		case Z80_R: return (Z80.R & 0x7f) | (Z80.R2 & 0x80);
		case Z80_I: return Z80.I;
		case Z80_AF2: return Z80.AF2.w.l;
		case Z80_BC2: return Z80.BC2.w.l;
		case Z80_DE2: return Z80.DE2.w.l;
		case Z80_HL2: return Z80.HL2.w.l;
		case Z80_IM: return Z80.IM;
		case Z80_IFF1: return Z80.IFF1;
		case Z80_IFF2: return Z80.IFF2;
		case Z80_HALT: return Z80.HALT;
		case Z80_NMI_STATE: return Z80.nmi_state;
		case Z80_IRQ_STATE: return Z80.irq_state;
		case Z80_DC0: return Z80.int_state[0];
		case Z80_DC1: return Z80.int_state[1];
		case Z80_DC2: return Z80.int_state[2];
		case Z80_DC3: return Z80.int_state[3];
		case REG_PREVIOUSPC: return Z80.PREPC.w.l;
		default:
			if( regnum <= REG_SP_CONTENTS )
			{
				unsigned offset = _SPD + 2 * (REG_SP_CONTENTS - regnum);
				if( offset < 0xffff )
					return RM( offset ) | ( RM( offset + 1) << 8 );
			}
	}
	return 0;
}

/****************************************************************************
 * Set a specific register
 ****************************************************************************/
void z80_set_reg (int regnum, unsigned val)
{
	switch( regnum )
	{
		//case REG_PC: _PC = val; change_pc16(_PCD); break;
		case Z80_PC: Z80.PC.w.l = val; break;
		//case REG_SP: _SP = val; break;
		case Z80_SP: Z80.SP.w.l = val; break;
		case Z80_AF: Z80.AF.w.l = val; break;
		case Z80_BC: Z80.BC.w.l = val; break;
		case Z80_DE: Z80.DE.w.l = val; break;
		case Z80_HL: Z80.HL.w.l = val; break;
		case Z80_IX: Z80.IX.w.l = val; break;
		case Z80_IY: Z80.IY.w.l = val; break;
		case Z80_R: Z80.R = val; Z80.R2 = val & 0x80; break;
		case Z80_I: Z80.I = val; break;
		case Z80_AF2: Z80.AF2.w.l = val; break;
		case Z80_BC2: Z80.BC2.w.l = val; break;
		case Z80_DE2: Z80.DE2.w.l = val; break;
		case Z80_HL2: Z80.HL2.w.l = val; break;
		case Z80_IM: Z80.IM = val; break;
		case Z80_IFF1: Z80.IFF1 = val; break;
		case Z80_IFF2: Z80.IFF2 = val; break;
		case Z80_HALT: Z80.HALT = val; break;
		case Z80_NMI_STATE: z80_set_irq_line(IRQ_LINE_NMI,val); break;
		case Z80_IRQ_STATE: z80_set_irq_line(0,val); break;
		case Z80_DC0: Z80.int_state[0] = val; break;
		case Z80_DC1: Z80.int_state[1] = val; break;
		case Z80_DC2: Z80.int_state[2] = val; break;
		case Z80_DC3: Z80.int_state[3] = val; break;
		default:
			if( regnum <= REG_SP_CONTENTS )
			{
				unsigned offset = _SPD + 2 * (REG_SP_CONTENTS - regnum);
				if( offset < 0xffff )
				{
					WM( offset, val & 0xff );
					WM( offset+1, (val >> 8) & 0xff );
				}
			}
	}
}

/****************************************************************************
 * Set IRQ line state
 ****************************************************************************/
void z80_set_irq_line(int irqline, int state)
{
	if (irqline == IRQ_LINE_NMI)
	{
		if( Z80.nmi_state == state ) return;

		LOG(("Z80 #%d set_irq_line (NMI) %d\n", cpu_getactivecpu(), state));
		Z80.nmi_state = state;
		if( state == CLEAR_LINE ) return;

		LOG(("Z80 #%d take NMI\n", cpu_getactivecpu()));
		_PPC = -1;			/* there isn't a valid previous program counter */
		LEAVE_HALT;			/* Check if processor was halted */

		_IFF1 = 0;
		PUSH( PC );
		_PCD = 0x0066;
		Z80.extra_cycles += 11;
	}
	else
	{
		LOG(("Z80 #%d set_irq_line %d\n",cpu_getactivecpu() , state));
		Z80.irq_state = state;
		if( state == CLEAR_LINE ) return;

		if( Z80.irq_max )
		{
			int daisychain, device, int_state;
			daisychain = (*Z80.irq_callback)(irqline);
			device = daisychain >> 8;
			int_state = daisychain & 0xff;
			LOG(("Z80 #%d daisy chain $%04x -> device %d, state $%02x",cpu_getactivecpu(), daisychain, device, int_state));

			if( Z80.int_state[device] != int_state )
			{
				LOG((" change\n"));
				/* set new interrupt status */
				Z80.int_state[device] = int_state;
				/* check interrupt status */
				Z80.request_irq = Z80.service_irq = -1;

				/* search higher IRQ or IEO */
				for( device = 0 ; device < Z80.irq_max ; device ++ )
				{
					/* IEO = disable ? */
					if( Z80.int_state[device] & Z80_INT_IEO )
					{
						Z80.request_irq = -1;		/* if IEO is disable , masking lower IRQ */
						Z80.service_irq = device;	/* set highest interrupt service device */
					}
					/* IRQ = request ? */
					if( Z80.int_state[device] & Z80_INT_REQ )
						Z80.request_irq = device;
				}
				LOG(("Z80 #%d daisy chain service_irq $%02x, request_irq $%02x\n", cpu_getactivecpu(), Z80.service_irq, Z80.request_irq));
				if( Z80.request_irq < 0 ) return;
			}
			else
			{
				LOG((" no change\n"));
				return;
			}
		}
		take_interrupt();
	}
}

/****************************************************************************
 * Set IRQ vector callback
 ****************************************************************************/
void z80_set_irq_callback(int (*callback)(int))
{
	LOG(("Z80 #%d set_irq_callback $%08x\n",cpu_getactivecpu() , (int)callback));
	Z80.irq_callback = callback;
}

/****************************************************************************
 * Return a formatted string for a register
 ****************************************************************************/
const char *z80_info(void *context, int regnum)
{
	static char buffer[32][47+1];
	static int which = 0;
	Z80_Regs *r = context;

	which = (which+1) % 32;
	buffer[which][0] = '\0';
	if( !context )
		r = &Z80;

	switch( regnum )
	{
		case CPU_INFO_REG+Z80_PC: sprintf(buffer[which], "PC:%04X", r->PC.w.l); break;
		case CPU_INFO_REG+Z80_SP: sprintf(buffer[which], "SP:%04X", r->SP.w.l); break;
		case CPU_INFO_REG+Z80_AF: sprintf(buffer[which], "AF:%04X", r->AF.w.l); break;
		case CPU_INFO_REG+Z80_BC: sprintf(buffer[which], "BC:%04X", r->BC.w.l); break;
		case CPU_INFO_REG+Z80_DE: sprintf(buffer[which], "DE:%04X", r->DE.w.l); break;
		case CPU_INFO_REG+Z80_HL: sprintf(buffer[which], "HL:%04X", r->HL.w.l); break;
		case CPU_INFO_REG+Z80_IX: sprintf(buffer[which], "IX:%04X", r->IX.w.l); break;
		case CPU_INFO_REG+Z80_IY: sprintf(buffer[which], "IY:%04X", r->IY.w.l); break;
		case CPU_INFO_REG+Z80_R: sprintf(buffer[which], "R:%02X", (r->R & 0x7f) | (r->R2 & 0x80)); break;
		case CPU_INFO_REG+Z80_I: sprintf(buffer[which], "I:%02X", r->I); break;
		case CPU_INFO_REG+Z80_AF2: sprintf(buffer[which], "AF'%04X", r->AF2.w.l); break;
		case CPU_INFO_REG+Z80_BC2: sprintf(buffer[which], "BC'%04X", r->BC2.w.l); break;
		case CPU_INFO_REG+Z80_DE2: sprintf(buffer[which], "DE'%04X", r->DE2.w.l); break;
		case CPU_INFO_REG+Z80_HL2: sprintf(buffer[which], "HL'%04X", r->HL2.w.l); break;
		case CPU_INFO_REG+Z80_IM: sprintf(buffer[which], "IM:%X", r->IM); break;
		case CPU_INFO_REG+Z80_IFF1: sprintf(buffer[which], "IFF1:%X", r->IFF1); break;
		case CPU_INFO_REG+Z80_IFF2: sprintf(buffer[which], "IFF2:%X", r->IFF2); break;
		case CPU_INFO_REG+Z80_HALT: sprintf(buffer[which], "HALT:%X", r->HALT); break;
		case CPU_INFO_REG+Z80_NMI_STATE: sprintf(buffer[which], "NMI:%X", r->nmi_state); break;
		case CPU_INFO_REG+Z80_IRQ_STATE: sprintf(buffer[which], "IRQ:%X", r->irq_state); break;
		case CPU_INFO_REG+Z80_DC0: if(Z80.irq_max >= 1) sprintf(buffer[which], "DC0:%X", r->int_state[0]); break;
		case CPU_INFO_REG+Z80_DC1: if(Z80.irq_max >= 2) sprintf(buffer[which], "DC1:%X", r->int_state[1]); break;
		case CPU_INFO_REG+Z80_DC2: if(Z80.irq_max >= 3) sprintf(buffer[which], "DC2:%X", r->int_state[2]); break;
		case CPU_INFO_REG+Z80_DC3: if(Z80.irq_max >= 4) sprintf(buffer[which], "DC3:%X", r->int_state[3]); break;
		case CPU_INFO_FLAGS:
			sprintf(buffer[which], "%c%c%c%c%c%c%c%c",
				r->AF.b.l & 0x80 ? 'S':'.',
				r->AF.b.l & 0x40 ? 'Z':'.',
				r->AF.b.l & 0x20 ? '5':'.',
				r->AF.b.l & 0x10 ? 'H':'.',
				r->AF.b.l & 0x08 ? '3':'.',
				r->AF.b.l & 0x04 ? 'P':'.',
				r->AF.b.l & 0x02 ? 'N':'.',
				r->AF.b.l & 0x01 ? 'C':'.');
			break;
		case CPU_INFO_NAME: return "Z80";
		case CPU_INFO_FAMILY: return "Zilog Z80";
		case CPU_INFO_VERSION: return "3.5";
		case CPU_INFO_FILE: return __FILE__;
		case CPU_INFO_CREDITS: return "Copyright (C) 1998,1999,2000 Juergen Buchmueller, all rights reserved.";
		case CPU_INFO_REG_LAYOUT: return (const char *)z80_reg_layout;
		case CPU_INFO_WIN_LAYOUT: return (const char *)z80_win_layout;
	}
	return buffer[which];
}

unsigned z80_dasm( char *buffer, unsigned pc )
{
#ifdef MAME_DEBUG
	return DasmZ80( buffer, pc );
#else
	sprintf( buffer, "$%02X", cpu_readop(pc) );
	return 1;
#endif
}

