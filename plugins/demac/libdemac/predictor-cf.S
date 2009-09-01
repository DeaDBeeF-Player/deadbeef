/*

libdemac - A Monkey's Audio decoder

$Id: predictor-cf.S 19296 2008-12-02 02:26:04Z amiconn $

Copyright (C) Dave Chapman 2007

Coldfire predictor copyright (C) 2007 Jens Arnold

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
#include "demac_config.h"

/* NOTE: The following need to be kept in sync with parser.h */

#define YDELAYA        200
#define YDELAYB        168
#define XDELAYA        136
#define XDELAYB        104
#define YADAPTCOEFFSA   72
#define XADAPTCOEFFSA   56
#define YADAPTCOEFFSB   40
#define XADAPTCOEFFSB   20

/* struct predictor_t members: */
#define buf              0    /* int32_t* buf */

#define YlastA           4    /* int32_t YlastA; */
#define XlastA           8    /* int32_t XlastA; */

#define YfilterB        12    /* int32_t YfilterB; */
#define XfilterA        16    /* int32_t XfilterA; */

#define XfilterB        20    /* int32_t XfilterB; */
#define YfilterA        24    /* int32_t YfilterA; */

#define YcoeffsA        28    /* int32_t YcoeffsA[4]; */
#define XcoeffsA        44    /* int32_t XcoeffsA[4]; */
#define YcoeffsB        60    /* int32_t YcoeffsB[5]; */
#define XcoeffsB        80    /* int32_t XcoeffsB[5]; */

#define historybuffer  100    /* int32_t historybuffer[] */


    .text

    .align  2

    .global predictor_decode_stereo
    .type   predictor_decode_stereo,@function

| void predictor_decode_stereo(struct predictor_t* p,
|                              int32_t* decoded0,
|                              int32_t* decoded1,
|                              int count)

predictor_decode_stereo:
    lea.l   (-12*4,%sp), %sp
    movem.l %d2-%d7/%a2-%a6, (4,%sp)

    movem.l (12*4+8,%sp), %a3-%a5       | %a3 = decoded0
                                        | %a4 = decoded1
    move.l  %a5, (%sp)                  | (%sp) = count

    move.l  #0, %macsr                  | signed integer mode
    move.l  (12*4+4,%sp), %a6           | %a6 = p
    move.l  (%a6), %a5                  | %a5 = p->buf
    
.loop:
    
    | ***** PREDICTOR Y *****
    
    | Predictor Y, Filter A
    
    move.l  (YlastA,%a6), %d3           | %d3  = p->YlastA

    movem.l (YDELAYA-12,%a5), %d0-%d2   | %d0  = p->buf[YDELAYA-3]
                                        | %d1  = p->buf[YDELAYA-2]
                                        | %d2  = p->buf[YDELAYA-1]

    move.l  %d3, (YDELAYA,%a5)          | p->buf[YDELAYA]  = %d3

    sub.l   %d3, %d2
    neg.l   %d2                         | %d2 = %d3 - %d2

    move.l  %d2, (YDELAYA-4,%a5)        | p->buf[YDELAYA-1]  = %d2

    movem.l (YcoeffsA,%a6), %d4-%d7     | %d4  = p->YcoeffsA[0]
                                        | %d5  = p->YcoeffsA[1]
                                        | %d6  = p->YcoeffsA[2]
                                        | %d7  = p->YcoeffsA[3]

    mac.l   %d3, %d4, %acc0     | %acc0  = p->buf[YDELAYA] * p->YcoeffsA[0]
    mac.l   %d2, %d5, %acc0     | %acc0 += p->buf[YDELAYA-1] * p->YcoeffsA[1]
    mac.l   %d1, %d6, %acc0     | %acc0 += p->buf[YDELAYA-2] * p->YcoeffsA[2]
    mac.l   %d0, %d7, %acc0     | %acc0 += p->buf[YDELAYA-3] * p->YcoeffsA[3]

    tst.l   %d2
    beq.s   1f
    spl.b   %d2                         | pos: 0x??????ff, neg: 0x??????00
    extb.l  %d2                         | pos: 0xffffffff, neg: 0x00000000
    or.l    #1, %d2                     | pos: 0xffffffff, neg: 0x00000001
