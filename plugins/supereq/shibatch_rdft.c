#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>

#include "SIMDBase.h"
#include "DFT.h"

typedef float REAL;
#define TYPE SIMDBase_TYPE_FLOAT

void rfft(int n,int isign,REAL *x) {
    static DFT *p = NULL;
    static int ipsize = 0;
    static int mode = 0;
    int newipsize;
    if (n == 0) {
        if (p) {
            DFT_dispose(p, mode);
            p = NULL;
        }
        return;
    }
    n = 1 << n;
    newipsize = 2+sqrt(n/2);
    if (newipsize > ipsize) {
        ipsize = newipsize;

        if (p) {
            DFT_dispose(p, mode);
            p = NULL;
        }

        mode = SIMDBase_chooseBestMode(TYPE);
        p = DFT_init(mode, n, 0);
    }

    DFT_execute(p, mode, x, 1);
}
