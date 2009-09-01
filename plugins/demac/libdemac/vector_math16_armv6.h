/*

libdemac - A Monkey's Audio decoder

$Id: vector_math16_armv6.h 19198 2008-11-24 18:40:43Z amiconn $

Copyright (C) Dave Chapman 2007

ARMv6 vector math copyright (C) 2008 Jens Arnold

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
#if ORDER > 32
    int cnt = ORDER>>5;
#endif

#if ORDER > 16
#define ADD_SUB_BLOCKS "4"
#else
#define ADD_SUB_BLOCKS "2"
#endif

    asm volatile (
        "tst     %[v2], #2           \n"
        "beq     20f                 \n"

    "10:                             \n"
        "bic     %[v2], %[v2], #2    \n"
        "ldmia   %[v2]!, {r4-r5}     \n"
    "1:                              \n"
        ".rept " ADD_SUB_BLOCKS     "\n"
        "ldmia   %[v2]!, {r6-r7}     \n"
        "ldmia   %[v1],  {r0-r3}     \n"
        "mov     r5, r5, ror #16     \n"
        "pkhtb   r4, r5, r4, asr #16 \n"
        "sadd16  r0, r0, r4          \n"
        "pkhbt   r5, r5, r6, lsl #16 \n"
        "sadd16  r1, r1, r5          \n"
        "ldmia   %[v2]!, {r4-r5}     \n"
        "mov     r7, r7, ror #16     \n"
        "pkhtb   r6, r7, r6, asr #16 \n"
        "sadd16  r2, r2, r6          \n"
        "pkhbt   r7, r7, r4, lsl #16 \n"
        "sadd16  r3, r3, r7          \n"
        "stmia   %[v1]!, {r0-r3}     \n"
        ".endr                       \n"
#if ORDER > 32
        "subs    %[cnt], %[cnt], #1  \n"
        "bne     1b                  \n"
#endif
        "b       99f                 \n"

    "20:                             \n"
    "1:                              \n"
        ".rept " ADD_SUB_BLOCKS     "\n"
        "ldmia   %[v2]!, {r4-r7}     \n"
        "ldmia   %[v1],  {r0-r3}     \n"
        "sadd16  r0, r0, r4          \n"
        "sadd16  r1, r1, r5          \n"
        "sadd16  r2, r2, r6          \n"
        "sadd16  r3, r3, r7          \n"
        "stmia   %[v1]!, {r0-r3}     \n"
        ".endr                       \n"
#if ORDER > 32
        "subs    %[cnt], %[cnt], #1  \n"
        "bne     1b                  \n"
#endif

    "99:                             \n"
        : /* outputs */
#if ORDER > 32
        [cnt]"+r"(cnt),
#endif
        [v1] "+r"(v1),
        [v2] "+r"(v2)
        : /* inputs */
        : /* clobbers */
        "r0", "r1", "r2", "r3", "r4",
        "r5", "r6", "r7", "memory"
    );
}

/* This version fetches data as 32 bit words, and *requires* v1 to be
 * 32 bit aligned, otherwise it will result either in a data abort, or
 * incorrect results (if ARM aligncheck is disabled). */