1:                                      | %d2 = SIGN(%d2)
    move.l  %d2, (YADAPTCOEFFSA-4,%a5)  | p->buf[YADAPTCOEFFSA-1]  = %d2

    tst.l   %d3
    beq.s   1f
    spl.b   %d3
    extb.l  %d3
    or.l    #1, %d3
1:                                      | %d3 = SIGN(%d3)
    move.l  %d3, (YADAPTCOEFFSA,%a5)    | p->buf[YADAPTCOEFFSA]  = %d3

    | Predictor Y, Filter B
    
    movem.l (YfilterB,%a6), %d2-%d3     | %d2  = p->YfilterB
                                        | %d3  = p->XfilterA
    move.l  %d3, (YfilterB,%a6)         | p->YfilterB = %d3

    move.l  %d2, %d1                    | %d1  = %d2
    lsl.l   #5, %d2                     | %d2  = %d2 * 32
    sub.l   %d1, %d2                    | %d2 -= %d1 (== 31 * old_d2)
    asr.l   #5, %d2                     | %d2 >>= 5
    sub.l   %d2, %d3                    | %d3 -= %d2
    
    movem.l (YDELAYB-16,%a5), %d4-%d7   | %d4  = p->buf[YDELAYB-4]
                                        | %d5  = p->buf[YDELAYB-3]
                                        | %d6  = p->buf[YDELAYB-2]
                                        | %d7  = p->buf[YDELAYB-1]
    sub.l   %d3, %d7
    neg.l   %d7                         | %d7  = %d3 - %d7

    move.l  %d7, (YDELAYB-4,%a5)        | p->buf[YDELAYB-1]  = %d7

    movem.l (YcoeffsB,%a6), %d1-%d2/%a0-%a2 | %d1  = p->YcoeffsB[0]
                                        | %d2  = p->YcoeffsB[1]
                                        | %a0  = p->YcoeffsB[2]
                                        | %a1  = p->YcoeffsB[3]
                                        | %a2  = p->YcoeffsB[4]

    mac.l   %d3, %d1, %acc1     | %acc1  = p->buf[YDELAYB] * p->YcoeffsB[0]
    mac.l   %d7, %d2, %acc1     | %acc1 += p->buf[YDELAYB-1] * p->YcoeffsB[1]
    mac.l   %d6, %a0, %acc1     | %acc1 += p->buf[YDELAYB-2] * p->YcoeffsB[2]
    mac.l   %d5, %a1, %acc1     | %acc1 += p->buf[YDELAYB-3] * p->YcoeffsB[3]
    mac.l   %d4, %a2, %acc1     | %acc1 += p->buf[YDELAYB-4] * p->YcoeffsB[4]
    
    move.l  %d3, (YDELAYB, %a5)         | p->buf[YDELAYB]  = %d3
    
    tst.l   %d7
    beq.s   1f
    spl.b   %d7
    extb.l  %d7
    or.l    #1, %d7
1:                                      | %d7 = SIGN(%d7)
    move.l  %d7, (YADAPTCOEFFSB-4,%a5)  | p->buf[YADAPTCOEFFSB-1]  = %d7
    tst.l   %d3
    beq.s   1f
    spl.b   %d3
    extb.l  %d3
    or.l    #1, %d3
