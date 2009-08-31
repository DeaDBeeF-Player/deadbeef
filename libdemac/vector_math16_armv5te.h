/*

libdemac - A Monkey's Audio decoder

$Id: vector_math16_armv5te.h 19260 2008-11-28 23:50:22Z amiconn $

Copyright (C) Dave Chapman 2007

ARMv5te vector math copyright (C) 2008 Jens Arnold

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

/* This version fetches data as 32 bit words, and *requires* v1 to be
 * 32 bit aligned, otherwise it will result either in a data abort, or
 * incorrect results (if ARM aligncheck is disabled). */
static inline void vector_add(int16_t* v1, int16_t* v2)
{
#if ORDER > 16
    int cnt = ORDER>>4;
#endif

#define ADDHALFREGS(sum, s1)                             /* Adds register */    \
        "mov   " #s1  ", " #s1  ",   ror #16         \n" /* halves straight. */ \
        "add     r8    , " #s1  ", " #sum ", lsl #16 \n" /* Clobbers 's1' */    \
        "add   " #sum ", " #s1  ", " #sum ", lsr #16 \n" /* and r8. */          \
        "mov   " #sum ", " #sum ",   lsl #16         \n" \
        "orr   " #sum ", " #sum ",   r8    , lsr #16 \n"

#define ADDHALFXREGS(sum, s1, s2)                        /* Adds register */    \
        "add   " #s1  ", " #s1  ", " #sum ", lsl #16 \n" /* halves across. */ \
        "add   " #sum ", " #s2  ", " #sum ", lsr #16 \n" /* Clobbers 's1'. */ \
        "mov   " #sum ", " #sum ",   lsl #16         \n" \
        "orr   " #sum ", " #sum ", " #s1  ", lsr #16 \n"

    asm volatile (
        "tst     %[v2], #2           \n"
        "beq     20f                 \n"

    "10:                             \n"
        "ldrh    r4, [%[v2]], #2     \n"
        "mov     r4, r4, lsl #16     \n"
    "1:                              \n"
        "ldmia   %[v1],  {r0-r3}     \n"
        "ldmia   %[v2]!, {r5-r8}     \n"
        ADDHALFXREGS(r0, r4, r5)
        ADDHALFXREGS(r1, r5, r6)
        ADDHALFXREGS(r2, r6, r7)
        ADDHALFXREGS(r3, r7, r8)
        "stmia   %[v1]!, {r0-r3}     \n"
        "mov     r4, r8              \n"
        "ldmia   %[v1],  {r0-r3}     \n"
        "ldmia   %[v2]!, {r5-r8}     \n"
        ADDHALFXREGS(r0, r4, r5)
        ADDHALFXREGS(r1, r5, r6)
        ADDHALFXREGS(r2, r6, r7)
        ADDHALFXREGS(r3, r7, r8)
        "stmia   %[v1]!, {r0-r3}     \n"
#if ORDER > 16
        "mov     r4, r8              \n"
        "subs    %[cnt], %[cnt], #1  \n"
        "bne     1b                  \n"
#endif
        "b       99f                 \n"

    "20:                             \n"
    "1:                              \n"
        "ldmia   %[v1],  {r0-r3}     \n"
        "ldmia   %[v2]!, {r4-r7}     \n"
        ADDHALFREGS(r0, r4)
        ADDHALFREGS(r1, r5)
        ADDHALFREGS(r2, r6)
        ADDHALFREGS(r3, r7)
        "stmia   %[v1]!, {r0-r3}     \n"
        "ldmia   %[v1],  {r0-r3}     \n"
        "ldmia   %[v2]!, {r4-r7}     \n"
        ADDHALFREGS(r0, r4)
        ADDHALFREGS(r1, r5)
        ADDHALFREGS(r2, r6)
        ADDHALFREGS(r3, r7)
        "stmia   %[v1]!, {r0-r3}     \n"
#if ORDER > 16
        "subs    %[cnt], %[cnt], #1  \n"
        "bne     1b                  \n"
#endif

    "99:                             \n"
        : /* outputs */
#if ORDER > 16
        [cnt]"+r"(cnt),
#endif
        [v1] "+r"(v1),
        [v2] "+r"(v2)
        : /* inputs */
        : /* clobbers */
        "r0", "r1", "r2", "r3", "r4",
        "r5", "r6", "r7", "r8", "memory"
    );
}

