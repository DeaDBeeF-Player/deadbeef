/*
 * @file    de68.c
 * @brief   68000 disassembler
 * @author  http://sourceforge.net/users/benjihan
 *
 * Copyright (c) 1998-2015 Benjamin Gerard
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.
 *
 * If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "desa68.h"

/**********************************************************
 * Defines my types :                                      *
 * Should be OK without configure, since 32-bit and 16-bit *
 * are not really needed.                                  *
 **********************************************************/

#if defined(HAVE_STDINT_H)
#include <stdint.h>
typedef int8_t   s8;
typedef uint8_t  u8;
typedef int16_t  s16;
typedef uint16_t u16;
/* typedef int32_t  s32; */
/* typedef uint32_t u32; */
#else
typedef signed char    s8;
typedef unsigned char  u8;
typedef unsigned short u16;
typedef short          s16;
#endif

typedef int            s32;
typedef unsigned int   u32;

#define REG0(W)         (((W))&7)
#define REG9(W)         (((W)>>9)&7)
#define OPSZ(W)         (((W)>>6)&3)
#define LINE(W)         (((W)>>12)&15)
#define MODE3(W)        (((W)>>3)&7)
#define MODE6(W)        (((W)>>6)&7)

/******************************
 * Disassembler string tables *
 ******************************/

enum adressingword {
  MODE_DN=0,
  MODE_AN,
  MODE_iAN,
  MODE_ANp,
  MODE_pAN,
  MODE_dAN,
  MODE_dANXI,
  MODE_ABSW,
  MODE_ABSL,
  MODE_dPC,
  MODE_dPCXI,
  MODE_IMM
};

/* Hexa digit table */
static const char Thex[16] = {
  '0','1','2','3','4','5','6','7',
  '8','9','A','B','C','D','E','F'
};

/* Branch Condition Code String Tables */
static const u16 bcc_ascii[] = {
  'RA','SR','HI','LS',
  'CC','CS','NE','EQ',
  'VC','VS','PL','MI',
  'GE','LT','GT','LE'
};

/* Scc Condition Code String Tables */
static const u16 scc_ascii[] = {
  'T', 'F','HI','LS',
  'CC','CS','NE','EQ',
  'VC','VS','PL','MI',
  'GE','LT','GT','LE'
};

/* Dbcc Condition Code String Tables */
static const u16 dbcc_ascii[] = {
  'RA', 'F','HI','LS',
  'CC','CS','NE','EQ',
  'VC','VS','PL','MI',
  'GE','LT','GT','LE'
};

/*******************
 * String functions *
 *******************/

/* Add a char to disassembly string */
#if 0
static void desa_char(desa68_parm_t * d,const unsigned char c)
{
  *d->s++ = c;
}
#else
# define desa_char(D,C) (*(D)->s++ = (C))
#endif

/* Add a string to disassembly string */
static void desa_str(desa68_parm_t * d, char *str)
{
  char c;
  while (c=*str++, c)
    desa_char(d, c);
}

/* Add a string to disassembly string */
static void desa_ascii(desa68_parm_t * d, unsigned int n)
{
  int shift;
  for (shift=(sizeof(int)-1)*8; shift>=0; shift-=8) {
    u8 c;
    c = (u8)(n>>shift);
    if (c) desa_char(d, c);
  }
}

/* Add a N-digit unsigned hexa number with header char
   to disassembly string */
static void desa_uhexacat(desa68_parm_t * d, unsigned int n,
                          int ndigit, int header_char)
{
  int shf;
  desa_char(d,header_char);
  for (shf=(ndigit-1)*4; shf>=0; shf-=4) {
    desa_char(d,Thex[(n>>shf)&15] );
  }
}

/* Add a signifiant digit only unsigned hexa number
 * with heading '$' to disassembly string
 */
static void desa_usignifiant(desa68_parm_t * d, unsigned int n)
{
  int shf;
  desa_char(d,'$');
  for (shf=(sizeof(int)*2-1)*4; shf>=0 && !(n>>shf); shf-=4)
    ;
  if (shf<0) shf=0;
  for (; shf>=0; shf-=4)
    desa_char(d,Thex[(n>>shf)&15] );
}

/* idem desa_usignifiant, except it is signed */
static void desa_signifiant(desa68_parm_t * d, int n)
{
  if (n<0) {
    desa_char(d,'-');
    n = -n;
  }
  desa_usignifiant(d,n);
}

static int my_isascii(u8 c)
{
  return
    c=='_' || c==' ' || c == '!' || c == '.' || c == '#'
    || (c>='a' && c<='z')
    || (c>='A' && c<='Z')
    || (c>='0' && c<='9');
}

/********************************
 * General disassembly function *
 *******************************/

/* Read next word , increment pc */
static int read_pc(desa68_parm_t * d)
{
  unsigned int pc = d->pc;
  d->w   = (s8)d->mem[pc++&d->memmsk]<<8;
  d->w  += d->mem[pc++&d->memmsk];
  d->pc  = pc;
  return d->w;
}

static s32 immB(desa68_parm_t * d)
{
  return (s32)(s8)read_pc(d);
}