1:                                      | %d3 = SIGN(%d3)
    move.l  %d3, (YADAPTCOEFFSB, %a5)   | p->buf[YADAPTCOEFFSB]  = %d3

    | %d1, %d2, %a0, %a1, %a2 contain p->YcoeffsB[0..4]
    | %d7, %d3 contain p->buf[YADAPTCOEFFSB-1] and p->buf[YADAPTCOEFFSB]

    move.l  (%a3), %d0                  | %d0  = *decoded0
    beq.s   3f

    movem.l (YADAPTCOEFFSB-16,%a5), %d4-%d6 | %d4  = p->buf[YADAPTCOEFFSB-4]
                                            | %d5  = p->buf[YADAPTCOEFFSB-3]
                                            | %d6  = p->buf[YADAPTCOEFFSB-2]

    bmi.s   1f                          | flags still valid here

    | *decoded0 > 0

    sub.l   %d3, %d1        | %d1  = p->YcoeffsB[0] - p->buf[YADAPTCOEFFSB]
    sub.l   %d7, %d2        | %d2  = p->YcoeffsB[1] - p->buf[YADAPTCOEFFSB-1]
    sub.l   %d6, %a0        | %a0  = p->YcoeffsB[2] - p->buf[YADAPTCOEFFSB-2]
    sub.l   %d5, %a1        | %a1  = p->YcoeffsB[3] - p->buf[YADAPTCOEFFSB-3]
    sub.l   %d4, %a2        | %a2  = p->YcoeffsB[4] - p->buf[YADAPTCOEFFSB-4]

    movem.l %d1-%d2/%a0-%a2, (YcoeffsB,%a6) | Save p->YcoeffsB[]
    
    movem.l (YcoeffsA,%a6), %d4-%d7     | %d4  = p->YcoeffsA[0]
                                        | %d5  = p->YcoeffsA[1]
                                        | %d6  = p->YcoeffsA[2]
                                        | %d7  = p->YcoeffsA[3]
                                        
    movem.l (YADAPTCOEFFSA-12,%a5), %d2/%a0-%a2 
                                        | %d2  = p->buf[YADAPTCOEFFSA-3]
                                        | %a0  = p->buf[YADAPTCOEFFSA-2]
                                        | %a1  = p->buf[YADAPTCOEFFSA-1]
                                        | %a2  = p->buf[YADAPTCOEFFSA]
                                        
    sub.l   %a2, %d4        | %d4  = p->YcoeffsA[0] - p->buf[YADAPTCOEFFSA]
    sub.l   %a1, %d5        | %d5  = p->YcoeffsA[1] - p->buf[YADAPTCOEFFSA-1]
    sub.l   %a0, %d6        | %d6  = p->YcoeffsA[2] - p->buf[YADAPTCOEFFSA-2]
    sub.l   %d2, %d7        | %d7  = p->YcoeffsA[3] - p->buf[YADAPTCOEFFSA-3]
    
    bra.s   2f

1:  | *decoded0 < 0

    add.l   %d3, %d1        | %d1  = p->YcoeffsB[0] + p->buf[YADAPTCOEFFSB]
    add.l   %d7, %d2        | %d2  = p->YcoeffsB[1] + p->buf[YADAPTCOEFFSB-1]
    add.l   %d6, %a0        | %a0  = p->YcoeffsB[2] + p->buf[YADAPTCOEFFSB-2]
    add.l   %d5, %a1        | %a1  = p->YcoeffsB[3] + p->buf[YADAPTCOEFFSB-3]
    add.l   %d4, %a2        | %a2  = p->YcoeffsB[4] + p->buf[YADAPTCOEFFSB-4]

    movem.l %d1-%d2/%a0-%a2, (YcoeffsB,%a6) | Save p->YcoeffsB[]

    movem.l (YcoeffsA,%a6), %d4-%d7     | %d4  = p->YcoeffsA[0]
                                        | %d5  = p->YcoeffsA[1]
                                        | %d6  = p->YcoeffsA[2]
                                        | %d7  = p->YcoeffsA[3]

    movem.l (YADAPTCOEFFSA-12,%a5), %d2/%a0-%a2 
                                        | %d2  = p->buf[YADAPTCOEFFSA-3]
                                        | %a0  = p->buf[YADAPTCOEFFSA-2]
                                        | %a1  = p->buf[YADAPTCOEFFSA-1]
                                        | %a2  = p->buf[YADAPTCOEFFSA]

    add.l   %a2, %d4        | %d4  = p->YcoeffsA[0] + p->buf[YADAPTCOEFFSA]
    add.l   %a1, %d5        | %d5  = p->YcoeffsA[1] + p->buf[YADAPTCOEFFSA-1]
    add.l   %a0, %d6        | %d6  = p->YcoeffsA[2] + p->buf[YADAPTCOEFFSA-2]
    add.l   %d2, %d7        | %d7  = p->YcoeffsA[3] + p->buf[YADAPTCOEFFSA-3]

2:
    movem.l %d4-%d7, (YcoeffsA,%a6)     | Save p->YcoeffsA[]