/* This version fetches data as 32 bit words, and *requires* v1 to be
 * 32 bit aligned, otherwise it will result either in a data abort, or
 * incorrect results (if ARM aligncheck is disabled). */
static inline void vector_sub(int16_t* v1, int16_t* v2)
{
#if ORDER > 16
    int cnt = ORDER>>4;
#endif

#define SUBHALFREGS(dif, s1)                             /* Subtracts register */ \
        "sub     r8    , " #dif ", " #s1            "\n" /* halves straight. */   \
        "and     r8    ,   r8    ,   r9              \n" /* Needs r9 = 0x0000ffff, */ \
        "mov   " #dif ", " #dif ",   lsr #16         \n" /* clobbers r8. */      \
        "sub   " #dif ", " #dif ", " #s1  ", lsr #16 \n"  \
        "orr   " #dif ",   r8    , " #dif ", lsl #16 \n"

#define SUBHALFXREGS(dif, s1, s2)                        /* Subtracts register */ \
        "sub   " #s1  ", " #dif ", " #s1  ", lsr #16 \n" /* halves across. */     \
        "and   " #s1  ", " #s1  ",   r9              \n" /* Needs r9 = 0x0000ffff, */ \
        "rsb   " #dif ", " #s2  ", " #dif ", lsr #16 \n" /* clobbers 's1'. */     \
        "orr   " #dif ", " #s1  ", " #dif ", lsl #16 \n"
        
    asm volatile (
        "mov     r9, #0xff           \n"
        "orr     r9, r9, #0xff00     \n"
        "tst     %[v2], #2           \n"
        "beq     20f                 \n"

    "10:                             \n"
        "ldrh    r4, [%[v2]], #2     \n"
        "mov     r4, r4, lsl #16     \n"
    "1:                              \n"
        "ldmia   %[v1],  {r0-r3}     \n"
        "ldmia   %[v2]!, {r5-r8}     \n"
        SUBHALFXREGS(r0, r4, r5)
        SUBHALFXREGS(r1, r5, r6)
        SUBHALFXREGS(r2, r6, r7)
        SUBHALFXREGS(r3, r7, r8)
        "stmia   %[v1]!, {r0-r3}     \n"
        "mov     r4, r8              \n"
        "ldmia   %[v1],  {r0-r3}     \n"
        "ldmia   %[v2]!, {r5-r8}     \n"
        SUBHALFXREGS(r0, r4, r5)
        SUBHALFXREGS(r1, r5, r6)
        SUBHALFXREGS(r2, r6, r7)
        SUBHALFXREGS(r3, r7, r8)
        "stmia   %[v1]!, {r0-r3}     \n"
#if ORDER > 16
        "mov     r4, r8              \n"
        "subs    %[cnt], %[cnt], #1  \n"
        "bne     1b                  \n"
#endif
        "b       99f                 \n"

    "20:                             \n"
    "1:                              \n"
        "ldmia   %[v1],  {r0-r3}     \n"
        "ldmia   %[v2]!, {r4-r7}     \n"
        SUBHALFREGS(r0, r4)
        SUBHALFREGS(r1, r5)
        SUBHALFREGS(r2, r6)
        SUBHALFREGS(r3, r7)
        "stmia   %[v1]!, {r0-r3}     \n"
        "ldmia   %[v1],  {r0-r3}     \n"
        "ldmia   %[v2]!, {r4-r7}     \n"
        SUBHALFREGS(r0, r4)
        SUBHALFREGS(r1, r5)
        SUBHALFREGS(r2, r6)
        SUBHALFREGS(r3, r7)
        "stmia   %[v1]!, {r0-r3}     \n"
#if ORDER > 16
        "subs    %[cnt], %[cnt], #1  \n"
        "bne     1b                  \n"
#endif

    "99:                             \n"
        : /* outputs */
#if ORDER > 16
        [cnt]"+r"(cnt),
#endif
        [v1] "+r"(v1),
        [v2] "+r"(v2)
        : /* inputs */
        : /* clobbers */
        "r0", "r1", "r2", "r3", "r4", "r5", 
        "r6", "r7", "r8", "r9", "memory"
    );
}

/* This version fetches data as 32 bit words, and *requires* v1 to be
 * 32 bit aligned, otherwise it will result either in a data abort, or
 * incorrect results (if ARM aligncheck is disabled). */
