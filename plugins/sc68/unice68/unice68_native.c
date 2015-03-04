/*
 *                 unice68 - ice depacker library (native version)
 *                    Copyright (C) 2003 Benjamin Gerard
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#ifdef TYPE_U8
typedef TYPE_U8 u8
#else
typedef unsigned char u8;
#endif

#ifdef TYPE_S8
typedef TYPE_S8 s8
#else
typedef signed char s8;
#endif

#ifdef TYPE_S16
typedef TYPE_S16 s16
#else
typedef signed short s16;
#endif

typedef u8 * areg_t;
typedef int dreg_t;

areg_t a0,a1,a2,a3,a4,a5,a6,a7;
dreg_t d0,d1,d2,d3,d4,d5,d6,d7;

#define ICE_MAGIC 0x49434521 /* 'ICE!' */

#define B_CC(CC, LABEL) if (CC) { goto LABEL; } else
#define DB_CC(CC, REG, LABEL) if (!(CC) && --REG >= 0) {goto LABEL;} else
#define DBF(REG,LABEL) DB_CC(0, REG, LABEL)
#define GET_1_BIT_BCC(LABEL) B_CC(get_1_bit()==0, LABEL)

static int direkt_tab[] = {
  0x7fff000e,0x00ff0007,0x00070002,0x00030001,0x00030001,
  270-1,	15-1,	 8-1,	 5-1,	 2-1
};

static u8 length_tab[] = {
  9,1,0,-1,-1,
  8,4,2,1,0
};

static s8 more_offset[] = {
  11,   4,   7,  0,
  0x01,0x1f, -1,-1, 0x00,0x1F
};

static int more_offset_word[] = {
  0x0b04,0x0700,0x011f,-1,0x001f
};

#if 0
static void dump_regs(const char * label)
{
  if (label) {
    fprintf(stderr, "%s:\n", label);
  }
#define DUMP(N) fprintf(stderr,"d%d=%08X a%d=%p\n",N,d##N,N,a##N)
  DUMP(0);
  DUMP(1);
  DUMP(2);
  DUMP(3);
  DUMP(4);
  DUMP(5);
  DUMP(6);
  DUMP(7);
}
#endif


static void strings(void);
static void normal_bytes(void);
static int get_d0_bits(int d0);

/* getinfo:	moveq	#3,d1 */
/* getbytes: lsl.l	#8,d0 */
/* 	move.b	(a0)+,d0 */
/* 	dbf	d1,getbytes */
/* 	rts */
static int getinfo(void)
{
  int d0 =
    (0[a0] << 24)
    + (1[a0] << 16)
    + (2[a0] <<  8)
    + 3[a0];

  a0 += 4;

  return d0;
}

/* get_1_bit: */
/* 	add.b	d7,d7 */
/* 	bne.s	bitfound */
/* 	move.b	-(a5),d7 */
/* 	addx.b	d7,d7 */
/* bitfound: */
/* 	rts */

static int get_1_bit(void)
{
  int r;

  r = (d7 & 255) << 1;
  B_CC(r & 255, bitfound);
  r = (r>>8) + (*(--a5) << 1);
 bitfound:
  d7 = (d7 & ~0xFF) | (r & 0xFF);
  return r >> 8;
}


/* ice_decrunch: */
/* ; a0 = Pointer to packed data */
/* ; a1 = Address to which the data is unpacked */