3:
    | Finish Predictor Y

    movclr.l %acc0, %d1                 | %d1 = predictionA
    movclr.l %acc1, %d2                 | %d2 = predictionB
    asr.l   #1, %d2
    add.l   %d2, %d1                    | %d1 += (%d2 >> 1)
    asr.l   #8, %d1
    asr.l   #2, %d1                     | %d1 >>= 10
    add.l   %d0, %d1                    | %d1 += %d0
    move.l  %d1, (YlastA,%a6)           | p->YlastA  = %d1
    
    move.l  (YfilterA,%a6), %d2         | %d2  = p->YfilterA
    move.l  %d2, %d0
    lsl.l   #5, %d2
    sub.l   %d0, %d2                    | %d2 = 31 * %d2
    asr.l   #5, %d2                     | %d2 >>= 5
    add.l   %d1, %d2
    move.l  %d2, (YfilterA,%a6)         | p->YfilterA  = %d2

    | *decoded0 stored 2 instructions down, avoiding pipeline stall

    | ***** PREDICTOR X *****
    
    | Predictor X, Filter A
    
    move.l  (XlastA,%a6), %d3           | %d3  = p->XlastA

    move.l  %d2, (%a3)+                 | *(decoded0++)  = %d2 (p->YfilterA)

    movem.l (XDELAYA-12,%a5), %d0-%d2   | %d0  = p->buf[XDELAYA-3]
                                        | %d1  = p->buf[XDELAYA-2]
                                        | %d2  = p->buf[XDELAYA-1]

    move.l  %d3, (XDELAYA,%a5)          | p->buf[XDELAYA]  = %d3

    sub.l   %d3, %d2
    neg.l   %d2                         | %d2  = %d3 -%d2

    move.l  %d2, (XDELAYA-4,%a5)        | p->buf[XDELAYA-1]  = %d2

    movem.l (XcoeffsA,%a6), %d4-%d7     | %d4  = p->XcoeffsA[0]
                                        | %d5  = p->XcoeffsA[1]
                                        | %d6  = p->XcoeffsA[2]
                                        | %d7  = p->XcoeffsA[3]

    mac.l   %d3, %d4, %acc0     | %acc0  = p->buf[XDELAYA] * p->XcoeffsA[0]
    mac.l   %d2, %d5, %acc0     | %acc0 += p->buf[XDELAYA-1] * p->XcoeffsA[1]
    mac.l   %d1, %d6, %acc0     | %acc0 += p->buf[XDELAYA-2] * p->XcoeffsA[2]
    mac.l   %d0, %d7, %acc0     | %acc0 += p->buf[XDELAYA-3] * p->XcoeffsA[3]

    tst.l   %d2
    beq.s   1f
    spl.b   %d2                         | pos: 0x??????ff, neg: 0x??????00
    extb.l  %d2                         | pos: 0xffffffff, neg: 0x00000000
    or.l    #1, %d2                     | pos: 0xffffffff, neg: 0x00000001
1:                                      | %d2 = SIGN(%d2)
    move.l  %d2, (XADAPTCOEFFSA-4,%a5)  | p->buf[XADAPTCOEFFSA-1]  = %d2

    tst.l   %d3
    beq.s   1f
    spl.b   %d3
    extb.l  %d3
    or.l    #1, %d3
1:                                      | %d3 = SIGN(%d3)
    move.l  %d3, (XADAPTCOEFFSA,%a5)    | p->buf[XADAPTCOEFFSA]  = %d3

    | Predictor X, Filter B
    
    movem.l (XfilterB,%a6), %d2-%d3     | %d2  = p->XfilterB
                                        | %d3  = p->YfilterA
    move.l  %d3, (XfilterB,%a6)         | p->XfilterB = %d3

    move.l  %d2, %d1                    | %d1  = %d2
    lsl.l   #5, %d2                     | %d2  = %d2 * 32
    sub.l   %d1, %d2                    | %d2 -= %d1 (== 31 * old_d2)
    asr.l   #5, %d2                     | %d2 >>= 5
    sub.l   %d2, %d3                    | %d3 -= %d2 
    
    movem.l (XDELAYB-16,%a5), %d4-%d7   | %d4  = p->buf[XDELAYB-4]
                                        | %d5  = p->buf[XDELAYB-3]
                                        | %d6  = p->buf[XDELAYB-2]
                                        | %d7  = p->buf[XDELAYB-1]
    sub.l   %d3, %d7
    neg.l   %d7                         | %d7  = %d3 - %d7

    move.l  %d7, (XDELAYB-4,%a5)        | p->buf[XDELAYB-1]  = %d7

    movem.l (XcoeffsB,%a6), %d1-%d2/%a0-%a2 | %d1  = p->XcoeffsB[0]
                                        | %d2  = p->XcoeffsB[1]
                                        | %a0  = p->XcoeffsB[2]
                                        | %a1  = p->XcoeffsB[3]
                                        | %a2  = p->XcoeffsB[4]

    mac.l   %d3, %d1, %acc1     | %acc1  = p->buf[XDELAYB] * p->XcoeffsB[0]
    mac.l   %d7, %d2, %acc1     | %acc1 += p->buf[XDELAYB-1] * p->XcoeffsB[1]
    mac.l   %d6, %a0, %acc1     | %acc1 += p->buf[XDELAYB-2] * p->XcoeffsB[2]
    mac.l   %d5, %a1, %acc1     | %acc1 += p->buf[XDELAYB-3] * p->XcoeffsB[3]
    mac.l   %d4, %a2, %acc1     | %acc1 += p->buf[XDELAYB-4] * p->XcoeffsB[4]
    
    move.l  %d3, (XDELAYB, %a5)         | p->buf[XDELAYB]  = %d3
    
    tst.l   %d7
    beq.s   1f
    spl.b   %d7
    extb.l  %d7
    or.l    #1, %d7