#define immW read_pc

static s32 immL(desa68_parm_t * d)
{
  return (read_pc(d)<<16) | (read_pc(d)&0xffff);
}

static s32 adrW(desa68_parm_t * d)
{
  return immW(d);
}

static s32 adrL(desa68_parm_t * d)
{
  return immL(d);
}

static s32 relPC(desa68_parm_t * d)
{
  read_pc(d);
  return (d->pc + d->w - 2) & d->memmsk;
}

/* return [AD][0-7][d][WL] : SZ='W'/'L' XI = hexa value for reg ($D0) */
static s32 indAnXi(desa68_parm_t * d)
{
  s32 v;
  read_pc(d);
  v  = (d->w&0x8000)? ('A'<<24) : ('D'<<24);
  v |= ('0'+((d->w>>12)&7)) << 16;
  v |= ((u8)d->w)<<8;
  v |= ((d->w&(1<<11)) ? 'L' : 'W');
  return v;
}

static void desa_dcw(desa68_parm_t * d)
{
  desa_ascii(d,'DC.W');
  desa_char (d,' ');
  desa_uhexacat(d,(u16)d->w, 4, '$');
  d->status = 0;
}

static void update_ea(desa68_parm_t * d, unsigned int v)
{
  if (d->ea_src == -1) {
    d->ea_src = v;
  } else if (d->ea_dst == -1) {
    d->ea_dst = v;
  }
}

static void desa_label(desa68_parm_t * d, unsigned int v)
{
  desa_uhexacat(d, v, 6, 'L');
}

static int desa_is_symbol(desa68_parm_t * d, unsigned int v2, unsigned int bit)
{
  int r =
    ((d->flags & DESA68_SYMBOL_FLAG)
     &&
     ( (bit < 5u && (d->flags & (DESA68_FORCESYMB_FLAG<<bit)))
       || (v2>=d->immsym_min && v2<d->immsym_max))
      );
  return r;
}

static void desa_immL(desa68_parm_t * d, int v, int pc)
{
  unsigned int v2 = v;
  const int bit = (pc == -1)
    ? 256
    : ( ((pc - d->pc_org) & d->memmsk) >> 1);
  desa_char(d,'#');
  if (desa_is_symbol(d, v, bit)) {
    desa_label(d,v2);
    update_ea(d,v2);
  } else {
    if ((d->flags & DESA68_ASCII_FLAG)
        && my_isascii(v2)
        && my_isascii(v2>>8)
        && my_isascii(v2>>16)
        && my_isascii(v2>>24)) {
      desa_char (d,'\'');
      desa_ascii(d,(u32)v2);
      desa_char (d,'\'');
    } else {
      desa_signifiant(d,v);
    }
  }
}

static void desa_absL(desa68_parm_t * d, int v, int pc)
{
  unsigned int v2 = v & 0xFFFFFF;
  const int bit = (pc == -1)
    ? 256
    : ( ((pc - d->pc_org) & d->memmsk) >> 1);

  if (desa_is_symbol(d, v, bit)) {
    desa_uhexacat(d, v2, 6, 'L');
  } else {
    desa_usignifiant(d, v);
  }
}

static void get_ea_2(desa68_parm_t * d, u32 mode, u32 reg, u8 sz)
{
  s32 v;

  if (mode == MODE_ABSW) {
    mode += reg;
  }

  switch(mode ) {
  case MODE_DN:
    desa_ascii(d,'D0'+reg);
    break;
  case MODE_AN:
    desa_ascii(d,'A0'+reg);
    break;
  case MODE_iAN:
    desa_ascii(d,'(A0)'+(reg<<8));
    break;
  case MODE_pAN:
    desa_char(d,'-');
    desa_ascii(d,'(A0)'+(reg<<8));
    break;
  case MODE_ANp:
    desa_ascii(d,'(A0)'+(reg<<8));
    desa_char(d,'+');
    break;
  case MODE_dAN:
    read_pc(d);
    desa_signifiant(d,d->w);
    desa_ascii(d,'(A0)'+(reg<<8));
    break;
  case MODE_dANXI:
    v = indAnXi(d);
    desa_signifiant(d,(s8)(v>>8));
    desa_ascii(d,'(A0,'+(reg<<8));
    v = (v&0xFFFF00FF)+('.'<<8);
    desa_ascii(d,v);
    desa_char(d,')');
    break;
  case MODE_ABSW:
    d->ea = v = adrW(d);
    update_ea(d,v);
    desa_absL(d,v, (d->pc-2) & 0xFFFFFF );
    desa_ascii(d,'.W');
    break;
  case MODE_ABSL:
    d->ea = v = adrL(d);
    update_ea(d, v);
    desa_absL(d, v, (d->pc - 4) & 0xFFFFFF);
    break;
  case MODE_dPC:
    d->ea = v = relPC(d);
    update_ea(d,v);
    if (d->flags & DESA68_SYMBOL_FLAG) {
      desa_label(d, v);
    } else {
      desa_usignifiant(d, v);
    }
    desa_ascii(d,'(PC)');
    break;
  case MODE_dPCXI:
    v = indAnXi(d);
    d->ea = d->pc-2+(s8)(v>>8);
    update_ea(d,d->ea);
    if (d->flags & DESA68_SYMBOL_FLAG) {
      desa_label(d, d->ea);
    } else {
      desa_usignifiant(d, d->ea);
    }
    desa_ascii(d,'(PC,');
    v = (v&0xFFFF00FF)+('.'<<8);
    desa_ascii(d,v);
    desa_char(d,')');
    break;
  case MODE_IMM:
    switch(sz )
    {
    case 1:
    case 'B':
      v = (s8)immB(d);
      desa_char(d,'#');
      desa_signifiant(d,v);
      break;
    case 2:
    case 'W':
      v = /* (s16) */immW(d); /* should not be ok without cast */
      desa_char(d,'#');
      desa_signifiant(d,v);
      break;
    case 4:
    case 'L':
      v = (u32)immL(d);
      desa_immL(d,v,(d->pc-4)&0xFFFFFF);
      return;
    default:
      desa_ascii(d,'#?');
      break;
    }
    break;

  default:
    desa_char(d,'?');
    desa_usignifiant(d, mode);
    desa_char(d,'?');
    break;
  }
}