static int ice_decrunch(void)
{
  int id;
  int csize;
  int dsize;

/* 	movem.l	d0-a6,-(a7) */
/* 	bsr.s	getinfo */
/* 	cmpi.l	#'ICE!',d0 */
/* 	bne	not_packed */
/* 	bsr.s	getinfo	 */
/* 	lea.l	-8(a0,d0.l),a5 */
/* 	bsr.s	getinfo */
/* 	move.l	d0,(a7) */
/* 	move.l	a1,a4 */
/* 	move.l	a1,a6 */
/* 	adda.l	d0,a6 */
/* 	move.l	a6,a3 */
/* 	move.b	-(a5),d7 */
/* 	bsr	normal_bytes */

  id = getinfo();
  if (id != ICE_MAGIC) {
    return -1;
  }
  csize = getinfo();
  a5 = a0 - 8 + csize;
  d0 = dsize = getinfo();

  a6 = a4 = a1;
  a6 += d0;

  a3 = a6;

/*   dump_regs("init"); */

  d7 = *(--a5);

  normal_bytes();
/*   dump_regs("out of normal_bytes"); */

/* 	move.l	a3,a6 */
/* 	bsr	get_1_bit */
/* 	bcc.s	not_packed */
/* 	move.w	#$0f9f,d7 */
/* 	bsr	get_1_bit */
/* 	bcc.s	ice_00 */
/* 	moveq	#15,d0	 */
/* 	bsr	get_d0_bits */
/* 	move.w	d1,d7 */

  a6 = a3;
  GET_1_BIT_BCC(not_packed);
  d7 = 0x0f9f;
  GET_1_BIT_BCC(ice_00);
  d7 = d1 = get_d0_bits(15);
  d0 = 0xFFFF;

/* ice_00:	moveq	#3,d6 */
/* ice_01:	move.w	-(a3),d4 */
/* 	moveq	#3,d5 */
/* ice_02:	add.w	d4,d4 */
/* 	addx.w	d0,d0 */
/* 	add.w	d4,d4 */
/* 	addx.w	d1,d1 */
/* 	add.w	d4,d4 */
/* 	addx.w	d2,d2 */
/* 	add.w	d4,d4 */
/* 	addx.w	d3,d3 */
/* 	dbra	d5,ice_02 */
/* 	dbra	d6,ice_01 */
/* 	movem.w	d0-d3,(a3) */
/* 	dbra	d7,ice_00 */

ice_00:
  d6 = 3;
ice_01:
  a3 -= 2;
  d4 = (a3[0]<<8) | a3[1];
  d5 = 3;
ice_02:
  d4 += d4;
  d0 += d0 + (d4>>16);
  d4 &= 0xFFFF;
  
  d4 += d4;
  d1 += d1 + (d4>>16);
  d4 &= 0xFFFF;

  d4 += d4;
  d2 += d2 + (d4>>16);
  d4 &= 0xFFFF;

  d4 += d4;
  d3 += d3 + (d4>>16);
  d4 &= 0xFFFF;

  DBF(d5,ice_02);
  DBF(d6,ice_01);

  0[a3] = d0 >> 8;
  1[a3] = d0;

  2[a3] = d1 >> 8;
  3[a3] = d1;

  4[a3] = d2 >> 8;
  5[a3] = d2;

  6[a3] = d3 >> 8;
  7[a3] = d3;

  DBF(d7,ice_00);

not_packed:
  return 0;
}

static void normal_bytes(void)
{
/* normal_bytes:	 */
/* 	bsr.s	get_1_bit */
/* 	bcc.s	test_if_end */
/* 	moveq.l	#0,d1 */
/* 	bsr.s	get_1_bit */
/* 	bcc.s	copy_direkt */
/* 	lea.l	direkt_tab+20(pc),a1 */
/* 	moveq.l	#4,d3 */
/* nextgb:	move.l	-(a1),d0 */
/* 	bsr.s	get_d0bits */
/* 	swap.w	d0 */
/* 	cmp.w	d0,d1 */
/* 	dbne	d3,nextgb */
/* no_more:	add.l	20(a1),d1 */
/* copy_direkt:	 */
/* 	move.b	-(a5),-(a6) */
/* 	dbf	d1,copy_direkt */
/* test_if_end:	 */
/* 	cmpa.l	a4,a6 */
/* 	bgt.s	strings */
/* 	rts	 */


  while (1) {
    int * tab;

    GET_1_BIT_BCC(test_if_end);
    d1 = 0;
    GET_1_BIT_BCC(copy_direkt);

    tab = direkt_tab + (20>>2);
    d3 = 4;
  nextgb:
    d0 = * (--tab);
    d1 = get_d0_bits(d0);
    d0 = (d0 >> 16) | ~0xFFFF;
    DB_CC((d0^d1)&0xFFFF, d3, nextgb);
/*   no_more: */
    d1 += tab[(20>>2)];

  copy_direkt:
    *(--a6) = *(--a5);
    DBF(d1,copy_direkt);

  test_if_end:
/*     a6-a4 > 0; */
/*     a6 > a4; */
/*     !(a6 > a4) == (a6 <= a4) */

    if (a6 <= a4) {
      break;
    }
    strings();
  }
}



/* get_d0_bits: */
/* 	moveq.l	#0,d1 */
/* hole_bit_loop:	 */
/* 	add.b	d7,d7 */
/* 	bne.s	on_d0 */
/* 	move.b	-(a5),d7 */
/* 	addx.b	d7,d7 */
/* on_d0:	addx.w	d1,d1 */
/* 	dbf	d0,hole_bit_loop */
/* 	rts	 */

static int get_d0_bits(int d0)
{
  int r7 = d7;
  int r1 = 0;

  d0 &= 0xFFFF;
 hole_bit_loop:	
  r7 = (r7 & 255) << 1;
  B_CC(r7 & 255, on_d0);
  r7 = (*(--a5) * 2) + (r7>>8);
 on_d0:
  r1 += r1 + (r7>>8);
  DBF(d0,hole_bit_loop);
  d7 = (d7 &~0xFF) | (r7&0xFF);
  return r1;
}

