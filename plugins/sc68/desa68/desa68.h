/**
 * @ingroup   lib_desa68
 * @file      desa68.h
 * @brief     Motorola 68K disassembler header.
 * @author    Benjamin Gerard
 * @date      1999-03-17
 */

/* Copyright (c) 1998-2015 Benjamin Gerard */

#ifndef DESA68_H
#define DESA68_H

#ifndef DESA68_EXTERN
# ifdef __cplusplus
#   define DESA68_EXTERN extern "C"
# else
#   define DESA68_EXTERN extern
# endif
#endif

#ifndef DESA68_API
/* Building */
# ifdef DESA68_EXPORT
#  if defined(DLL_EXPORT) && defined(HAVE_DECLSPEC)
#   define DESA68_API __declspec(dllexport)
#  elif defined(HAVE_VISIBILITY)
#   define DESA68_API DESA68_EXTERN __attribute__ ((visibility("default")))
#  else
#   define DESA68_API DESA68_EXTERN
#  endif
/* Using */
# else
#  if defined(DESA68_DLL)
#   define DESA68_API __declspec(dllimport)
#  else
#   define DESA68_API DESA68_EXTERN
#  endif
# endif
#endif

/**
 * @defgroup lib_desa68 desa68 library
 * @ingroup  sc68_dev_lib
 *
 * desa68 is a standalone library to disassemble 68000 machine code.
 *
 * Additionally desa68 classes instructions by category; along with it
 * determines possible program branch. Together with it helps to
 * source machine code.
 *
 * Optionnally the disassembler may disassemble with symbol instead of
 * absolute address or long immediat. A supplemental control is
 * available to choose the range of address that must be disassembled
 * as symbol.
 *
 * A complete example of how to use it may be found in the @ref
 * prg_sourcer68 sourcer68 directory. This library is also used by the
 * deprecated @ref prg_debug68 debug68 and @ref prg_cdbg68 cdbg68
 * programs.
 *
 * @{
 */

#ifndef DESA68_API
# error "DESA68_API should be defined"
#endif

/**
 * Disassembly option flags.
 * @anchor desa68_opt_flags
 *
 * Use bitwise OR operation with these values to set the the
 * desa68_parm_t::flags in order to configure the disassembler.
 *
 */
enum {
  /**
   * Disassemble with symbol.
   *
   * If the DESA68_SYMBOL_FLAG is set in the desa68_parm_t::flags and
   * the value of absolute long addressing mode or an immediat long is
   * in greater or equal to desa68_parm_t::immsym_min and less than
   * desa68_parm_t::immsym_max then the disassembler replaces the
   * value by a named symbol.  The named symbol constist on the value
   * transformed to an 6 hexadecimal digit number with a prefixed 'L'.
   */
  DESA68_SYMBOL_FLAG = (1<<0),

  /**
   * Disassemble with ascii char.
   *
   * If the DESA68_ASCII_FLAG is set in the desa68_parm_t::flags
   * immediat values are converted to ASCII string (if possible).
   */
  DESA68_ASCII_FLAG = (1<<1),

  /**
   * Force symbol disassemble.
   *
   * The DESA68_FORCESYMB_FLAG is a set of 5 bits. If the Nth bit is
   * set it forces a symbolic dissassembly for a long starting at the
   * Nth word.  Since 68000 instructions are not more than 10 bytes
   * long 5 bit is just enough.
   */
  DESA68_FORCESYMB_FLAG = (1<<8),
};

/**
 * Instruction type flags.
 * @anchor desa68_inst_flags
 *
 * These flags are setted in the desa68_parm_t::status field by
 * desa68() function. It allow to determine the type of the
 * dissassembled instruction.
 *
 */
enum {
  /** Valid instruction. */
  DESA68_INST = (1<<0),

  /** Branch always instruction (bra/jmp). */
  DESA68_BRA  = (1<<1),

  /** Subroutine (bsr/jsr) or Conditionnal branch instruction (bcc/dbcc). */
  DESA68_BSR  = (1<<2),