1:                                      | %d7 = SIGN(%d7)
    move.l  %d7, (XADAPTCOEFFSB-4,%a5)  | p->buf[XADAPTCOEFFSB-1]  = %d7

    tst.l   %d3
    beq.s   1f
    spl.b   %d3
    extb.l  %d3
    or.l    #1, %d3
1:                                      | %d3 = SIGN(%d3)
    move.l  %d3, (XADAPTCOEFFSB, %a5)   | p->buf[XADAPTCOEFFSB]  = %d3

    | %d1, %d2, %a0, %a1, %a2 contain p->XcoeffsB[0..4]
    | %d7, %d3 contain p->buf[XADAPTCOEFFSB-1] and p->buf[XADAPTCOEFFSB]

    move.l  (%a4), %d0                  | %d0  = *decoded1
    beq.s   3f
    
    movem.l (XADAPTCOEFFSB-16,%a5), %d4-%d6 | %d4  = p->buf[XADAPTCOEFFSB-4]
                                            | %d5  = p->buf[XADAPTCOEFFSB-3]
                                            | %d6  = p->buf[XADAPTCOEFFSB-2]

    bmi.s   1f                          | flags still valid here
    
    | *decoded1 > 0

    sub.l   %d3, %d1        | %d1  = p->XcoeffsB[0] - p->buf[XADAPTCOEFFSB]
    sub.l   %d7, %d2        | %d2  = p->XcoeffsB[1] - p->buf[XADAPTCOEFFSB-1]
    sub.l   %d6, %a0        | %a0  = p->XcoeffsB[2] - p->buf[XADAPTCOEFFSB-2]
    sub.l   %d5, %a1        | %a1  = p->XcoeffsB[3] - p->buf[XADAPTCOEFFSB-3]
    sub.l   %d4, %a2        | %a2  = p->XcoeffsB[4] - p->buf[XADAPTCOEFFSB-4]

    movem.l %d1-%d2/%a0-%a2, (XcoeffsB,%a6) | Save p->XcoeffsB[]

    movem.l (XcoeffsA,%a6), %d4-%d7     | %d4  = p->XcoeffsA[0]
                                        | %d5  = p->XcoeffsA[1]
                                        | %d6  = p->XcoeffsA[2]
                                        | %d7  = p->XcoeffsA[3]
                                        
    movem.l (XADAPTCOEFFSA-12,%a5), %d2/%a0-%a2 
                                        | %d2  = p->buf[XADAPTCOEFFSA-3]
                                        | %a0  = p->buf[XADAPTCOEFFSA-2]
                                        | %a1  = p->buf[XADAPTCOEFFSA-1]
                                        | %a2  = p->buf[XADAPTCOEFFSA]
                                        
    sub.l   %a2, %d4        | %d4  = p->XcoeffsA[0] - p->buf[XADAPTCOEFFSA]
    sub.l   %a1, %d5        | %d5  = p->XcoeffsA[1] - p->buf[XADAPTCOEFFSA-1]
    sub.l   %a0, %d6        | %d6  = p->XcoeffsA[2] - p->buf[XADAPTCOEFFSA-2]
    sub.l   %d2, %d7        | %d7  = p->XcoeffsA[3] - p->buf[XADAPTCOEFFSA-3]

    bra.s   2f

