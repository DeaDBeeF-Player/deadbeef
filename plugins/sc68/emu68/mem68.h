/**
 * @ingroup   emu68_devel
 * @file      mem68.h
 * @author    Benjamin Gerard <ben@sashipa.com>
 * @date      13/03/1999
 * @brief     68k memory and IO manager.
 * @version   $Id: mem68.h,v 2.1 2003/09/30 06:29:57 benjihan Exp $ 
 *
 *   EMU68 memory manager assumes that all addresses in the lowest half part
 *   of address space are memory access. A simple bit test over most
 *   signifiant bit (23) of address allow to choose beetween memory or
 *   eventual IO access. In case of memory access, address is masked to fit
 *   available 68K onboard memory. Overflow does NOT generate address error. 
 *   IO access are performed towards quick access tables. There are 6
 *   acccess tables: for each read and write access in 3 sizes (byte, word
 *   and long).  Each of this 6 tables has 256 entries filled with a pointer
 *   to suitable function. At init time, the entries of all tables are
 *   initialized to access 68K onboard memory. When an IO is plugged by
 *   user, it is mapped somewhere in 68K address space. EMU68 memory manager
 *   get bit 8 to 15 of address to make an index to be used in the suitable
 *   table (R/W for B/W/L).
 *
 *   Featuring
 *   - Onboard memory byte, word and long read/write access.
 *   - Optimized IO warm mapping/unmapping.
 *   - Optionnal (compile time) enhanced memory control with RWX access tag
 *     and hardware breakpoints.
 *
 *   Limitations
 *   - For optimization purposes IO must be mapped in high half memory
 *     (bit 23 of address setted).
 *   - Two IO can not shared the same memory location for bit 8 to 15.
 *     Conflicts could be resolved by creating an intermediate IO which
 *     dispatches to others. This mechanism has not been implemented yet, so
 *     users must do it them self if needed.
 *
 *   Atari-ST & Amiga IO areas
 *   - @p  FF8800-FF88FF : YM2149 (ST)
 *   - @p  FF8900-FF89FF : Micro-Wire (STE)
 *   - @p  FF8200-FF82FF : Shifter (ST)
 *   - @p  FFFA00-FFFAFF : MFP (ST)
 *   - @p  DFF000-DFF0DF : Paula (AMIGA)
 */

/* Copyright (C) 1998-2001 Ben(jamin) Gerard */

#ifndef _MEM68_H_
#define _MEM68_H_

#include "emu68/struct68.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @name  Memory access flags for reg68.chk (debug mode only).
 *  @{
 */

#ifdef EMU68DEBUG
# define READ_68      1  /**< Memory location has been read */
# define WRITTEN_68   2  /**< Memory location has been written */
# define EXECUTED_68  4  /**< Memory location has been executed */
# define BREAKED_68   8  /**< Memory location has emulator-breakpoint */
#endif

/*@}*/


/** @name  Memory/IO quick access tables.
 *  @{
 */

/** Test for direct memory access or IO quick table access */
#define ISIO68(ADDR) ((ADDR)&0x800000)

extern memrfunc68_t read_mem_jmp_l[256];  /**< Read long */
extern memrfunc68_t read_mem_jmp_w[256];  /**< Read word */
extern memrfunc68_t read_mem_jmp_b[256];  /**< Read byte */
extern memwfunc68_t write_mem_jmp_l[256]; /**< Write long */
extern memwfunc68_t write_mem_jmp_w[256]; /**< Write word */
extern memwfunc68_t write_mem_jmp_b[256]; /**< Write byte */

/*@}*/


/** @name  68K onboard memory access.
 *  @{
 */

extern u32 read_68000mem_b(u32 addr); /**< Read memory byte */
extern u32 read_68000mem_w(u32 addr); /**< Read memory word */
extern u32 read_68000mem_l(u32 addr); /**< Read memory long */

extern void write_68000mem_b(u32 addr,u32 v); /**< Write memory byte */
extern void write_68000mem_w(u32 addr,u32 v); /**< Write memory word */
extern void write_68000mem_l(u32 addr,u32 v); /**< Write memory long */

#define read_B(ADDR) read_68000mem_b(ADDR) /**< Read memory byte */
#define read_W(ADDR) read_68000mem_w(ADDR) /**< Read memory word */
#define read_L(ADDR) read_68000mem_l(ADDR) /**< Read memory long */

/** Write memory byte */
#define write_B(ADDR,VAL) write_68000mem_b(ADDR,VAL)
/** Write memory word */
#define write_W(ADDR,VAL) write_68000mem_w(ADDR,VAL)
/** Write memory long */
#define write_L(ADDR,VAL) write_68000mem_l(ADDR,VAL)

/*@}*/


/** @name Instruction read.
 *  @{
 */

s32 get_nextw(void);  /**< Decode word and update PC */
s32 get_nextl(void);  /**< Decode long and update PC */

/*@}*/


/** @name Stack access.
 *  @{
 */
void pushl(s32 v);  /**< Push long */
void pushw(s32 v);  /**< Push word */
s32 popl(void);     /**< Pop long */
s32 popw(void);     /**< Pop word */
/*@}*/

/** Init memory quick access table.
 *
 *    The EMU68memory_init() function must be call at init time. Currently
 *    this function only call the EMU68memory_reset() function.
 *
 *  @see EMU68memory_reset()
 */
void EMU68memory_init(void);

/** Reset memory quick access table.
 *
 *    The EMU68memory_reset() function restores all memory access to
 *    default. All mapped IO will be discard and replace by onboard memory
 *    access.
 */
void EMU68memory_reset(void);

/** Add a new memory access control area (for new IO)
 *
 *  @param  area      Which area (bit 16 to 23 of address) to change.
 *  @param  read_bwl  Read function table (byte, word and long in this order)
 *  @param  write_bwl idem read_bwl for write access.
 *
 * @see EMU68memory_reset_area()
 */
void EMU68memory_new_area(u8 area,
                          memrfunc68_t *read_bwl,
                          memwfunc68_t *write_bwl);

/** Reset memory access control area to default state.
 *
 * @see EMU68memory_new_area()
*/
void EMU68memory_reset_area(u8 area);


#ifdef __cplusplus
}
#endif

#endif /* #ifndef _MEM68_H_ */