/* Get move operand
 * bit : 0/6 -> src/dest
 * w   : current opcode
 */
static void get_ea_move(desa68_parm_t * d, int bit, s32 w, u32 easz)
{
  u32 ea    = (w>>bit)&63;

  if (bit)
    get_ea_2(d, ea&7,ea>>3, easz);
  else if (bit==0)
    get_ea_2(d, ea>>3,ea&7, easz);
}

static void desa_reg(desa68_parm_t * d, int reg)
{
  desa_ascii(d,((reg&8)? 'A0':'D0') + (reg&7));
}

/* Used with ABCD, SBCD, ADDX, SUBX */
static void desa_ry_rx(desa68_parm_t * d, int inst, int size)
{
  desa_ascii(d,inst);
  desa_ascii(d,size);
  desa_char(d,' ');
  if (d->mode3&1) {             /* -(Ay),-(Ax) */
    desa_ascii(d,'-(A0'+d->reg0);
    desa_ascii(d,'),-(');
    desa_ascii(d,'A0)' + (d->reg9<<8));
  } else {                      /* Dy,Dx */
    desa_reg(d, d->reg0);
    desa_char(d,',');
    desa_reg(d, d->reg9);
  }
}

static void desa_dn_ae(desa68_parm_t * d, int name)
{
  desa_ascii(d, name);
  desa_ascii(d, d->szchar);
  desa_char (d, ' ');
  /*  dn,<ae> */
  if (d->w&0400) {
    desa_ascii(d, 'D0,' + (d->reg9<<8));
    get_ea_2(d, d->mode3, d->reg0, d->szchar);
  }

  /*  <ae>,dn */
  else {
    get_ea_2(d, d->mode3, d->reg0, d->szchar);
    desa_ascii(d,',D0'+d->reg9);
  }
}

/**************
 *
 *   LINE 0 :
 *   -Immediat operations
 *   -SR & CCR operations
 *   -Bit operations
 *   -Movep
 *
 ***************/

static int check_desa_bitop(desa68_parm_t * d)
{
  static u32 fn[] = { 'BTST', 'BCHG', 'BCLR', 'BSET'};
  int modemsk = 00775;
  unsigned int inst;

  if (!(modemsk&(1<<d->adrmode0)))
    return 0;

  inst = fn[d->opsz];
  if (!(d->w&0400)) {
    /* B... #x,<AE>*/
    if ((d->w&0xF00)!=0x800)
      return 0;
    desa_ascii(d,inst);
    desa_ascii(d,' #');
    read_pc(d);
    desa_usignifiant(d, (u8)d->w);
  } else {
    /* B... dn,<AE>*/
    desa_ascii(d,inst);
    desa_ascii(d,' D0'+d->reg9);
  }
  desa_char(d,',');
  get_ea_2(d, d->mode3, d->reg0, 'B');

  return 1;
}

static int check_desa_IMM_SR(desa68_parm_t * d)
{
  u32 modemsk = 1 + (1<<2) + (1<<10);
  int mode = (d->w>>8)&15, inst='ORI';

  if ((d->w&0677)!=0074 || !(modemsk&(1<<mode)))
    return 0;

  switch(mode) {
  case 0xA: /* EORI */
    inst = 'EORI';
    break;
#if 0
  case 0x0: /* ORI */
    inst = 'ORI';
    break;
#endif
  case 0x2: /* ANDI */
    inst = 'ANDI';
    break;
  }
  desa_ascii(d,inst);
  desa_char(d,' ');
  get_ea_2(d, MODE_ABSW, MODE_IMM-MODE_ABSW, 'W');
  desa_ascii(d,(d->mode6&1) ? ',SR' : ',CCR');
  return 1;
}