static void strings(void) 
{
/* strings: */
/* 	lea.l	length_tab(pc),a1 */
/* 	moveq.l	#3,d2 */
/* get_length_bit:	 */
/* 	bsr.s	get_1_bit */
/* 	dbcc	d2,get_length_bit */
/* no_length_bit:	 */
/* 	moveq.l	#0,d4 */
/* 	moveq.l	#0,d1 */
/* 	move.b	1(a1,d2.w),d0 */
/* 	ext.w	d0 */
/* 	bmi.s	no_Ober */
/* get_Ober: bsr.s	get_d0_bits */
/* no_Ober:	move.b	6(a1,d2.w),d4 */
/* 	add.w	d1,d4 */
/* 	beq.s	get_offset_2 */

  a1 = length_tab;
  d2 = 3;
get_length_bit:	
  DB_CC(get_1_bit()==0, d2, get_length_bit);
/* no_length_bit:	 */
  d4 = d1 = 0; /* $$$ d4 is not needed here. */
  d0 = (d0 & ~0xFFFF) | (0xFFFF & (s8)a1[ 1 + (s16)d2 ]);
  B_CC(d0&0x8000, no_Ober);
/*  get_Ober: */
  d1 = get_d0_bits(d0);
  d0 |= 0xFFFF;
 no_Ober:
  d4 = a1 [ 6 + (s16)d2 ];
  d4 += d1;
  B_CC(d4==0,get_offset_2);
  
/* 	lea.l	more_offset(pc),a1 */
/* 	moveq.l	#1,d2 */
/* getoffs:	bsr.s	get_1_bit */
/* 	dbcc	d2,getoffs */
/* 	moveq.l	#0,d1 */
/* 	move.b	1(a1,d2.w),d0 */
/* 	ext.w	d0 */
/* 	bsr.s	get_d0_bits */
/* 	add.w	d2,d2 */
/* 	add.w	6(a1,d2.w),d1 */
/* 	bpl.s	depack_bytes */
/* 	sub.w	d4,d1 */
/* 	bra.s	depack_bytes */

  a1 = more_offset;
  d2 = 1;
 getoffs:
  DB_CC(get_1_bit()==0,d2,getoffs);

  d1 = get_d0_bits((int)(s8)more_offset[1+(s16)d2]);
  d0 |= 0xFFFF;
  d1 += more_offset_word[3+(s16)d2];
  if (d1 < 0) {
    d1 -= d4;
  }
  goto depack_bytes;
  
  
  /* get_offset_2:	 */
  /* 	moveq.l	#0,d1 */
  /* 	moveq.l	#5,d0 */
  /* 	moveq.l	#-1,d2 */
  /* 	bsr.s	get_1_bit */
  /* 	bcc.s	less_40 */
  /* 	moveq.l	#8,d0 */
  /* 	moveq.l	#$3f,d2 */
  /* less_40:	bsr.s	get_d0_bits */
  /* 	add.w	d2,d1 */
  
 get_offset_2:	
  d1 = 0;
  d0 = 5;
  d2 = -1;
  GET_1_BIT_BCC(less_40);
  d0 = 8;
  d2 = 0x3f;
 less_40:
  d1 = get_d0_bits(d0);
  d0 |= 0xFFFF;
  d1 += d2;
  
  /* depack_bytes: */
  /* 	lea.l	2(a6,d4.w),a1 */
  /* 	adda.w	d1,a1 */
  /* 	move.b	-(a1),-(a6) */
  /* dep_b:	move.b	-(a1),-(a6) */
  /* 	dbf	d4,dep_b */
  /* 	bra	normal_bytes */
  
 depack_bytes:
  a1 = a6 + 2 + (s16)d4 + (s16)d1;
  *(--a6) = *(--a1);
 dep_b:
  *(--a6) = *(--a1);
  DBF(d4,dep_b);
}


int unice68_get_depacked_size(const void * buffer, int * p_csize)
{
  int id, csize, dsize;
  int csize_verif = p_csize ? *p_csize : 0;

  if (csize_verif && csize_verif<12) {
    return -1;
  }

  a0 = (areg_t)buffer;
  id = getinfo();
  if (id != ICE_MAGIC) {
    return -1;
  }
  csize = getinfo();
  if (csize < 12) {
    return -2;
  }
  csize -= 12;
  dsize = getinfo();
  if (p_csize) {
    *p_csize = csize;
  }
  if (csize_verif && (csize != csize_verif)) {
    dsize = ~dsize;
  }
  return dsize;
}

int unice68_depacker(void * dest, const void * src)
{
  d0 = d1 = d2 = d3 = d4 = d5 = d6 = d7 = 0;
  a0 = (areg_t)src;
  a1 = dest;
  a2 = a3 = a4 = a5 = a6 = a7 = 0;

  return ice_decrunch();
}

int unice68_ice_version(void)
{
  return 240;
}