1:  | *decoded1 < 0

    add.l   %d3, %d1        | %d1  = p->XcoeffsB[0] + p->buf[XADAPTCOEFFSB]
    add.l   %d7, %d2        | %d2  = p->XcoeffsB[1] + p->buf[XADAPTCOEFFSB-1]
    add.l   %d6, %a0        | %a0  = p->XcoeffsB[2] + p->buf[XADAPTCOEFFSB-2]
    add.l   %d5, %a1        | %a1  = p->XcoeffsB[3] + p->buf[XADAPTCOEFFSB-3]
    add.l   %d4, %a2        | %a2  = p->XcoeffsB[4] + p->buf[XADAPTCOEFFSB-4]
    
    movem.l %d1-%d2/%a0-%a2, (XcoeffsB,%a6) | Save p->XcoeffsB[]
    
    movem.l (XcoeffsA,%a6), %d4-%d7     | %d4  = p->XcoeffsA[0]
                                        | %d5  = p->XcoeffsA[1]
                                        | %d6  = p->XcoeffsA[2]
                                        | %d7  = p->XcoeffsA[3]

    movem.l (XADAPTCOEFFSA-12,%a5), %d2/%a0-%a2 
                                        | %d2  = p->buf[XADAPTCOEFFSA-3]
                                        | %a0  = p->buf[XADAPTCOEFFSA-2]
                                        | %a1  = p->buf[XADAPTCOEFFSA-1]
                                        | %a2  = p->buf[XADAPTCOEFFSA]
                                        
    add.l   %a2, %d4        | %d4  = p->XcoeffsA[0] + p->buf[XADAPTCOEFFSA]
    add.l   %a1, %d5        | %d5  = p->XcoeffsA[1] + p->buf[XADAPTCOEFFSA-1]
    add.l   %a0, %d6        | %d6  = p->XcoeffsA[2] + p->buf[XADAPTCOEFFSA-2]
    add.l   %d2, %d7        | %d7  = p->XcoeffsA[3] + p->buf[XADAPTCOEFFSA-3]

2:
    movem.l %d4-%d7, (XcoeffsA,%a6)     | Save p->XcoeffsA[]

3:
    | Finish Predictor X

    movclr.l %acc0, %d1                 | %d1 = predictionA
    movclr.l %acc1, %d2                 | %d2 = predictionB
    asr.l   #1, %d2
    add.l   %d2, %d1                    | %d1 += (%d2 >> 1)
    asr.l   #8, %d1
    asr.l   #2, %d1                     | %d1 >>= 10
    add.l   %d0, %d1                    | %d1 += %d0
    move.l  %d1, (XlastA,%a6)           | p->XlastA  = %d1
    
    move.l  (XfilterA,%a6), %d2         | %d2  = p->XfilterA
    move.l  %d2, %d0
    lsl.l   #5, %d2
    sub.l   %d0, %d2                    | %d2 = 31 * %d2
    asr.l   #5, %d2                     | %d6 >>= 2
    add.l   %d1, %d2
    move.l  %d2, (XfilterA,%a6)         | p->XfilterA  = %d2

    | *decoded1 stored 3 instructions down, avoiding pipeline stall

    | ***** COMMON *****
    
    addq.l  #4, %a5                     | p->buf++
    lea.l   (historybuffer+PREDICTOR_HISTORY_SIZE*4,%a6), %a2
                            | %a2 = &p->historybuffer[PREDICTOR_HISTORY_SIZE]
    
    move.l  %d2, (%a4)+                 | *(decoded1++)  = %d2 (p->XfilterA)

    cmp.l   %a2, %a5
    beq.s   .move_hist      | History buffer is full, we need to do a memmove

    subq.l  #1, (%sp)                   | decrease loop count
    bne.w   .loop

.done:
    move.l  %a5, (%a6)                  | Save value of p->buf
    movem.l (4,%sp), %d2-%d7/%a2-%a6
    lea.l   (12*4,%sp), %sp
    rts
    