static int check_desa_movep(desa68_parm_t * d)
{
  if ((d->w & 0470) != 0410 )
    return 0;

  desa_str(d, "MOVEP.");
  desa_ascii(d,(d->opsz&1)?'L ' : 'W ');

  if (!(d->w&(1<<7))) {
    /* MOVEP.? d(Ax),Dy */
    get_ea_2(d, MODE_dAN, d->reg0, 0 );
    desa_ascii(d,',D0' + d->reg9 );
  } else {
    /* MOVEP.? Dx,d(Ay) */
    desa_ascii(d,'D0,' + (d->reg9<<8) );
    get_ea_2(d, MODE_dAN, d->reg0, 0 );
  }
  return 1;
}

static int check_desa_imm_op(desa68_parm_t * d)
{
  static u32 fn[8] = {
    'ORI' , 'ANDI', 'SUBI', 'ADDI',
    '???I', 'EORI', 'CMPI', '???I'
  };
  int modemsk=0x6F;

  if ((d->w&0x100) || !(modemsk&(1<<d->reg9)) || d->opsz==3)
    return 0;

  desa_ascii(d,fn[d->reg9] );
  desa_ascii(d,(d->szchar<<8) + ' ');
  get_ea_2(d, MODE_ABSW, MODE_IMM-MODE_ABSW, d->szchar);
  desa_char(d,',');
  get_ea_2(d, d->mode3, d->reg0, d->szchar);
  return 1;
}

static void desa_line0(desa68_parm_t * d)
{
  if (check_desa_movep(d) )     return;
  if (check_desa_bitop(d) )     return;
  if (check_desa_IMM_SR(d) )    return;
  if (check_desa_imm_op(d) )    return;
  else desa_dcw(d);
}

/* General move(a) disassemble
 */
static void desa_move(desa68_parm_t * d)
{
  s32 w=d->w;
  int movsz  = (u8)('WLB?' >> ((w&(3<<12))>>(12-3)));
  desa_ascii(d,'MOVE');
  desa_ascii(d,((d->adrmode6==MODE_AN)? ('A'<<24) : 0)
             + ('.'<<16) + ' ' + (movsz<<8) );
  get_ea_move(d,0,w,movsz);
  desa_char(d,',');
  get_ea_move(d,6,w,movsz);
}

/* Check and disassemble a valid move
 * return TRUE if valid move
 */
static int check_desa_move(desa68_parm_t * d)
{
  u32 srcmsk = 0xFFF, dstmsk = 0x1FF;
  /* Remove An source & destination addressing mode for byte access */
  if (d->line==0x1) {
    srcmsk &= ~(1<<MODE_AN);
    dstmsk &= ~(1<<MODE_AN);
  }
  if ((srcmsk&(1<<d->adrmode0)) && (dstmsk&(1<<d->adrmode6)) ) {
    desa_move(d);
    return 1;
  }
  return 0;
}

/**************
 *
 *   LINE 1 :
 *   -MOVE.B
 *   LINE 2 :
 *   -MOVE.L
 *   LINE 3 :
 *   -MOVE.W
 *
 ***************/

static void desa_li123(desa68_parm_t * d)
{
  if (!check_desa_move(d))
    desa_dcw(d);
}

/**************
 *
 *   LINE 4 :
 *   -Other instructions
 *
 ***************/

static void get_movemsub(desa68_parm_t * d,
                         s32 i, s32 j)
{
  desa_reg(d, i);
  if (i!=j) {
    desa_char(d,'-');
    desa_reg(d, j);
  }
}

static void get_movemreg(desa68_parm_t * d,
                         u32 v, u32 rev)
{
  s32 i,j,p=0;
  for (i=0 ; i<16; i++) {
    for (; i<16 && (v&(1<<(i^rev)))==0; i++);
    if (i==16) break;
    j = i;
    for (; i<16 && (v&(1<<(i^rev))); i++);
    if (p) desa_char(d,'/');
    get_movemsub(d,j,(i-1));
    p = 1;
  }
}

static int desa_check_imp(desa68_parm_t * d,
                          unsigned int name, int mskmode)
{
  if ((d->w&0400) || !(mskmode&(1<<d->adrmode0)))
    return 0;
  desa_ascii(d,name);
  desa_ascii(d,d->szchar);
  desa_char (d,' ');
  get_ea_2(d, d->mode3, d->reg0, d->szchar);
  return 1;
}

static int check_desa_lea(desa68_parm_t * d)
{
  int modemsk = 03744;
  if ((d->w&0700) != 0700 || !(modemsk&(1<<d->adrmode0)))
    return 0;
  desa_ascii(d,'LEA ');
  get_ea_2(d, d->mode3, d->reg0, 0);
  desa_ascii(d,',A0'+d->reg9);
  return 1;
}

static int check_desa_chk(desa68_parm_t * d)
{
  int modemsk = 07775;
  if ((d->w&0700) != 0600 || !(modemsk&(1<<d->adrmode0)))
    return 0;
  desa_ascii(d,'CHK ');
  get_ea_2(d, d->mode3, d->reg0, 0);
  desa_ascii(d,',D0'+d->reg9);
  d->status = DESA68_INST | DESA68_INT;
  d->branch = 0x18;
  return 1;
}

