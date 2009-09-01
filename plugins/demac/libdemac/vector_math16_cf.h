/*

libdemac - A Monkey's Audio decoder

$Id: vector_math16_cf.h 19144 2008-11-19 21:31:33Z amiconn $

Copyright (C) Dave Chapman 2007

Coldfire vector math copyright (C) 2007 Jens Arnold

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110, USA

*/

/* This version fetches data as 32 bit words, and *recommends* v1 to be
 * 32 bit aligned, otherwise performance will suffer. */
static inline void vector_add(int16_t* v1, int16_t* v2)
{
#if ORDER > 16
    int cnt = ORDER>>4;
#endif

#define ADDHALFREGS(s1, sum)           /* Add register halves straight. */ \
        "move.l " #s1  ",   %%d4  \n"  /* 's1' can be an A or D reg. */    \
        "add.l  " #sum ", " #s1  "\n"  /* 'sum' must be a D reg. */        \
        "clr.w    %%d4            \n"  /* 's1' and %%d4 are clobbered! */  \
        "add.l    %%d4  , " #sum "\n"  \
        "move.w " #s1  ", " #sum "\n"
        
#define ADDHALFXREGS(s1, s2, sum)      /* Add register halves across. */    \
        "clr.w  " #sum "          \n"  /* Needs 'sum' pre-swapped, swaps */ \
        "add.l  " #s1  ", " #sum "\n"  /* 's2', and clobbers 's1'. */       \
        "swap   " #s2  "          \n"  /* 's1' can be an A or D reg. */     \
        "add.l  " #s2  ", " #s1  "\n"  /* 'sum' and 's2' must be D regs. */ \
        "move.w " #s1  ", " #sum "\n"

    asm volatile (
        "move.l  %[v2], %%d0         \n"
        "and.l   #2, %%d0            \n"
        "jeq     20f                 \n"
        
    "10:                             \n"
        "move.w  (%[v2])+, %%d0      \n"
        "swap    %%d0                \n"
    "1:                              \n"
        "movem.l (%[v1]), %%a0-%%a3  \n"
        "movem.l (%[v2]), %%d1-%%d4  \n"
        ADDHALFXREGS(%%a0, %%d1, %%d0)
        "move.l  %%d0, (%[v1])+      \n"
        ADDHALFXREGS(%%a1, %%d2, %%d1)
        "move.l  %%d1, (%[v1])+      \n"
        ADDHALFXREGS(%%a2, %%d3, %%d2)
        "move.l  %%d2, (%[v1])+      \n"
        ADDHALFXREGS(%%a3, %%d4, %%d3)
        "move.l  %%d3, (%[v1])+      \n"
        "lea.l   (16, %[v2]), %[v2]  \n"
        "move.l  %%d4, %%d0          \n"

        "movem.l (%[v1]), %%a0-%%a3  \n"
        "movem.l (%[v2]), %%d1-%%d4  \n"
        ADDHALFXREGS(%%a0, %%d1, %%d0)
        "move.l  %%d0, (%[v1])+      \n"
        ADDHALFXREGS(%%a1, %%d2, %%d1)
        "move.l  %%d1, (%[v1])+      \n"
        ADDHALFXREGS(%%a2, %%d3, %%d2)
        "move.l  %%d2, (%[v1])+      \n"
        ADDHALFXREGS(%%a3, %%d4, %%d3)
        "move.l  %%d3, (%[v1])+      \n"
#if ORDER > 16
        "lea.l   (16, %[v2]), %[v2]  \n"
        "move.l  %%d4, %%d0          \n"

        "subq.l  #1, %[cnt]          \n"
        "jne     1b                  \n"
#endif
        "jra     99f                 \n"

    "20:                             \n"
    "1:                              \n"
        "movem.l (%[v2]), %%a0-%%a3  \n"
        "movem.l (%[v1]), %%d0-%%d3  \n"
        ADDHALFREGS(%%a0, %%d0)
        "move.l  %%d0, (%[v1])+      \n"
        ADDHALFREGS(%%a1, %%d1)
        "move.l  %%d1, (%[v1])+      \n"
        ADDHALFREGS(%%a2, %%d2)
        "move.l  %%d2, (%[v1])+      \n"
        ADDHALFREGS(%%a3, %%d3)
        "move.l  %%d3, (%[v1])+      \n"
        "lea.l   (16, %[v2]), %[v2]  \n"

        "movem.l (%[v2]), %%a0-%%a3  \n"
        "movem.l (%[v1]), %%d0-%%d3  \n"
        ADDHALFREGS(%%a0, %%d0)
        "move.l  %%d0, (%[v1])+      \n"
        ADDHALFREGS(%%a1, %%d1)
        "move.l  %%d1, (%[v1])+      \n"
        ADDHALFREGS(%%a2, %%d2)
        "move.l  %%d2, (%[v1])+      \n"
        ADDHALFREGS(%%a3, %%d3)
        "move.l  %%d3, (%[v1])+      \n"
#if ORDER > 16
        "lea.l   (16, %[v2]), %[v2]  \n"

        "subq.l  #1, %[cnt]          \n"
        "jne     1b                  \n"
#endif
    "99:                             \n"
        : /* outputs */
#if ORDER > 16
        [cnt]"+d"(cnt),
#endif
        [v1] "+a"(v1),
        [v2] "+a"(v2)
        : /* inputs */
        : /* clobbers */
        "d0", "d1", "d2", "d3", "d4",
        "a0", "a1", "a2", "a3", "memory"
    );
}