.move_hist:
    lea.l   (historybuffer,%a6), %a2

    | dest = %a2 (p->historybuffer)
    | src = %a5 (p->buf)
    | n = 200
    
    movem.l (%a5), %d0-%d7/%a0-%a1      | 40 bytes
    movem.l %d0-%d7/%a0-%a1, (%a2)
    movem.l (40,%a5), %d0-%d7/%a0-%a1   | 40 bytes
    movem.l %d0-%d7/%a0-%a1, (40,%a2)
    movem.l (80,%a5), %d0-%d7/%a0-%a1   | 40 bytes
    movem.l %d0-%d7/%a0-%a1, (80,%a2)
    movem.l (120,%a5), %d0-%d7/%a0-%a1  | 40 bytes
    movem.l %d0-%d7/%a0-%a1, (120,%a2)
    movem.l (160,%a5), %d0-%d7/%a0-%a1  | 40 bytes
    movem.l %d0-%d7/%a0-%a1, (160,%a2)

    move.l  %a2, %a5                    | p->buf = &p->historybuffer[0]

    subq.l  #1, (%sp)                   | decrease loop count
    bne.w   .loop

    bra.s   .done
    .size   predictor_decode_stereo, .-predictor_decode_stereo


    .global predictor_decode_mono
    .type   predictor_decode_mono,@function

| void predictor_decode_mono(struct predictor_t* p,
|                            int32_t* decoded0,
|                            int count)

predictor_decode_mono:
    lea.l   (-11*4,%sp), %sp
    movem.l %d2-%d7/%a2-%a6, (%sp)

    move.l  #0, %macsr                  | signed integer mode

    move.l  (11*4+4,%sp), %a6           | %a6 = p
    move.l  (11*4+8,%sp), %a4           | %a4 = decoded0
    move.l  (11*4+12,%sp), %d7          | %d7 = count
    move.l  (%a6), %a5                  | %a5 = p->buf

    move.l  (YlastA,%a6), %d3           | %d3  = p->YlastA
    
.loopm:

    | ***** PREDICTOR *****

    movem.l (YDELAYA-12,%a5), %d0-%d2   | %d0  = p->buf[YDELAYA-3]
                                        | %d1  = p->buf[YDELAYA-2]
                                        | %d2  = p->buf[YDELAYA-1]

    move.l  %d3, (YDELAYA,%a5)          | p->buf[YDELAYA]  = %d3

    sub.l   %d3, %d2
    neg.l   %d2                         | %d2 = %d3 - %d2

    move.l  %d2, (YDELAYA-4,%a5)        | p->buf[YDELAYA-1]  = %d2

    movem.l (YcoeffsA,%a6), %a0-%a3     | %a0  = p->YcoeffsA[0]
                                        | %a1  = p->YcoeffsA[1]
                                        | %a2  = p->YcoeffsA[2]
                                        | %a3  = p->YcoeffsA[3]

    mac.l   %d3, %a0, %acc0     | %acc0  = p->buf[YDELAYA] * p->YcoeffsA[0]
    mac.l   %d2, %a1, %acc0     | %acc0 += p->buf[YDELAYA-1] * p->YcoeffsA[1]
    mac.l   %d1, %a2, %acc0     | %acc0 += p->buf[YDELAYA-2] * p->YcoeffsA[2]
    mac.l   %d0, %a3, %acc0     | %acc0 += p->buf[YDELAYA-3] * p->YcoeffsA[3]

    tst.l   %d2
    beq.s   1f
    spl.b   %d2                         | pos: 0x??????ff, neg: 0x??????00
    extb.l  %d2                         | pos: 0xffffffff, neg: 0x00000000
    or.l    #1, %d2                     | pos: 0xffffffff, neg: 0x00000001
1:                                      | %d2 = SIGN(%d2)
    move.l  %d2, (YADAPTCOEFFSA-4,%a5)  | p->buf[YADAPTCOEFFSA-1]  = %d2

    tst.l   %d3
    beq.s   1f
    spl.b   %d3
    extb.l  %d3
    or.l    #1, %d3