static int check_desa_ext(desa68_parm_t * d)
{
  if ((d->w&07670)!=04200)
    return 0;
  desa_ascii(d,'EXT.' );
  desa_ascii(d,(!(d->w&0100)? 'W D0':'L D0') + d->reg0);
  return 1;
}

static int check_desa_movem(desa68_parm_t * d)
{
  int modemsk, regmsk;
  int mode3, reg0, w;
  if ((d->w&05600)!=04200)
    return 0;
  modemsk = !(d->w&02000) ? 00764 : 03754;
  if (!(modemsk&(1<<d->adrmode0)))
    return 0;

  desa_ascii(d,'MOVE');
  desa_ascii(d,'M.');
  desa_char (d,!(d->w&0100)?'W':'L');
  desa_char (d,' ');

  w = d->w;
  mode3 = d->mode3;
  reg0 = d->reg0;
  regmsk = immW(d);

  if (w&02000) {
    /*  -> reg */
    get_ea_2(d, mode3, reg0, 0);
    desa_char(d,',');
    get_movemreg(d,regmsk,0);
  } else {
    /* -> mem */
    get_movemreg(d, regmsk, (mode3==MODE_pAN) ? 15 : 0);
    desa_char(d,',');
    get_ea_2(d, mode3, reg0, 0);
  }
  return 1;
}

static int check_desa_jmp(desa68_parm_t * d)
{
  int modemsk = 03744;
  if ((d->w&07600) != 07200) {
    return 0;
  }
  /* JMP / JSR */
  if (modemsk & (1<<d->adrmode0)) {
    int s = 'MP ', stat = DESA68_INST | DESA68_BRA;
    desa_char(d,'J');
    if (d->opsz==2) {
      stat = DESA68_INST | DESA68_BSR;
      s = 'SR ';
    }
    d->status = stat;
    desa_ascii(d,s);
    get_ea_2(d, d->mode3, d->reg0, d->szchar);
    d->branch = d->ea & 0xFFFFFF;

    return 1;
  }
  /* invalid JSR / JMP address mode */
  /* $$$ Not sure for a DC.W here */
  desa_dcw(d);
  return 1;
  /*return 0;*/
}

static int check_desa_line4_mode3(desa68_parm_t * d)
{
  if (d->mode6 != 3) {
    return 0;
  }

  switch(d->reg9) {

  case 0: {                     /* MOVE FROM SR */
    int modemsk = 00775;
    if (!(modemsk&(1<<d->adrmode0)))
      break;
    desa_ascii(d,'MOVE');
    desa_ascii(d,' SR,');
    get_ea_2(d, d->mode3, d->reg0, 'W');
    return 1;
  }

  case 1:
    break;

  case 2: {                     /* MOVE TO CCR */
    int modemsk = 07775;
    if (!(modemsk&(1<<d->adrmode0)))
      break;
    desa_ascii(d,'MOVE');
    desa_char (d, ' ');
    get_ea_2(d, d->mode3, d->reg0, 'B');
    desa_ascii(d,',CCR');
    return 1;
  }

  case 3: {                     /* MOVE TO SR */
    int modemsk = 07775;
    if (!(modemsk&(1<<d->adrmode0)))
      break;
    desa_ascii(d,'MOVE');
    desa_char (d, ' ');
    get_ea_2(d, d->mode3, d->reg0, 'W');
    desa_ascii(d,',SR');
    return 1;
  }

  case 4:
    break;

  case 5: {                     /* TAS */
    d->szchar = 0;
    if (desa_check_imp(d, 'TAS', 00775)) {
      return 1;
    } else if (d->w == 0x4AFC) {
      desa_ascii(d,'ILLE');
      desa_ascii(d,'GAL');
      d->status = DESA68_INST | DESA68_INT;
      d->branch = 0x10;
      return 1;
    }
    break;
  }

  case 6:
    break;

  case 7:
    break;
  }
  return 0;
}