  /** Return from subroutine/Interruption instruction (rts/rte). */
  DESA68_RTS  = (1<<3),

  /** Software interrupt instruction (trap/chk). */
  DESA68_INT  = (1<<4),

  /** nop instruction. */
  DESA68_NOP  = (1<<5)
};

/**
 * Type for the 68K disassemble pass parameters structure.
 */
typedef struct desa68_parm_s desa68_parm_t;

/**
 * 68K disassemble pass parameters.
 *
 * The desa68_parm_t data structure contains the information necessary to
 * disassemble 68K instructions.
 *
 * There are 3 categories of fields in this structure.
 * - Input parameters; Must be set before calling the desa68() function.
 * - Output parameters; Information on disassembled instruction
 *                          filled by desa68() function.
 * - Miscellaneous internal fields.
 *
 * @note The desa68_parm_t::pc field is both input and output since it is use
 *       to set the address of the instruction to decode and returns with
 *       the value of the next one.
 */
struct desa68_parm_s
{

  /**
   * @name Input parameters.
   *
   * These parameters must be set before calling the desa68() functions.
   *
   * @{
   */

  unsigned char *mem;    /**< Base of 68K memory.                         */
  unsigned int   memmsk; /**< Size of memory - 1 (mask).                  */
  /**
   * Address (Offset in mem) of instruction to disassemble; Returns
   * with the address of the next instruction.
   */
  unsigned int   pc;
  int            flags;  /**< @ref desa68_opt_flags "Disassemble options" */
  char          *str;    /**< Destination string.                         */
  int            strmax; /**< Destination string buffer size.
                            @warning Unused                               */
  /**
   * Minimum value to interpret long immediat or absolute long as symbol.
   * @see DESA68_SYMBOL_FLAG for more details
   * @see immsym_max
   */
  unsigned int   immsym_min;
  /**
   * Maximum value to interpret long immediat or absolute long as symbol.
   * @see DESA68_SYMBOL_FLAG for more details
   * @see immsym_min
   */
  unsigned int   immsym_max;

  /** @} */


  /**
   * @name Output parameters.
   *
   * These parameters are setted by the desa68() functions.
   * @{
   */

  /**
   * Effective address of source operand (-1:not a memory operand).
   * Use DESA68_INDIRECT_EA to detect indirection.
   */
  unsigned int ea_src;
  /**
   * Effective address of destination operand (-1:not a memory operand).
   * Use DESA68_INDIRECT_EA to detect indirection.
   */
  unsigned int ea_dst;

  /**
   * @ref desa68_inst_flags "disassembly instruction flags".
   */
  unsigned int status;

  /**
   * Branch or interrupt vector address.
   *
   * If the dissassembled instruction was a branch a call or a
   * sotfware interrupt the desa68_parm_t::branch is set to the jump
   * address or the interrupt vector involved.
   *
   * @see status for more information on instruction type.
   */
  unsigned int branch;

  /**
   * Last decoded word (16 bit sign extended).
   */
  int w;

  /**
   *Pointer to current destination char.
   */
  char *s;

  /**
   * @}
   */


  /**
   * @name Miscellaneous internal variables.
   * @internal
   * @{
   */

  /**
   * Intermediat opcode decoding.
   */
  unsigned int pc_org;
  int   reg0;
  int   reg9;
  int   mode3;
  int   mode6;
  int   opsz;
  int   line;
  int   adrmode0;
  int   adrmode6;
  int   szchar;
  unsigned int ea;

  /**
   * @}
   */

};

DESA68_API
/**
 * Disassemble a single 68000 instruction.
 *
 * @param  d  Pointer to disassemble pass parameter structure.
 */
void desa68(desa68_parm_t *d);


DESA68_API
/**
 * Get version number.
 *
 * @return X*100+Y*10+Z.
 */
int desa68_version(void);


DESA68_API
/**
 * Get version string.
 *
 * @return version string.
 */
const char * desa68_versionstr(void);

/**
 * @}
 */

#endif