static inline int32_t scalarproduct(int16_t* v1, int16_t* v2)
{
    int res;
#if ORDER > 32
    int cnt = ORDER>>5;
#endif

#if ORDER > 16
#define MLA_BLOCKS "3"
#else
#define MLA_BLOCKS "1"
#endif

    asm volatile (
#if ORDER > 32
        "mov     %[res], #0              \n"
#endif
        "tst     %[v2], #2               \n"
        "beq     20f                     \n"

    "10:                                 \n"
        "ldrh    r7, [%[v2]], #2         \n"
#if ORDER > 32
        "mov     r7, r7, lsl #16         \n"
    "1:                                  \n"
        "ldmia   %[v1]!, {r0-r3}         \n"
        "smlabt  %[res], r0, r7, %[res]  \n"
#else
        "ldmia   %[v1]!, {r0-r3}         \n"
        "smulbb  %[res], r0, r7          \n"
#endif
        "ldmia   %[v2]!, {r4-r7}         \n"
        "smlatb  %[res], r0, r4, %[res]  \n"
        "smlabt  %[res], r1, r4, %[res]  \n"
        "smlatb  %[res], r1, r5, %[res]  \n"
        "smlabt  %[res], r2, r5, %[res]  \n"
        "smlatb  %[res], r2, r6, %[res]  \n"
        "smlabt  %[res], r3, r6, %[res]  \n"
        "smlatb  %[res], r3, r7, %[res]  \n"
        
        ".rept " MLA_BLOCKS             "\n"
        "ldmia   %[v1]!, {r0-r3}         \n"
        "smlabt  %[res], r0, r7, %[res]  \n"
        "ldmia   %[v2]!, {r4-r7}         \n"
        "smlatb  %[res], r0, r4, %[res]  \n"
        "smlabt  %[res], r1, r4, %[res]  \n"
        "smlatb  %[res], r1, r5, %[res]  \n"
        "smlabt  %[res], r2, r5, %[res]  \n"
        "smlatb  %[res], r2, r6, %[res]  \n"
        "smlabt  %[res], r3, r6, %[res]  \n"
        "smlatb  %[res], r3, r7, %[res]  \n"
        ".endr                           \n"
#if ORDER > 32
        "subs    %[cnt], %[cnt], #1  \n"
        "bne     1b                  \n"
#endif
        "b       99f                 \n"

    "20:                                 \n"
    "1:                                  \n"
        "ldmia   %[v1]!, {r0-r3}         \n"
        "ldmia   %[v2]!, {r4-r7}         \n"
#if ORDER > 32
        "smlabb  %[res], r0, r4, %[res]  \n"
#else
        "smulbb  %[res], r0, r4          \n"
#endif
        "smlatt  %[res], r0, r4, %[res]  \n"
        "smlabb  %[res], r1, r5, %[res]  \n"
        "smlatt  %[res], r1, r5, %[res]  \n"
        "smlabb  %[res], r2, r6, %[res]  \n"
        "smlatt  %[res], r2, r6, %[res]  \n"
        "smlabb  %[res], r3, r7, %[res]  \n"
        "smlatt  %[res], r3, r7, %[res]  \n"

        ".rept " MLA_BLOCKS             "\n"
        "ldmia   %[v1]!, {r0-r3}         \n"
        "ldmia   %[v2]!, {r4-r7}         \n"
        "smlabb  %[res], r0, r4, %[res]  \n"
        "smlatt  %[res], r0, r4, %[res]  \n"
        "smlabb  %[res], r1, r5, %[res]  \n"
        "smlatt  %[res], r1, r5, %[res]  \n"
        "smlabb  %[res], r2, r6, %[res]  \n"
        "smlatt  %[res], r2, r6, %[res]  \n"
        "smlabb  %[res], r3, r7, %[res]  \n"
        "smlatt  %[res], r3, r7, %[res]  \n"
        ".endr                           \n"
#if ORDER > 32
        "subs    %[cnt], %[cnt], #1      \n"
        "bne     1b                      \n"  
#endif

    "99:                                 \n"
        : /* outputs */
#if ORDER > 32
        [cnt]"+r"(cnt),
#endif
        [v1] "+r"(v1),
        [v2] "+r"(v2),
        [res]"=r"(res)
        : /* inputs */
        : /* clobbers */
        "r0", "r1", "r2", "r3",
        "r4", "r5", "r6", "r7"
    );
    return res;
}