static void desa_line4(desa68_parm_t * d)
{
  switch (d->mode6) {

  case 7:
    if (check_desa_lea(d)) return;
    break;
  case 5:
    if (check_desa_chk(d)) return;
    break;
  case 2:
  case 3:
    if (check_desa_ext(d)) return;
    else if (check_desa_movem(d)) return;
    else if (check_desa_jmp(d)) return;
    if (check_desa_line4_mode3(d)) return;

  default:

    switch(d->reg9) {
    case 0:                             /* NEGX */
      if (desa_check_imp(d, 'NEGX', 00775))
        return;
      break;

    case 1:                             /* CLR */
      if (desa_check_imp(d, 'CLR', 00775))
        return;
      break;

    case 2:                             /* NEG */
      if (desa_check_imp(d, 'NEG', 00775))
        return;
      break;

    case 3:                             /* NOT */
      if (desa_check_imp(d, 'NOT', 00775))
        return;
      break;

    case 4:
      if (d->mode6==0) {
        /* NBCD */
        d->szchar = 0;
        if (desa_check_imp(d, 'NBCD', 00775))
          return;
      } else if (d->mode6==1) {
        if (d->mode3 == MODE_DN) {
          /* SWAP */
          desa_ascii(d,'SWAP');
          desa_ascii(d,' D0'+d->reg0);
          return;
        } else {
          /* PEA */
          d->szchar = 0;
          if (desa_check_imp(d, 'PEA', 0x7E4))
            return;
        }
      }
      break;

    case 5:                             /* TST */
      if (desa_check_imp(d, 'TST', 00775))
        return;
      break;

    case 6:
      break;

    case 7:
      if (d->mode6 == 1) {
        /* FUNKY LINE 4 */
        switch(d->mode3) {
        case 0:
        case 1: {
          int num = d->w&0xF;
          desa_ascii(d,'TRAP');
          desa_ascii(d,' #$');
          desa_char (d, Thex[num]);
          d->status = DESA68_INST | DESA68_INT;
          d->branch = 0x80 + (num<<2);
        } return;
        case 2:
          desa_ascii(d,'LINK');
          desa_ascii(d,' A0,'+(d->reg0<<8));
          get_ea_2(d, MODE_ABSW, MODE_IMM-MODE_ABSW, 'W');
          return;
        case 3:
          desa_ascii(d,'UNLK');
          desa_ascii(d,' A0'+d->reg0);
          return;
        case 4:
          desa_ascii(d,'MOVE');
          desa_ascii(d,' A0'+d->reg0);
          desa_ascii(d,',USP');
          return;
        case 5:
          desa_ascii(d,'MOVE');
          desa_ascii(d,' USP');
          desa_ascii(d,',A0'+d->reg0);
          return;
        case 6: {
          /* FUNKY LINE 4 MODE 6 (4E */
          static char *str[8] = { /* $4E70 - $4E77 */
            /* 0      1      2      3     4    5      6      7 */
            "RESET","NOP","STOP ","RTE", "?","RTS","TRAPV","RTR"
          };

          if (d->reg0 == 4) {
            break;
          }
          if ((d->reg0 & 1)){
            d->status = DESA68_INST
              | ( (d->reg0 == 1)
                  ? DESA68_NOP
                  : DESA68_RTS);
          }

          desa_str(d, str[d->reg0]);
          if (d->reg0 == 2) {
            get_ea_2(d, MODE_IMM,0,'W');
          }
        } return;
        }
      }
    }
  }
  desa_dcw(d);
}


/**************
 *
 *   LINE 5 :
 *   -ADDQ
 *   -SUBQ
 *   -Scc
 *   -Dcc
 *
 ***************/

static void desa_addq_subq(desa68_parm_t * d)
{
  int v;

  v = d->reg9;
  v += (!v) << 3;
  v <<= 8;
  desa_ascii(d,(d->w&0400) ? 'SUBQ':'ADDQ');
  desa_ascii(d,d->szchar);
  desa_ascii(d,' #0,' + v);
  get_ea_2(d, d->mode3, d->reg0, d->szchar);
}

static void desa_Dcc(desa68_parm_t * d)
{
  desa_ascii(d,('DB'<<16) + dbcc_ascii[(d->w>>8)&15]);
  desa_ascii(d,' D0,' + (d->reg0<<8));
  get_ea_2(d, MODE_ABSW, MODE_dPC-MODE_ABSW, 0);
  d->s[-4] = 0; /* $$$ hack : remove (PC) at the end */
  d->branch = d->ea;
  d->status = DESA68_INST | DESA68_BSR;
}

static void desa_Scc(desa68_parm_t * d)
{
  desa_ascii(d, ('S'<<24) + (scc_ascii[(d->w>>8)&15]<<8) + ' ');
  get_ea_2(d, d->mode3, d->reg0, d->szchar);
}

static void desa_line5(desa68_parm_t * d)
{
  if ( (d->w&0370) == 0310 ) {
    desa_Dcc(d);
  } else if (d->opsz == 3) {
    desa_Scc(d);
  } else {
    desa_addq_subq(d);
  }
}

/**************
 *
 *   LINE 6 :
 *   -Bcc
 *
 * Format 0110 COND OFFSET08 [OFFSET16 if OFFSET08==0]
 * !!! COND=0001(false) => BSR
 ***************/

static void desa_line6(desa68_parm_t * d)
{
  s32 a;

  int cond = (d->w>>8) & 0xF;

  desa_ascii(d,('B'<<16) + bcc_ascii[cond]);

  /* Bcc.S */
  if (d->w & 255) {
    desa_ascii(d,'.S');
    a = (s32)(s8)d->w;
    a += (s32)d->pc;
  }
  /* Bcc.W */
  else {
    a = relPC(d);
  }
  desa_char(d,' ');
  desa_absL(d, a, -1);
  if (cond==0) {
    d->status = DESA68_INST|DESA68_BRA;
  } else {
    d->status = DESA68_INST|DESA68_BSR;
  }
  d->branch = a & d->memmsk;
}

/**************
 *
 *   LINE 7 :
 *   -MOVEQ
 *
 * Format : 01111 REG 0 XXXXXXXX
 *
 ***************/