/* This version fetches data as 32 bit words, and *recommends* v1 to be
 * 32 bit aligned, otherwise performance will suffer. */
static inline void vector_sub(int16_t* v1, int16_t* v2)
{
#if ORDER > 16
    int cnt = ORDER>>4;
#endif

#define SUBHALFREGS(min, sub, dif)    /* Subtract register halves straight. */ \
        "move.l " #min ", " #dif "\n" /* 'min' can be an A or D reg */         \
        "sub.l  " #sub ", " #min "\n" /* 'sub' and 'dif' must be D regs */     \
        "clr.w  " #sub           "\n" /* 'min' and 'sub' are clobbered! */     \
        "sub.l  " #sub ", " #dif "\n" \
        "move.w " #min ", " #dif "\n" 
        
#define SUBHALFXREGS(min, s2, s1d)    /* Subtract register halves across. */ \
        "clr.w  " #s1d           "\n" /* Needs 's1d' pre-swapped, swaps */   \
        "sub.l  " #s1d ", " #min "\n" /* 's2' and clobbers 'min'. */         \
        "move.l " #min ", " #s1d "\n" /* 'min' can be an A or D reg, */      \
        "swap   " #s2            "\n" /* 's2' and 's1d' must be D regs. */   \
        "sub.l  " #s2  ", " #min "\n" \
        "move.w " #min ", " #s1d "\n"

    asm volatile (
        "move.l  %[v2], %%d0         \n"
        "and.l   #2, %%d0            \n"
        "jeq     20f                 \n"
        
    "10:                             \n"
        "move.w  (%[v2])+, %%d0      \n"
        "swap    %%d0                \n"
    "1:                              \n"
        "movem.l (%[v2]), %%d1-%%d4  \n"
        "movem.l (%[v1]), %%a0-%%a3  \n"
        SUBHALFXREGS(%%a0, %%d1, %%d0)
        "move.l  %%d0, (%[v1])+      \n"
        SUBHALFXREGS(%%a1, %%d2, %%d1)
        "move.l  %%d1, (%[v1])+      \n"
        SUBHALFXREGS(%%a2, %%d3, %%d2)
        "move.l  %%d2, (%[v1])+      \n"
        SUBHALFXREGS(%%a3, %%d4, %%d3)
        "move.l  %%d3, (%[v1])+      \n"
        "lea.l   (16, %[v2]), %[v2]  \n"
        "move.l  %%d4, %%d0          \n"

        "movem.l (%[v2]), %%d1-%%d4  \n"
        "movem.l (%[v1]), %%a0-%%a3  \n"
        SUBHALFXREGS(%%a0, %%d1, %%d0)
        "move.l  %%d0, (%[v1])+      \n"
        SUBHALFXREGS(%%a1, %%d2, %%d1)
        "move.l  %%d1, (%[v1])+      \n"
        SUBHALFXREGS(%%a2, %%d3, %%d2)
        "move.l  %%d2, (%[v1])+      \n"
        SUBHALFXREGS(%%a3, %%d4, %%d3)
        "move.l  %%d3, (%[v1])+      \n"
#if ORDER > 16
        "lea.l   (16, %[v2]), %[v2]  \n"
        "move.l  %%d4, %%d0          \n"
        
        "subq.l  #1, %[cnt]          \n"
        "bne.w   1b                  \n"
#endif
        "jra     99f                 \n"

    "20:                             \n"
    "1:                              \n"
        "movem.l (%[v2]), %%d1-%%d4  \n"
        "movem.l (%[v1]), %%a0-%%a3  \n"
        SUBHALFREGS(%%a0, %%d1, %%d0)
        "move.l  %%d0, (%[v1])+      \n"
        SUBHALFREGS(%%a1, %%d2, %%d1)
        "move.l  %%d1, (%[v1])+      \n"
        SUBHALFREGS(%%a2, %%d3, %%d2)
        "move.l  %%d2, (%[v1])+      \n"
        SUBHALFREGS(%%a3, %%d4, %%d3)
        "move.l  %%d3, (%[v1])+      \n"
        "lea.l   (16, %[v2]), %[v2]  \n"

        "movem.l (%[v2]), %%d1-%%d4  \n"
        "movem.l (%[v1]), %%a0-%%a3  \n"
        SUBHALFREGS(%%a0, %%d1, %%d0)
        "move.l  %%d0, (%[v1])+      \n"
        SUBHALFREGS(%%a1, %%d2, %%d1)
        "move.l  %%d1, (%[v1])+      \n"
        SUBHALFREGS(%%a2, %%d3, %%d2)
        "move.l  %%d2, (%[v1])+      \n"
        SUBHALFREGS(%%a3, %%d4, %%d3)
        "move.l  %%d3, (%[v1])+      \n"
#if ORDER > 16
        "lea.l   (16, %[v2]), %[v2]  \n"

        "subq.l  #1, %[cnt]          \n"
        "bne.w   1b                  \n"
#endif

    "99:                             \n"
        : /* outputs */
#if ORDER > 16
        [cnt]"+d"(cnt),
#endif
        [v1] "+a"(v1),
        [v2] "+a"(v2)
        : /* inputs */
        : /* clobbers */
        "d0", "d1", "d2", "d3", "d4",
        "a0", "a1", "a2", "a3", "memory"
    );
}