1:                                      | %d3 = SIGN(%d3)
    move.l  %d3, (YADAPTCOEFFSA,%a5)    | p->buf[YADAPTCOEFFSA]  = %d3

    move.l  (%a4), %d0                  | %d0 = *decoded0
    beq.s   3f

    movem.l (YADAPTCOEFFSA-12,%a5),%d4-%d5  | %d4  = p->buf[YADAPTCOEFFSA-3]
                                            | %d5  = p->buf[YADAPTCOEFFSA-2]
                                            
    bmi.s   1f                          | flags still valid here
    
    | *decoded0 > 0
    
    sub.l   %d3, %a0        | %a0  = p->YcoeffsA[0] - p->buf[YADAPTCOEFFSA]
    sub.l   %d2, %a1        | %a1  = p->YcoeffsA[1] - p->buf[YADAPTCOEFFSA-1]
    sub.l   %d5, %a2        | %a2  = p->YcoeffsA[2] - p->buf[YADAPTCOEFFSA-2]
    sub.l   %d4, %a3        | %a3  = p->YcoeffsA[3] - p->buf[YADAPTCOEFFSA-3]

    bra.s   2f

1:  | *decoded0 < 0

    add.l   %d3, %a0        | %a0  = p->YcoeffsA[0] - p->buf[YADAPTCOEFFSA]
    add.l   %d2, %a1        | %a1  = p->YcoeffsA[1] - p->buf[YADAPTCOEFFSA-1]
    add.l   %d5, %a2        | %a2  = p->YcoeffsA[2] - p->buf[YADAPTCOEFFSA-2]
    add.l   %d4, %a3        | %a3  = p->YcoeffsA[3] - p->buf[YADAPTCOEFFSA-3]

2:
    movem.l %a0-%a3, (YcoeffsA,%a6)     | save p->YcoeffsA[]

3:
    | Finish Predictor

    movclr.l %acc0, %d3                 | %d3 = predictionA
    asr.l   #8, %d3
    asr.l   #2, %d3                     | %d3 >>= 10
    add.l   %d0, %d3                    | %d3 += %d0

    move.l  (YfilterA,%a6), %d2         | %d2  = p->YfilterA
    move.l  %d2, %d0
    lsl.l   #5, %d2
    sub.l   %d0, %d2                    | %d2 = 31 * %d2
    asr.l   #5, %d2                     | %d2 >>= 5
    add.l   %d3, %d2
    move.l  %d2, (YfilterA,%a6)         | p->YfilterA  = %d2

    | *decoded0 stored 3 instructions down, avoiding pipeline stall

    | ***** COMMON *****

    addq.l  #4, %a5                     | p->buf++
    lea.l   (historybuffer+PREDICTOR_HISTORY_SIZE*4,%a6), %a3
                            | %a3 = &p->historybuffer[PREDICTOR_HISTORY_SIZE]
    
    move.l  %d2, (%a4)+                 | *(decoded0++)  = %d2 (p->YfilterA)

    cmp.l   %a3, %a5
    beq.s   .move_histm     | History buffer is full, we need to do a memmove
                                              
    subq.l  #1, %d7                     | decrease loop count
    bne.w   .loopm

    move.l  %d3, (YlastA,%a6)           | %d3  = p->YlastA

.donem:
    move.l  %a5, (%a6)                  | Save value of p->buf
    movem.l (%sp), %d2-%d7/%a2-%a6
    lea.l   (11*4,%sp), %sp
    rts
    
.move_histm:
    move.l  %d3, (YlastA,%a6)           | %d3  = p->YlastA

    lea.l   (historybuffer,%a6), %a3

    | dest = %a3 (p->historybuffer)
    | src = %a5 (p->buf)
    | n = 200
    
    movem.l (%a5), %d0-%d6/%a0-%a2      | 40 bytes
    movem.l %d0-%d6/%a0-%a2, (%a3)
    movem.l (40,%a5), %d0-%d6/%a0-%a2   | 40 bytes
    movem.l %d0-%d6/%a0-%a2, (40,%a3)
    movem.l (80,%a5), %d0-%d6/%a0-%a2   | 40 bytes
    movem.l %d0-%d6/%a0-%a2, (80,%a3)
    movem.l (120,%a5), %d0-%d6/%a0-%a2  | 40 bytes
    movem.l %d0-%d6/%a0-%a2, (120,%a3)
    movem.l (160,%a5), %d0-%d6/%a0-%a2  | 40 bytes
    movem.l %d0-%d6/%a0-%a2, (160,%a3)

    move.l  %a3, %a5                    | p->buf = &p->historybuffer[0]

    move.l  (YlastA,%a6), %d3           | %d3  = p->YlastA

    subq.l  #1, %d7                     | decrease loop count
    bne.w   .loopm

    bra.s   .donem
    .size   predictor_decode_mono, .-predictor_decode_mono