static void desa_line7(desa68_parm_t * d)
{
  if (d->w & 0400) {
    desa_dcw(d);
  } else {
    desa_str(d,"MOVEQ #");
    desa_signifiant(d,(int)(s8)d->w);
    desa_ascii(d,',D0'+d->reg9);
  }
}


/**************
 *
 *   LINE B :
 *   -CMP
 *   -CMPM
 *   -EOR
 *
 ***************/

static int check_desa_cmpa(desa68_parm_t * d)
{
  int modemsk = 07777;

  if ( d->opsz != 3 || !(modemsk & (1<<d->adrmode0)) )
    return 0;

  d->szchar=(d->w&0400) ? '.L' : '.W';
  desa_ascii(d,'CMPA');
  desa_ascii(d,d->szchar);
  desa_char (d, ' ');
  get_ea_2  (d,d->mode3, d->reg0, d->szchar);
  desa_ascii(d,',A0'+d->reg9);
  return 1;
}

static int check_desa_eor_cmp(desa68_parm_t * d)
{
  int modemsk, inst;

  if (d->opsz == 3)
    return 0;

  /* EOR */
  else if (d->w & 0400) {
    modemsk = 00775;
    inst = 'EOR';
  }

  /* CMP */
  else {
    modemsk = (!d->opsz) ? 07775 : 07777;
    inst = 'CMP';
  }

  if ( ! (modemsk & (1<<d->adrmode0) ) )
    return 0;

  desa_dn_ae(d,inst);
  return 1;
}

static int check_desa_cmpm(desa68_parm_t * d)
{
  if ((d->w&0470) != 0410)
    return 0;
  desa_ascii(d,'CMPM');
  desa_ascii(d,' (A0' + d->reg0);
  desa_ascii(d,')+,(');
  desa_ascii(d,'A0)+' + (d->reg9<<16));
  return 1;
}

static void desa_lineB(desa68_parm_t * d)
{
  if (check_desa_cmpa(d))         return;
  else if (check_desa_eor_cmp(d)) return;
  else if (check_desa_cmpm(d))    return;
  else desa_dcw(d);
  return;
}


/**************
 *
 *   LINE 8 :
 *   -OR
 *   -SBCD
 *   -DIVU
 *
 *
 *   LINE C :
 *   -EXG
 *   -MULS,MULU
 *   -ABCD
 *   -AND
 *
 ***************/

static int check_desa_exg(desa68_parm_t * d)
{
  unsigned int reg;
  switch(d->w&0770) {
  case 0500:
    reg = 0x0000;
    break;
  case 0510:
    reg = 0x0808;
    break;
  case 0610:
    reg = 0x0008;
    break;
  default:
    return 0;
  }
  desa_ascii(d,'EXG ');
  desa_reg(d, d->reg9 + (reg>>8));
  desa_char (d, ',');
  desa_reg(d, d->reg0 + (reg&8));
  return 1;
}

static int check_desa_mul_div(desa68_parm_t * d)
{
  int modemsk = 0xFFD;
  if ( (d->w&0300) != 0300 || !( modemsk & (1<<d->adrmode0) ) )
    return 0;
  desa_ascii(d,(d->line==0xC) ? 'MUL' : 'DIV');
  desa_ascii(d,(d->w&0x100) ? 'S ' : 'U ');
  get_ea_2(d, d->mode3, d->reg0, 'W');
  desa_ascii(d,',D0'+d->reg9);
  return 1;
}

static int check_desa_abcd_sbcd(desa68_parm_t * d)
{
  if ( (d->w&0x1f0) != 0x100 )
    return 0;
  desa_ry_rx(d,(d->line==0xC) ? 'ABCD' : 'SBCD', 0) ;
  return 1;
}


static int check_desa_and_or(desa68_parm_t * d)
{
  int modemsk = !(d->w&0400)? 0xFFD : 0x1FC;
  if ( ! (modemsk & (1<<d->adrmode0) ) )
    return 0;
  desa_dn_ae(d, (d->line==0xC) ? 'AND' : 'OR');
  return 1;
}

static void desa_lin8C(desa68_parm_t * d)
{
  if (check_desa_abcd_sbcd(d))    return;
  else if (check_desa_mul_div(d)) return;
  else if (check_desa_exg(d))     return;
  else if (check_desa_and_or(d))  return;
  else desa_dcw(d);
  return;
}

/**************
 *
 *   LINE 9 :
 *   -SUB, SUBX, SUBA
 *
 *   LINE D :
 *   -ADD, ADDX, ADDA
 *
 **************/

static int check_desa_addx_subx(desa68_parm_t * d)
{
  if ( (d->w&0460) != 0400 )
    return 0;
  desa_ry_rx(d,(d->line==0xD) ? 'ADDX' : 'SUBX', d->szchar);
  return 1;
}

static int check_desa_adda_suba(desa68_parm_t * d)
{
  if (d->opsz != 3)
    return 0;

  d->szchar = (d->w&0400) ? '.L' : '.W';
  desa_ascii(d,(d->line==0xD) ? 'ADDA' : 'SUBA');
  desa_ascii(d,d->szchar);
  desa_char (d, ' ');
  get_ea_2(d, d->mode3, d->reg0, d->szchar);
  desa_ascii(d,',A0'+d->reg9);

  return 1;
}