static inline void vector_sub(int16_t* v1, int16_t* v2)
{
#if ORDER > 32
    int cnt = ORDER>>5;
#endif

    asm volatile (
        "tst     %[v2], #2           \n"
        "beq     20f                 \n"

    "10:                             \n"
        "bic     %[v2], %[v2], #2    \n"
        "ldmia   %[v2]!, {r4-r5}     \n"
    "1:                              \n"
        ".rept " ADD_SUB_BLOCKS     "\n"
        "ldmia   %[v2]!, {r6-r7}     \n"
        "ldmia   %[v1],  {r0-r3}     \n"
        "mov     r5, r5, ror #16     \n"
        "pkhtb   r4, r5, r4, asr #16 \n"
        "ssub16  r0, r0, r4          \n"
        "pkhbt   r5, r5, r6, lsl #16 \n"
        "ssub16  r1, r1, r5          \n"
        "ldmia   %[v2]!, {r4-r5}     \n"
        "mov     r7, r7, ror #16     \n"
        "pkhtb   r6, r7, r6, asr #16 \n"
        "ssub16  r2, r2, r6          \n"
        "pkhbt   r7, r7, r4, lsl #16 \n"
        "ssub16  r3, r3, r7          \n"
        "stmia   %[v1]!, {r0-r3}     \n"
        ".endr                       \n"
#if ORDER > 32
        "subs    %[cnt], %[cnt], #1  \n"
        "bne     1b                  \n"
#endif
        "b       99f                 \n"

    "20:                             \n"
    "1:                              \n"
        ".rept " ADD_SUB_BLOCKS     "\n"
        "ldmia   %[v2]!, {r4-r7}     \n"
        "ldmia   %[v1],  {r0-r3}     \n"
        "ssub16  r0, r0, r4          \n"
        "ssub16  r1, r1, r5          \n"
        "ssub16  r2, r2, r6          \n"
        "ssub16  r3, r3, r7          \n"
        "stmia   %[v1]!, {r0-r3}     \n"
        ".endr                       \n"
#if ORDER > 32
        "subs    %[cnt], %[cnt], #1  \n"
        "bne     1b                  \n"
#endif

    "99:                             \n"
        : /* outputs */
#if ORDER > 32
        [cnt]"+r"(cnt),
#endif
        [v1] "+r"(v1),
        [v2] "+r"(v2)
        : /* inputs */
        : /* clobbers */
        "r0", "r1", "r2", "r3", "r4",
        "r5", "r6", "r7", "memory"
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
        "bic     %[v2], %[v2], #2        \n"
        "ldmia   %[v2]!, {r5-r7}         \n"
        "ldmia   %[v1]!, {r0-r1}         \n"
    "1:                                  \n"
        "pkhbt   r8, r6, r5              \n"
        "ldmia   %[v2]!, {r4-r5}         \n"
#if ORDER > 32
        "smladx  %[res], r0, r8, %[res]  \n"
#else
        "smuadx  %[res], r0, r8          \n"
#endif
        ".rept " MLA_BLOCKS             "\n"
        "pkhbt   r8, r7, r6              \n"
        "ldmia   %[v1]!, {r2-r3}         \n"
        "smladx  %[res], r1, r8, %[res]  \n"
        "pkhbt   r8, r4, r7              \n"
        "ldmia   %[v2]!, {r6-r7}         \n"
        "smladx  %[res], r2, r8, %[res]  \n"
        "pkhbt   r8, r5, r4              \n"
        "ldmia   %[v1]!, {r0-r1}         \n"
        "smladx  %[res], r3, r8, %[res]  \n"
        "pkhbt   r8, r6, r5              \n"
        "ldmia   %[v2]!, {r4-r5}         \n"
        "smladx  %[res], r0, r8, %[res]  \n"
        ".endr                           \n"

        "pkhbt   r8, r7, r6              \n"
        "ldmia   %[v1]!, {r2-r3}         \n"
        "smladx  %[res], r1, r8, %[res]  \n"
        "pkhbt   r8, r4, r7              \n"
#if ORDER > 32
        "subs    %[cnt], %[cnt], #1      \n"
        "ldmneia %[v2]!, {r6-r7}         \n"
        "smladx  %[res], r2, r8, %[res]  \n"
        "pkhbt   r8, r5, r4              \n"
        "ldmneia %[v1]!, {r0-r1}         \n"
        "smladx  %[res], r3, r8, %[res]  \n"
        "bne     1b                      \n"
#else
        "pkhbt   r5, r5, r4              \n"
        "smladx  %[res], r2, r8, %[res]  \n"
        "smladx  %[res], r3, r5, %[res]  \n"
#endif
        "b       99f                     \n"

    "20:                                 \n"
        "ldmia   %[v1]!, {r0-r1}         \n"
        "ldmia   %[v2]!, {r5-r7}         \n"
    "1:                                  \n"
        "ldmia   %[v1]!, {r2-r3}         \n"
#if ORDER > 32
        "smlad   %[res], r0, r5, %[res]  \n"
#else
        "smuad   %[res], r0, r5          \n"
#endif
        ".rept " MLA_BLOCKS             "\n"
        "ldmia   %[v2]!, {r4-r5}         \n"
        "smlad   %[res], r1, r6, %[res]  \n"
        "ldmia   %[v1]!, {r0-r1}         \n"
        "smlad   %[res], r2, r7, %[res]  \n"
        "ldmia   %[v2]!, {r6-r7}         \n"
        "smlad   %[res], r3, r4, %[res]  \n"
        "ldmia   %[v1]!, {r2-r3}         \n"
        "smlad   %[res], r0, r5, %[res]  \n"
        ".endr                           \n"

        "ldmia   %[v2]!, {r4-r5}         \n"
        "smlad   %[res], r1, r6, %[res]  \n"
#if ORDER > 32
        "subs    %[cnt], %[cnt], #1      \n"
        "ldmneia %[v1]!, {r0-r1}         \n"
        "smlad   %[res], r2, r7, %[res]  \n"
        "ldmneia %[v2]!, {r6-r7}         \n"
        "smlad   %[res], r3, r4, %[res]  \n"
        "bne     1b                      \n"
#else
        "smlad   %[res], r2, r7, %[res]  \n"
        "smlad   %[res], r3, r4, %[res]  \n"
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
        "r0", "r1", "r2", "r3", "r4",
        "r5", "r6", "r7", "r8"
    );
    return res;
}