#define PREPARE_SCALARPRODUCT coldfire_set_macsr(0); /* signed integer mode */

/* This version fetches data as 32 bit words, and *recommends* v1 to be
 * 32 bit aligned, otherwise performance will suffer. It also needs EMAC
 * in signed integer mode - call above macro before use. */
static inline int32_t scalarproduct(int16_t* v1, int16_t* v2)
{
    int res;
#if ORDER > 32
    int cnt = ORDER>>5;
#endif

#if ORDER > 16
#define MAC_BLOCKS "7"
#else
#define MAC_BLOCKS "3"
#endif

    asm volatile (
        "move.l  %[v2], %%d0                         \n"
        "and.l   #2, %%d0                            \n"
        "jeq     20f                                 \n"

    "10:                                             \n"
        "move.l  (%[v1])+, %%d0                      \n"
        "move.w  (%[v2])+, %%d1                      \n"
    "1:                                              \n"
        ".rept " MAC_BLOCKS                         "\n"
        "mac.w   %%d0u, %%d1l, (%[v2])+, %%d1, %%acc0\n"
        "mac.w   %%d0l, %%d1u, (%[v1])+, %%d0, %%acc0\n"
        "mac.w   %%d0u, %%d1l, (%[v2])+, %%d1, %%acc0\n"
        "mac.w   %%d0l, %%d1u, (%[v1])+, %%d0, %%acc0\n"
        ".endr                                       \n"

        "mac.w   %%d0u, %%d1l, (%[v2])+, %%d1, %%acc0\n"
        "mac.w   %%d0l, %%d1u, (%[v1])+, %%d0, %%acc0\n"
        "mac.w   %%d0u, %%d1l, (%[v2])+, %%d1, %%acc0\n"
#if ORDER > 32
        "mac.w   %%d0l, %%d1u, (%[v1])+, %%d0, %%acc0\n"
        "subq.l  #1, %[res]                          \n"
        "bne.w   1b                                  \n"
#else
        "mac.w   %%d0l, %%d1u, %%acc0                \n"
#endif
        "jra     99f                                  \n"
        
    "20:                                             \n"
        "move.l  (%[v1])+, %%d0                      \n"
        "move.l  (%[v2])+, %%d1                      \n"
    "1:                                              \n"
        ".rept " MAC_BLOCKS                         "\n"
        "mac.w   %%d0u, %%d1u, (%[v1])+, %%d2, %%acc0\n"
        "mac.w   %%d0l, %%d1l, (%[v2])+, %%d1, %%acc0\n"
        "mac.w   %%d2u, %%d1u, (%[v1])+, %%d0, %%acc0\n"
        "mac.w   %%d2l, %%d1l, (%[v2])+, %%d1, %%acc0\n"
        ".endr                                       \n"

        "mac.w   %%d0u, %%d1u, (%[v1])+, %%d2, %%acc0\n"
        "mac.w   %%d0l, %%d1l, (%[v2])+, %%d1, %%acc0\n"
#if ORDER > 32
        "mac.w   %%d2u, %%d1u, (%[v1])+, %%d0, %%acc0\n"
        "mac.w   %%d2l, %%d1l, (%[v2])+, %%d1, %%acc0\n"
        "subq.l  #1, %[res]                          \n"
        "bne.w   1b                                  \n"
#else
        "mac.w   %%d2u, %%d1u, %%acc0                \n"
        "mac.w   %%d2l, %%d1l, %%acc0                \n"
#endif

    "99:                                             \n"
        "movclr.l %%acc0, %[res]                     \n"
        : /* outputs */
        [v1]"+a"(v1),
        [v2]"+a"(v2),
        [res]"=d"(res)
        : /* inputs */
#if ORDER > 32
        [cnt]"[res]"(cnt)
#endif
        : /* clobbers */
        "d0", "d1", "d2"
    );
    return res;
}