static int check_desa_add_sub(desa68_parm_t * d)
{
  int modemsk = !(d->w&0400) ? 07777 : 00774;

  if (d->opsz == 0) /* Fordib An for byte access size */
    modemsk &= ~(1<<MODE_AN);

  if ( ! (modemsk & (1<<d->adrmode0) ) )
    return 0;
  desa_dn_ae (d,d->line==0xD ? 'ADD' : 'SUB');
  return 1;
}

static void desa_lin9D(desa68_parm_t * d)
{
  if (check_desa_adda_suba(d))           return;
  else if (check_desa_addx_subx(d))     return;
  else if (check_desa_add_sub(d))       return;
  else desa_dcw(d);
  return;
}

/**************
 *
 *   LINE E :
 *   -Shifting
 *
 * Format Reg: 1110 VAL D SZ I TY RG0
 * Format Mem: 1110 0TY D 11 MODRG0
 ***************/

static void desa_lineE(desa68_parm_t * d)
{
  static u16 shf_ascii[4] = { 'AS', 'LS', 'RO', 'RO' };

  /* Memory */
  if (d->opsz == 3) {
    const int modemsk = 00774;
    int type = d->reg9;
    if (!(modemsk & (1<<d->adrmode0)) || (type&4)) {
      desa_dcw(d);
      return;
    }
    desa_ascii(d,shf_ascii[type]);
    if (type==2) desa_char(d,'X');
    desa_char (d, (d->w&0400) ? 'L' :'R');
    desa_char (d, ' ');
    get_ea_2(d, d->mode3, d->reg0, 0);
  }

  /* Register */
  else {
    int type = (d->w>>3)&3;
    desa_ascii (d,shf_ascii[type]);
    if (type==2) desa_char(d,'X');
    desa_char (d, (d->w&0400) ? 'L' :'R');
    desa_ascii(d,d->szchar);
    /* dn,dn */
    if (d->w&(1<<5))
      desa_ascii(d,' D0'+d->reg9);
    /* #x,dn */
    else
      desa_ascii(d,' #1'+((d->reg9-1)&7));
    desa_ascii(d,',D0'+d->reg0);
  }
}

/**************
 *
 *   LINE A :
 *   -LineA
 *   LINE F :
 *   -LineF
 *
 * Format : LINE XXXX XXXX XXXX
 *
 ***************/

static void desa_linAF(desa68_parm_t * d)
{
  int vector = d->line - 0xA;

  desa_ascii(d,'LINE');
  desa_ascii(d,'A '+(vector<<8));
  desa_uhexacat(d,d->w, 3, '$');
  d->branch = 0x40 + ( (!!vector) <<2 );
}

static void (*desa_line[16])(desa68_parm_t *) =
{
  desa_line0, desa_li123, desa_li123, desa_li123,
  desa_line4, desa_line5, desa_line6, desa_line7,
  desa_lin8C, desa_lin9D, desa_linAF, desa_lineB,
  desa_lin8C, desa_lin9D, desa_lineE, desa_linAF,
};

void desa68(desa68_parm_t * d)
{
  char tmp[128];

  /* Reset. */
  d->ea     = -1;
  d->branch = -1;
  d->ea_src = -1;
  d->ea_dst = -1;
  d->status = DESA68_INST;  /* Assume valid instruction */

  d->s = d->str;
  if (!d->s) {
    d->s = tmp;
    d->strmax = sizeof(tmp);
  }

  d->s[0]   = 0;
  d->pc_org = d->pc &= d->memmsk;
  read_pc(d);

  d->reg0     = REG0(d->w);
  d->reg9     = REG9(d->w);
  d->mode3    = MODE3(d->w);
  d->mode6    = MODE6(d->w);
  d->line     = LINE(d->w);
  d->opsz     = OPSZ(d->w);
  d->adrmode0 = d->mode3 + ((d->mode3==MODE_ABSW)? d->reg0 : 0);
  d->adrmode6 = d->mode6 + ((d->mode6==MODE_ABSW)? d->reg9 : 0);
  d->szchar   = ('.'<<8) | (u8)('?LWB'>>(d->opsz*8));

  (desa_line[d->line])(d);
  desa_char(d,0);

  /* Restore current status to caller struct */
  d->pc     = d->pc & d->memmsk;
  d->status = d->status;
  d->branch = (d->branch == -1) ? -1 : (d->branch & d->memmsk);
  d->ea_src = (d->ea_src == -1) ? -1 : (d->ea_src & d->memmsk);
  d->ea_dst = (d->ea_dst == -1) ? -1 : (d->ea_dst & d->memmsk);
}

int desa68_version(void)
{
#ifndef PACKAGE_VERNUM
# define PACKAGE_VERNUM 0
#endif
  return PACKAGE_VERNUM;
}

const char * desa68_versionstr(void)
{
#ifndef PACKAGE_STRING
# define PACKAGE_STRING "desa68 n/a"
#endif
  return PACKAGE_STRING;
}
