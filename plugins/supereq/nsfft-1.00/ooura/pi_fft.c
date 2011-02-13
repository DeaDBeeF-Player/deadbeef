/*
---- calculation of PI(= 3.14159...) using FFT ----
    by T.Ooura, ver. LG1.1.2-MP1.5a Sep. 2001.

This is a test program to estimate the performance of
the FFT routines: fft*g.c.

Example compilation:
    GNU      : gcc -O6 -ffast-math pi_fft.c fftsg.c -lm -o pi_fftsg
    SUN      : cc -fast -xO5 pi_fft.c fft8g.c -lm -o pi_fft8g
    Microsoft: cl /O2 /G6 pi_fft.c fft4g.c /Fepi_fft4g.exe
    ...
    etc.
*/

/* Please check the following macros before compiling */
#ifndef DBL_ERROR_MARGIN
#define DBL_ERROR_MARGIN 0.3  /* must be < 0.5 */
#endif


#include <math.h>
#include <limits.h>
#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>


void mp_load_0(int n, int radix, int out[]);
void mp_load_1(int n, int radix, int out[]);
void mp_copy(int n, int radix, int in[], int out[]);
void mp_round(int n, int radix, int m, int inout[]);
int mp_cmp(int n, int radix, int in1[], int in2[]);
void mp_add(int n, int radix, int in1[], int in2[], int out[]);
void mp_sub(int n, int radix, int in1[], int in2[], int out[]);
void mp_imul(int n, int radix, int in1[], int in2, int out[]);
int mp_idiv(int n, int radix, int in1[], int in2, int out[]);
void mp_idiv_2(int n, int radix, int in[], int out[]);
double mp_mul_radix_test(int n, int radix, int nfft, 
        double tmpfft[], int ip[], double w[]);
void mp_mul(int n, int radix, int in1[], int in2[], int out[], 
        int tmp[], int nfft, double tmp1fft[], double tmp2fft[], 
        double tmp3fft[], int ip[], double w[]);
void mp_squ(int n, int radix, int in[], int out[], int tmp[], 
        int nfft, double tmp1fft[], double tmp2fft[], 
        int ip[], double w[]);
void mp_mulh(int n, int radix, int in1[], int in2[], int out[], 
        int nfft, double in1fft[], double outfft[], 
        int ip[], double w[]);
void mp_squh(int n, int radix, int in[], int out[], 
        int nfft, double inoutfft[], int ip[], double w[]);
int mp_inv(int n, int radix, int in[], int out[], 
        int tmp1[], int tmp2[], int nfft, 
        double tmp1fft[], double tmp2fft[], int ip[], double w[]);
int mp_sqrt(int n, int radix, int in[], int out[], 
        int tmp1[], int tmp2[], int nfft, 
        double tmp1fft[], double tmp2fft[], int ip[], double w[]);
void mp_sprintf(int n, int log10_radix, int in[], char out[]);
void mp_sscanf(int n, int log10_radix, char in[], int out[]);
void mp_fprintf(int n, int log10_radix, int in[], FILE *fout);


int main()
{
    int nfft, log2_nfft, radix, log10_radix, n, npow, nprc;
    double err, d_time, n_op;
    int *a, *b, *c, *e, *i1, *i2, *ip;
    double *d1, *d2, *d3, *w;
    time_t t_1, t_2;
    FILE *f_log, *f_out;
    
    f_log = fopen("pi.log", "w");
    printf("PI calculation to estimate the FFT benchmarks\n");
    fprintf(f_log, "PI calculation to estimate the FFT benchmarks\n");
    printf("length of FFT =?\n");
    scanf("%d", &nfft);
    
    printf("initializing...\n");
    for (log2_nfft = 1; (1 << log2_nfft) < nfft; log2_nfft++);
    nfft = 1 << log2_nfft;
    n = nfft + 2;
    ip = (int *) malloc((3 + (int) sqrt(0.5 * nfft)) * sizeof(int));
    w = (double *) malloc(nfft / 2 * sizeof(double));
    a = (int *) malloc((n + 2) * sizeof(int));
    b = (int *) malloc((n + 2) * sizeof(int));
    c = (int *) malloc((n + 2) * sizeof(int));
    e = (int *) malloc((n + 2) * sizeof(int));
    i1 = (int *) malloc((n + 2) * sizeof(int));
    i2 = (int *) malloc((n + 2) * sizeof(int));
    d1 = (double *) malloc((nfft + 2) * sizeof(double));
    d2 = (double *) malloc((nfft + 2) * sizeof(double));
    d3 = (double *) malloc((nfft + 2) * sizeof(double));
    if (d3 == NULL) {
        printf("Allocation Failure!\n");
        exit(1);
    }
    ip[0] = 0;
    /* ---- radix test ---- */
    log10_radix = 1;
    radix = 10;
    err = mp_mul_radix_test(n, radix, nfft, d1, ip, w);
    err += DBL_EPSILON * (n * radix * radix / 4);
    while (100 * err < DBL_ERROR_MARGIN && radix <= INT_MAX / 20) {
        err *= 100;
        log10_radix++;
        radix *= 10;
    }
    printf("nfft= %d\nradix= %d\nerror_margin= %g\n", nfft, radix, err);
    fprintf(f_log, "nfft= %d\nradix= %d\nerror_margin= %g\n", nfft, radix, err);
    printf("calculating %d digits of PI...\n", log10_radix * (n - 2));
    fprintf(f_log, "calculating %d digits of PI...\n", log10_radix * (n - 2));
    /* ---- time check ---- */
    time(&t_1);
    /*
     * ---- a formula based on the AGM (Arithmetic-Geometric Mean) ----
     *   c = sqrt(0.125);
     *   a = 1 + 3 * c;
     *   b = sqrt(a);
     *   e = b - 0.625;
     *   b = 2 * b;
     *   c = e - c;
     *   a = a + e;
     *   npow = 4;
     *   do {
     *       npow = 2 * npow;
     *       e = (a + b) / 2;
     *       b = sqrt(a * b);
     *       e = e - b;
     *       b = 2 * b;
     *       c = c - e;
     *       a = e + b;
     *   } while (e > SQRT_SQRT_EPSILON);
     *   e = e * e / 4;
     *   a = a + b;
     *   pi = (a * a - e - e / 2) / (a * c - e) / npow;
     * ---- modification ----
     *   This is a modified version of Gauss-Legendre formula
     *   (by T.Ooura). It is faster than original version.
     * ---- reference ----
     *   1. E.Salamin, 
     *      Computation of PI Using Arithmetic-Geometric Mean, 
     *      Mathematics of Computation, Vol.30 1976.
     *   2. R.P.Brent, 
     *      Fast Multiple-Precision Evaluation of Elementary Functions, 
     *      J. ACM 23 1976.
     *   3. D.Takahasi, Y.Kanada, 
     *      Calculation of PI to 51.5 Billion Decimal Digits on 
     *      Distributed Memoriy Parallel Processors, 
     *      Transactions of Information Processing Society of Japan, 
     *      Vol.39 No.7 1998.
     *   4. T.Ooura, 
     *      Improvement of the PI Calculation Algorithm and 
     *      Implementation of Fast Multiple-Precision Computation, 
     *      Information Processing Society of Japan SIG Notes, 
     *      98-HPC-74, 1998.
     */
    /* ---- c = sqrt(0.125) ---- */
    mp_sscanf(n, log10_radix, "0.125", a);
    mp_sqrt(n, radix, a, c, i1, i2, nfft, d1, d2, ip, w);
    /* ---- a = 1 + 3 * c ---- */
    mp_imul(n, radix, c, 3, e);
    mp_sscanf(n, log10_radix, "1", a);
    mp_add(n, radix, a, e, a);
    /* ---- b = sqrt(a) ---- */
    mp_sqrt(n, radix, a, b, i1, i2, nfft, d1, d2, ip, w);
    /* ---- e = b - 0.625 ---- */
    mp_sscanf(n, log10_radix, "0.625", e);
    mp_sub(n, radix, b, e, e);
    /* ---- b = 2 * b ---- */
    mp_add(n, radix, b, b, b);
    /* ---- c = e - c ---- */
    mp_sub(n, radix, e, c, c);
    /* ---- a = a + e ---- */
    mp_add(n, radix, a, e, a);
    printf("AGM iteration\n");
    fprintf(f_log, "AGM iteration\n");
    npow = 4;
    do {
        npow *= 2;
        /* ---- e = (a + b) / 2 ---- */
        mp_add(n, radix, a, b, e);
        mp_idiv_2(n, radix, e, e);
        /* ---- b = sqrt(a * b) ---- */
        mp_mul(n, radix, a, b, a, i1, nfft, d1, d2, d3, ip, w);
        mp_sqrt(n, radix, a, b, i1, i2, nfft, d1, d2, ip, w);
        /* ---- e = e - b ---- */
        mp_sub(n, radix, e, b, e);
        /* ---- b = 2 * b ---- */
        mp_add(n, radix, b, b, b);
        /* ---- c = c - e ---- */
        mp_sub(n, radix, c, e, c);
        /* ---- a = e + b ---- */
        mp_add(n, radix, e, b, a);
        /* ---- convergence check ---- */
        nprc = -e[1];
        if (e[0] == 0) {
            nprc = n;
        }
        printf("precision= %d\n", 4 * nprc * log10_radix);
        fprintf(f_log, "precision= %d\n", 4 * nprc * log10_radix);
    } while (4 * nprc <= n);
    /* ---- e = e * e / 4 (half precision) ---- */
    mp_idiv_2(n, radix, e, e);
    mp_squh(n, radix, e, e, nfft, d1, ip, w);
    /* ---- a = a + b ---- */
    mp_add(n, radix, a, b, a);
    /* ---- a = (a * a - e - e / 2) / (a * c - e) / npow ---- */
    mp_mul(n, radix, a, c, c, i1, nfft, d1, d2, d3, ip, w);
    mp_sub(n, radix, c, e, c);
    mp_inv(n, radix, c, b, i1, i2, nfft, d1, d2, ip, w);
    mp_squ(n, radix, a, a, i1, nfft, d1, d2, ip, w);
    mp_sub(n, radix, a, e, a);
    mp_idiv_2(n, radix, e, e);
    mp_sub(n, radix, a, e, a);
    mp_mul(n, radix, a, b, a, i1, nfft, d1, d2, d3, ip, w);
    mp_idiv(n, radix, a, npow, a);
    /* ---- time check ---- */
    time(&t_2);
    /* ---- output ---- */
    f_out = fopen("pi.dat", "w");
    printf("writing pi.dat...\n");
    mp_fprintf(n - 1, log10_radix, a, f_out);
    fclose(f_out);
    free(d3);
    free(d2);
    free(d1);
    free(i2);
    free(i1);
    free(e);
    free(c);
    free(b);
    free(a);
    free(w);
    free(ip);
    /* ---- benchmark ---- */
    n_op = 50.0 * nfft * log2_nfft * log2_nfft;
    printf("floating point operation: %g op.\n", n_op);
    fprintf(f_log, "floating point operation: %g op.\n", n_op);
    /* ---- difftime ---- */
    d_time = difftime(t_2, t_1);
    printf("execution time: %g sec. (real time)\n", d_time);
    fprintf(f_log, "execution time: %g sec. (real time)\n", d_time);
    fclose(f_log);
    return 0;
}


/* -------- multiple precision routines -------- */


#include <math.h>
#include <float.h>
#include <stdio.h>

/* ---- floating point format ----
    data := data[0] * pow(radix, data[1]) * 
            (data[2] + data[3]/radix + data[4]/radix/radix + ...), 
    data[0]       : sign (1;data>0, -1;data<0, 0;data==0)
    data[1]       : exponent (0;data==0)
    data[2...n+1] : digits
   ---- function prototypes ----
    void mp_load_0(int n, int radix, int out[]);
    void mp_load_1(int n, int radix, int out[]);
    void mp_copy(int n, int radix, int in[], int out[]);
    void mp_round(int n, int radix, int m, int inout[]);
    int mp_cmp(int n, int radix, int in1[], int in2[]);
    void mp_add(int n, int radix, int in1[], int in2[], int out[]);
    void mp_sub(int n, int radix, int in1[], int in2[], int out[]);
    void mp_imul(int n, int radix, int in1[], int in2, int out[]);
    int mp_idiv(int n, int radix, int in1[], int in2, int out[]);
    void mp_idiv_2(int n, int radix, int in[], int out[]);
    double mp_mul_radix_test(int n, int radix, int nfft, 
            double tmpfft[], int ip[], double w[]);
    void mp_mul(int n, int radix, int in1[], int in2[], int out[], 
            int tmp[], int nfft, double tmp1fft[], double tmp2fft[], 
            double tmp3fft[], int ip[], double w[]);
    void mp_squ(int n, int radix, int in[], int out[], int tmp[], 
            int nfft, double tmp1fft[], double tmp2fft[], 
            int ip[], double w[]);
    void mp_mulh(int n, int radix, int in1[], int in2[], int out[], 
            int nfft, double in1fft[], double outfft[], 
            int ip[], double w[]);
    void mp_squh(int n, int radix, int in[], int out[], 
            int nfft, double inoutfft[], int ip[], double w[]);
    int mp_inv(int n, int radix, int in[], int out[], 
            int tmp1[], int tmp2[], int nfft, 
            double tmp1fft[], double tmp2fft[], int ip[], double w[]);
    int mp_sqrt(int n, int radix, int in[], int out[], 
            int tmp1[], int tmp2[], int nfft, 
            double tmp1fft[], double tmp2fft[], int ip[], double w[]);
    void mp_sprintf(int n, int log10_radix, int in[], char out[]);
    void mp_sscanf(int n, int log10_radix, char in[], int out[]);
    void mp_fprintf(int n, int log10_radix, int in[], FILE *fout);
   ----
*/


/* -------- mp_load routines -------- */


void mp_load_0(int n, int radix, int out[])
{
    int j;
    
    for (j = 0; j <= n + 1; j++) {
        out[j] = 0;
    }
}


void mp_load_1(int n, int radix, int out[])
{
    int j;
    
    out[0] = 1;
    out[1] = 0;
    out[2] = 1;
    for (j = 3; j <= n + 1; j++) {
        out[j] = 0;
    }
}


void mp_copy(int n, int radix, int in[], int out[])
{
    int j;
    
    for (j = 0; j <= n + 1; j++) {
        out[j] = in[j];
    }
}


void mp_round(int n, int radix, int m, int inout[])
{
    int j, x;
    
    if (m < n) {
        for (j = n + 1; j > m + 2; j--) {
            inout[j] = 0;
        }
        x = 2 * inout[m + 2];
        inout[m + 2] = 0;
        if (x >= radix) {
            for (j = m + 1; j >= 2; j--) {
                x = inout[j] + 1;
                if (x < radix) {
                    inout[j] = x;
                    break;
                }
                inout[j] = 0;
            }
            if (x >= radix) {
                inout[2] = 1;
                inout[1]++;
            }
        }
    }
}


/* -------- mp_add routines -------- */


int mp_cmp(int n, int radix, int in1[], int in2[])
{
    int mp_unsgn_cmp(int n, int in1[], int in2[]);
    
    if (in1[0] > in2[0]) {
        return 1;
    } else if (in1[0] < in2[0]) {
        return -1;
    }
    return in1[0] * mp_unsgn_cmp(n, &in1[1], &in2[1]);
}


void mp_add(int n, int radix, int in1[], int in2[], int out[])
{
    int mp_unsgn_cmp(int n, int in1[], int in2[]);
    int mp_unexp_add(int n, int radix, int expdif, 
            int in1[], int in2[], int out[]);
    int mp_unexp_sub(int n, int radix, int expdif, 
            int in1[], int in2[], int out[]);
    int outsgn, outexp, expdif;
    
    expdif = in1[1] - in2[1];
    outexp = in1[1];
    if (expdif < 0) {
        outexp = in2[1];
    }
    outsgn = in1[0] * in2[0];
    if (outsgn >= 0) {
        if (outsgn > 0) {
            outsgn = in1[0];
        } else {
            outsgn = in1[0] + in2[0];
            outexp = in1[1] + in2[1];
            expdif = 0;
        }
        if (expdif >= 0) {
            outexp += mp_unexp_add(n, radix, expdif, 
                    &in1[2], &in2[2], &out[2]);
        } else {
            outexp += mp_unexp_add(n, radix, -expdif, 
                    &in2[2], &in1[2], &out[2]);
        }
    } else {
        outsgn = mp_unsgn_cmp(n, &in1[1], &in2[1]);
        if (outsgn >= 0) {
            expdif = mp_unexp_sub(n, radix, expdif, 
                    &in1[2], &in2[2], &out[2]);
        } else {
            expdif = mp_unexp_sub(n, radix, -expdif, 
                    &in2[2], &in1[2], &out[2]);
        }
        outexp -= expdif;
        outsgn *= in1[0];
        if (expdif == n) {
            outsgn = 0;
        }
    }
    if (outsgn == 0) {
        outexp = 0;
    }
    out[0] = outsgn;
    out[1] = outexp;
}


void mp_sub(int n, int radix, int in1[], int in2[], int out[])
{
    int mp_unsgn_cmp(int n, int in1[], int in2[]);
    int mp_unexp_add(int n, int radix, int expdif, 
            int in1[], int in2[], int out[]);
    int mp_unexp_sub(int n, int radix, int expdif, 
            int in1[], int in2[], int out[]);
    int outsgn, outexp, expdif;
    
    expdif = in1[1] - in2[1];
    outexp = in1[1];
    if (expdif < 0) {
        outexp = in2[1];
    }
    outsgn = in1[0] * in2[0];
    if (outsgn <= 0) {
        if (outsgn < 0) {
            outsgn = in1[0];
        } else {
            outsgn = in1[0] - in2[0];
            outexp = in1[1] + in2[1];
            expdif = 0;
        }
        if (expdif >= 0) {
            outexp += mp_unexp_add(n, radix, expdif, 
                    &in1[2], &in2[2], &out[2]);
        } else {
            outexp += mp_unexp_add(n, radix, -expdif, 
                    &in2[2], &in1[2], &out[2]);
        }
    } else {
        outsgn = mp_unsgn_cmp(n, &in1[1], &in2[1]);
        if (outsgn >= 0) {
            expdif = mp_unexp_sub(n, radix, expdif, 
                    &in1[2], &in2[2], &out[2]);
        } else {
            expdif = mp_unexp_sub(n, radix, -expdif, 
                    &in2[2], &in1[2], &out[2]);
        }
        outexp -= expdif;
        outsgn *= in1[0];
        if (expdif == n) {
            outsgn = 0;
        }
    }
    if (outsgn == 0) {
        outexp = 0;
    }
    out[0] = outsgn;
    out[1] = outexp;
}


/* -------- mp_add child routines -------- */


int mp_unsgn_cmp(int n, int in1[], int in2[])
{
    int j, cmp;
    
    cmp = 0;
    for (j = 0; j <= n && cmp == 0; j++) {
        cmp = in1[j] - in2[j];
    }
    if (cmp > 0) {
        cmp = 1;
    } else if (cmp < 0) {
        cmp = -1;
    }
    return cmp;
}


int mp_unexp_add(int n, int radix, int expdif, 
        int in1[], int in2[], int out[])
{
    int j, x, carry;
    
    carry = 0;
    if (expdif == 0 && in1[0] + in2[0] >= radix) {
        x = in1[n - 1] + in2[n - 1];
        carry = x >= radix ? -1 : 0;
        for (j = n - 1; j > 0; j--) {
            x = in1[j - 1] + in2[j - 1] - carry;
            carry = x >= radix ? -1 : 0;
            out[j] = x - (radix & carry);
        }
        out[0] = -carry;
    } else {
        if (expdif > n) {
            expdif = n;
        }
        for (j = n - 1; j >= expdif; j--) {
            x = in1[j] + in2[j - expdif] - carry;
            carry = x >= radix ? -1 : 0;
            out[j] = x - (radix & carry);
        }
        for (j = expdif - 1; j >= 0; j--) {
            x = in1[j] - carry;
            carry = x >= radix ? -1 : 0;
            out[j] = x - (radix & carry);
        }
        if (carry != 0) {
            for (j = n - 1; j > 0; j--) {
                out[j] = out[j - 1];
            }
            out[0] = -carry;
        }
    }
    return -carry;
}


int mp_unexp_sub(int n, int radix, int expdif, 
        int in1[], int in2[], int out[])
{
    int j, x, borrow, ncancel;
    
    if (expdif > n) {
        expdif = n;
    }
    borrow = 0;
    for (j = n - 1; j >= expdif; j--) {
        x = in1[j] - in2[j - expdif] + borrow;
        borrow = x < 0 ? -1 : 0;
        out[j] = x + (radix & borrow);
    }
    for (j = expdif - 1; j >= 0; j--) {
        x = in1[j] + borrow;
        borrow = x < 0 ? -1 : 0;
        out[j] = x + (radix & borrow);
    }
    ncancel = 0;
    for (j = 0; j < n && out[j] == 0; j++) {
        ncancel = j + 1;
    }
    if (ncancel > 0 && ncancel < n) {
        for (j = 0; j < n - ncancel; j++) {
            out[j] = out[j + ncancel];
        }
        for (j = n - ncancel; j < n; j++) {
            out[j] = 0;
        }
    }
    return ncancel;
}


/* -------- mp_imul routines -------- */


void mp_imul(int n, int radix, int in1[], int in2, int out[])
{
    void mp_unsgn_imul(int n, double dradix, int in1[], double din2, 
            int out[]);
    
    if (in2 > 0) {
        out[0] = in1[0];
    } else if (in2 < 0) {
        out[0] = -in1[0];
        in2 = -in2;
    } else {
        out[0] = 0;
    }
    mp_unsgn_imul(n, radix, &in1[1], in2, &out[1]);
    if (out[0] == 0) {
        out[1] = 0;
    }
}


int mp_idiv(int n, int radix, int in1[], int in2, int out[])
{
    void mp_load_0(int n, int radix, int out[]);
    void mp_unsgn_idiv(int n, double dradix, int in1[], double din2, 
            int out[]);
    
    if (in2 == 0) {
        return -1;
    }
    if (in2 > 0) {
        out[0] = in1[0];
    } else {
        out[0] = -in1[0];
        in2 = -in2;
    }
    if (in1[0] == 0) {
        mp_load_0(n, radix, out);
        return 0;
    }
    mp_unsgn_idiv(n, radix, &in1[1], in2, &out[1]);
    return 0;
}


void mp_idiv_2(int n, int radix, int in[], int out[])
{
    int j, ix, carry, shift;
    
    out[0] = in[0];
    shift = 0;
    if (in[2] == 1) {
        shift = 1;
    }
    out[1] = in[1] - shift;
    carry = -shift;
    for (j = 2; j <= n + 1 - shift; j++) {
        ix = in[j + shift] + (radix & carry);
        carry = -(ix & 1);
        out[j] = ix >> 1;
    }
    if (shift > 0) {
        out[n + 1] = (radix & carry) >> 1;
    }
}


/* -------- mp_imul child routines -------- */


void mp_unsgn_imul(int n, double dradix, int in1[], double din2, 
        int out[])
{
    int j, carry, shift;
    double x, d1_radix;
    
    d1_radix = 1.0 / dradix;
    carry = 0;
    for (j = n; j >= 1; j--) {
        x = din2 * in1[j] + carry + 0.5;
        carry = (int) (d1_radix * x);
        out[j] = (int) (x - dradix * carry);
    }
    shift = 0;
    x = carry + 0.5;
    while (x > 1) {
        x *= d1_radix;
        shift++;
    }
    out[0] = in1[0] + shift;
    if (shift > 0) {
        while (shift > n) {
            carry = (int) (d1_radix * carry + 0.5);
            shift--;
        }
        for (j = n; j >= shift + 1; j--) {
            out[j] = out[j - shift];
        }
        for (j = shift; j >= 1; j--) {
            x = carry + 0.5;
            carry = (int) (d1_radix * x);
            out[j] = (int) (x - dradix * carry);
        }
    }
}


void mp_unsgn_idiv(int n, double dradix, int in1[], double din2, 
        int out[])
{
    int j, ix, carry, shift;
    double x, d1_in2;
    
    d1_in2 = 1.0 / din2;
    shift = 0;
    x = 0;
    do {
        shift++;
        x *= dradix;
        if (shift <= n) {
            x += in1[shift];
        }
    } while (x < din2 - 0.5);
    x += 0.5;
    ix = (int) (d1_in2 * x);
    carry = (int) (x - din2 * ix);
    out[1] = ix;
    shift--;
    out[0] = in1[0] - shift;
    if (shift >= n) {
        shift = n - 1;
    }
    for (j = 2; j <= n - shift; j++) {
        x = in1[j + shift] + dradix * carry + 0.5;
        ix = (int) (d1_in2 * x);
        carry = (int) (x - din2 * ix);
        out[j] = ix;
    }
    for (j = n - shift + 1; j <= n; j++) {
        x = dradix * carry + 0.5;
        ix = (int) (d1_in2 * x);
        carry = (int) (x - din2 * ix);
        out[j] = ix;
    }
}


/* -------- mp_mul routines -------- */


double mp_mul_radix_test(int n, int radix, int nfft, 
        double tmpfft[], int ip[], double w[])
{
    void rdft(int n, int isgn, double *a, int *ip, double *w);
    void mp_mul_csqu(int nfft, double dinout[]);
    double mp_mul_d2i_test(int radix, int nfft, double din[]);
    int j, ndata, radix_2;
    
    ndata = (nfft >> 1) + 1;
    if (ndata > n) {
        ndata = n;
    }
    tmpfft[nfft + 1] = radix - 1;
    for (j = nfft; j > ndata; j--) {
        tmpfft[j] = 0;
    }
    radix_2 = (radix + 1) / 2;
    for (j = ndata; j > 2; j--) {
        tmpfft[j] = radix_2;
    }
    tmpfft[2] = radix;
    tmpfft[1] = radix - 1;
    tmpfft[0] = 0;
    rdft(nfft, 1, &tmpfft[1], ip, w);
    mp_mul_csqu(nfft, tmpfft);
    rdft(nfft, -1, &tmpfft[1], ip, w);
    return 2 * mp_mul_d2i_test(radix, nfft, tmpfft);
}


void mp_mul(int n, int radix, int in1[], int in2[], int out[], 
        int tmp[], int nfft, double tmp1fft[], double tmp2fft[], 
        double tmp3fft[], int ip[], double w[])
{
    void mp_copy(int n, int radix, int in[], int out[]);
    void mp_add(int n, int radix, int in1[], int in2[], int out[]);
    void rdft(int n, int isgn, double *a, int *ip, double *w);
    void mp_mul_i2d(int n, int radix, int nfft, int shift, 
            int in[], double dout[]);
    void mp_mul_cmul(int nfft, double din[], double dinout[]);
    void mp_mul_cmuladd(int nfft, double din1[], double din2[], 
            double dinout[]);
    void mp_mul_d2i(int n, int radix, int nfft, double din[], int out[]);
    int n_h, shift;
    
    shift = (nfft >> 1) + 1;
    while (n > shift) {
        if (in1[shift + 2] + in2[shift + 2] != 0) {
            break;
        }
        shift++;
    }
    n_h = n / 2 + 1;
    if (n_h < n - shift) {
        n_h = n - shift;
    }
    /* ---- tmp3fft = (upper) in1 * (lower) in2 ---- */
    mp_mul_i2d(n, radix, nfft, 0, in1, tmp1fft);
    rdft(nfft, 1, &tmp1fft[1], ip, w);
    mp_mul_i2d(n, radix, nfft, shift, in2, tmp3fft);
    rdft(nfft, 1, &tmp3fft[1], ip, w);
    mp_mul_cmul(nfft, tmp1fft, tmp3fft);
    /* ---- tmp = (upper) in1 * (upper) in2 ---- */
    mp_mul_i2d(n, radix, nfft, 0, in2, tmp2fft);
    rdft(nfft, 1, &tmp2fft[1], ip, w);
    mp_mul_cmul(nfft, tmp2fft, tmp1fft);
    rdft(nfft, -1, &tmp1fft[1], ip, w);
    mp_mul_d2i(n, radix, nfft, tmp1fft, tmp);
    /* ---- tmp3fft += (upper) in2 * (lower) in1 ---- */
    mp_mul_i2d(n, radix, nfft, shift, in1, tmp1fft);
    rdft(nfft, 1, &tmp1fft[1], ip, w);
    mp_mul_cmuladd(nfft, tmp1fft, tmp2fft, tmp3fft);
    /* ---- out = tmp + tmp3fft ---- */
    rdft(nfft, -1, &tmp3fft[1], ip, w);
    mp_mul_d2i(n_h, radix, nfft, tmp3fft, out);
    if (out[0] != 0) {
        mp_add(n, radix, out, tmp, out);
    } else {
        mp_copy(n, radix, tmp, out);
    }
}


void mp_squ(int n, int radix, int in[], int out[], int tmp[], 
        int nfft, double tmp1fft[], double tmp2fft[], 
        int ip[], double w[])
{
    void mp_add(int n, int radix, int in1[], int in2[], int out[]);
    void rdft(int n, int isgn, double *a, int *ip, double *w);
    void mp_mul_i2d(int n, int radix, int nfft, int shift, 
            int in[], double dout[]);
    void mp_mul_cmul(int nfft, double din[], double dinout[]);
    void mp_mul_csqu(int nfft, double dinout[]);
    void mp_mul_d2i(int n, int radix, int nfft, double din[], int out[]);
    int n_h, shift;
    
    shift = (nfft >> 1) + 1;
    while (n > shift) {
        if (in[shift + 2] != 0) {
            break;
        }
        shift++;
    }
    n_h = n / 2 + 1;
    if (n_h < n - shift) {
        n_h = n - shift;
    }
    /* ---- tmp = (upper) in * (lower) in ---- */
    mp_mul_i2d(n, radix, nfft, 0, in, tmp1fft);
    rdft(nfft, 1, &tmp1fft[1], ip, w);
    mp_mul_i2d(n, radix, nfft, shift, in, tmp2fft);
    rdft(nfft, 1, &tmp2fft[1], ip, w);
    mp_mul_cmul(nfft, tmp1fft, tmp2fft);
    rdft(nfft, -1, &tmp2fft[1], ip, w);
    mp_mul_d2i(n_h, radix, nfft, tmp2fft, tmp);
    /* ---- out = 2 * tmp + ((upper) in)^2 ---- */
    mp_mul_csqu(nfft, tmp1fft);
    rdft(nfft, -1, &tmp1fft[1], ip, w);
    mp_mul_d2i(n, radix, nfft, tmp1fft, out);
    if (tmp[0] != 0) {
        mp_add(n_h, radix, tmp, tmp, tmp);
        mp_add(n, radix, out, tmp, out);
    }
}


void mp_mulh(int n, int radix, int in1[], int in2[], int out[], 
        int nfft, double in1fft[], double outfft[], int ip[], double w[])
{
    void rdft(int n, int isgn, double *a, int *ip, double *w);
    void mp_mul_i2d(int n, int radix, int nfft, int shift, 
            int in[], double dout[]);
    void mp_mul_cmul(int nfft, double din[], double dinout[]);
    void mp_mul_d2i(int n, int radix, int nfft, double din[], int out[]);
    
    mp_mul_i2d(n, radix, nfft, 0, in1, in1fft);
    rdft(nfft, 1, &in1fft[1], ip, w);
    mp_mul_i2d(n, radix, nfft, 0, in2, outfft);
    rdft(nfft, 1, &outfft[1], ip, w);
    mp_mul_cmul(nfft, in1fft, outfft);
    rdft(nfft, -1, &outfft[1], ip, w);
    mp_mul_d2i(n, radix, nfft, outfft, out);
}


void mp_mulh_use_in1fft(int n, int radix, double in1fft[], 
        int shift, int in2[], int out[], int nfft, double outfft[], 
        int ip[], double w[])
{
    void rdft(int n, int isgn, double *a, int *ip, double *w);
    void mp_mul_i2d(int n, int radix, int nfft, int shift, 
            int in[], double dout[]);
    void mp_mul_cmul(int nfft, double din[], double dinout[]);
    void mp_mul_d2i(int n, int radix, int nfft, double din[], int out[]);
    int n_h;
    
    while (n > shift) {
        if (in2[shift + 2] != 0) {
            break;
        }
        shift++;
    }
    n_h = n / 2 + 1;
    if (n_h < n - shift) {
        n_h = n - shift;
    }
    mp_mul_i2d(n, radix, nfft, shift, in2, outfft);
    rdft(nfft, 1, &outfft[1], ip, w);
    mp_mul_cmul(nfft, in1fft, outfft);
    rdft(nfft, -1, &outfft[1], ip, w);
    mp_mul_d2i(n_h, radix, nfft, outfft, out);
}


void mp_squh(int n, int radix, int in[], int out[], 
        int nfft, double inoutfft[], int ip[], double w[])
{
    void rdft(int n, int isgn, double *a, int *ip, double *w);
    void mp_mul_i2d(int n, int radix, int nfft, int shift, 
            int in[], double dout[]);
    void mp_mul_csqu(int nfft, double dinout[]);
    void mp_mul_d2i(int n, int radix, int nfft, double din[], int out[]);
    
    mp_mul_i2d(n, radix, nfft, 0, in, inoutfft);
    rdft(nfft, 1, &inoutfft[1], ip, w);
    mp_mul_csqu(nfft, inoutfft);
    rdft(nfft, -1, &inoutfft[1], ip, w);
    mp_mul_d2i(n, radix, nfft, inoutfft, out);
}


void mp_squh_use_in1fft(int n, int radix, double inoutfft[], int out[], 
        int nfft, int ip[], double w[])
{
    void rdft(int n, int isgn, double *a, int *ip, double *w);
    void mp_mul_csqu(int nfft, double dinout[]);
    void mp_mul_d2i(int n, int radix, int nfft, double din[], int out[]);
    
    mp_mul_csqu(nfft, inoutfft);
    rdft(nfft, -1, &inoutfft[1], ip, w);
    mp_mul_d2i(n, radix, nfft, inoutfft, out);
}


/* -------- mp_mul child routines -------- */


void mp_mul_i2d(int n, int radix, int nfft, int shift, 
        int in[], double dout[])
{
    int j, x, carry, ndata, radix_2, topdgt;
    
    ndata = 0;
    topdgt = 0;
    if (n > shift) {
        topdgt = in[shift + 2];
        ndata = (nfft >> 1) + 1;
        if (ndata > n - shift) {
            ndata = n - shift;
        }
    }
    dout[nfft + 1] = in[0] * topdgt;
    for (j = nfft; j > ndata; j--) {
        dout[j] = 0;
    }
    /* ---- abs(dout[j]) <= radix/2 (to keep FFT precision) ---- */
    if (ndata > 1) {
        radix_2 = radix / 2;
        carry = 0;
        for (j = ndata + 1; j > 3; j--) {
            x = in[j + shift] - carry;
            carry = x >= radix_2 ? -1 : 0;
            dout[j - 1] = x - (radix & carry);
        }
        dout[2] = in[shift + 3] - carry;
    }
    dout[1] = topdgt;
    dout[0] = in[1] - shift;
}


void mp_mul_cmul(int nfft, double din[], double dinout[])
{
    int j;
    double xr, xi, yr, yi;
    
    dinout[0] += din[0];
    dinout[1] *= din[1];
    dinout[2] *= din[2];
    for (j = 3; j < nfft; j += 2) {
        xr = din[j];
        xi = din[j + 1];
        yr = dinout[j];
        yi = dinout[j + 1];
        dinout[j] = xr * yr - xi * yi;
        dinout[j + 1] = xr * yi + xi * yr;
    }
    dinout[nfft + 1] *= din[nfft + 1];
}


void mp_mul_cmuladd(int nfft, double din1[], double din2[], 
        double dinout[])
{
    int j;
    double xr, xi, yr, yi;
    
    dinout[1] += din1[1] * din2[1];
    dinout[2] += din1[2] * din2[2];
    for (j = 3; j < nfft; j += 2) {
        xr = din1[j];
        xi = din1[j + 1];
        yr = din2[j];
        yi = din2[j + 1];
        dinout[j] += xr * yr - xi * yi;
        dinout[j + 1] += xr * yi + xi * yr;
    }
    dinout[nfft + 1] += din1[nfft + 1] * din2[nfft + 1];
}


void mp_mul_csqu(int nfft, double dinout[])
{
    int j;
    double xr, xi;
    
    dinout[0] *= 2;
    dinout[1] *= dinout[1];
    dinout[2] *= dinout[2];
    for (j = 3; j < nfft; j += 2) {
        xr = dinout[j];
        xi = dinout[j + 1];
        dinout[j] = xr * xr - xi * xi;
        dinout[j + 1] = 2 * xr * xi;
    }
    dinout[nfft + 1] *= dinout[nfft + 1];
}


void mp_mul_d2i(int n, int radix, int nfft, double din[], int out[])
{
    int j, carry, carry1, carry2, shift, ndata;
    double x, scale, d1_radix, d1_radix2, pow_radix, topdgt;
    
    scale = 2.0 / nfft;
    d1_radix = 1.0 / radix;
    d1_radix2 = d1_radix * d1_radix;
    topdgt = din[nfft + 1];
    x = topdgt < 0 ? -topdgt : topdgt;
    shift = x + 0.5 >= radix ? 1 : 0;
    /* ---- correction of cyclic convolution of din[1] ---- */
    x *= nfft * 0.5;
    din[nfft + 1] = din[1] - x;
    din[1] = x;
    /* ---- output of digits ---- */
    ndata = n;
    if (n > nfft + 1 + shift) {
        ndata = nfft + 1 + shift;
        for (j = n + 1; j > ndata + 1; j--) {
            out[j] = 0;
        }
    }
    x = 0;
    pow_radix = 1;
    for (j = ndata + 1 - shift; j <= nfft + 1; j++) {
        x += pow_radix * din[j];
        pow_radix *= d1_radix;
        if (pow_radix < DBL_EPSILON) {
            break;
        }
    }
    x = d1_radix2 * (scale * x + 0.5);
    carry2 = ((int) x) - 1;
    carry = (int) (radix * (x - carry2) + 0.5);
    for (j = ndata; j > 1; j--) {
        x = d1_radix2 * (scale * din[j - shift] + carry + 0.5);
        carry = carry2;
        carry2 = ((int) x) - 1;
        x = radix * (x - carry2);
        carry1 = (int) x;
        out[j + 1] = (int) (radix * (x - carry1));
        carry += carry1;
    }
    x = carry + ((double) radix) * carry2 + 0.5;
    if (shift == 0) {
        x += scale * din[1];
    }
    carry = (int) (d1_radix * x);
    out[2] = (int) (x - ((double) radix) * carry);
    if (carry > 0) {
        for (j = n + 1; j > 2; j--) {
            out[j] = out[j - 1];
        }
        out[2] = carry;
        shift++;
    }
    /* ---- output of exp, sgn ---- */
    x = din[0] + shift + 0.5;
    shift = ((int) x) - 1;
    out[1] = shift + ((int) (x - shift));
    out[0] = topdgt > 0.5 ? 1 : -1;
    if (out[2] == 0) {
        out[0] = 0;
        out[1] = 0;
    }
}


double mp_mul_d2i_test(int radix, int nfft, double din[])
{
    int j, carry, carry1, carry2;
    double x, scale, d1_radix, d1_radix2, err;
    
    scale = 2.0 / nfft;
    d1_radix = 1.0 / radix;
    d1_radix2 = d1_radix * d1_radix;
    /* ---- correction of cyclic convolution of din[1] ---- */
    x = din[nfft + 1] * nfft * 0.5;
    if (x < 0) {
        x = -x;
    }
    din[nfft + 1] = din[1] - x;
    /* ---- check of digits ---- */
    err = 0;
    carry = 0;
    carry2 = 0;
    for (j = nfft + 1; j > 1; j--) {
        x = d1_radix2 * (scale * din[j] + carry + 0.5);
        carry = carry2;
        carry2 = ((int) x) - 1;
        x = radix * (x - carry2);
        carry1 = (int) x;
        x = radix * (x - carry1);
        carry += carry1;
        x = x - 0.5 - ((int) x);
        if (x > err) {
            err = x;
        } else if (-x > err) {
            err = -x;
        }
    }
    return err;
}


/* -------- mp_inv routines -------- */


int mp_inv(int n, int radix, int in[], int out[], 
        int tmp1[], int tmp2[], int nfft, 
        double tmp1fft[], double tmp2fft[], int ip[], double w[])
{
    int mp_get_nfft_init(int radix, int nfft_max);
    void mp_inv_init(int n, int radix, int in[], int out[]);
    int mp_inv_newton(int n, int radix, int in[], int inout[], 
            int tmp1[], int tmp2[], int nfft, double tmp1fft[], 
            double tmp2fft[], int ip[], double w[]);
    int n_nwt, nfft_nwt, thr, prc;
    
    if (in[0] == 0) {
        return -1;
    }
    nfft_nwt = mp_get_nfft_init(radix, nfft);
    n_nwt = nfft_nwt + 2;
    if (n_nwt > n) {
        n_nwt = n;
    }
    mp_inv_init(n_nwt, radix, in, out);
    thr = 8;
    do {
        n_nwt = nfft_nwt + 2;
        if (n_nwt > n) {
            n_nwt = n;
        }
        prc = mp_inv_newton(n_nwt, radix, in, out, 
                tmp1, tmp2, nfft_nwt, tmp1fft, tmp2fft, ip, w);
        if (thr * nfft_nwt >= nfft) {
            thr = 0;
            if (2 * prc <= n_nwt - 2) {
                nfft_nwt >>= 1;
            }
        } else {
            if (3 * prc < n_nwt - 2) {
                nfft_nwt >>= 1;
            }
        }
        nfft_nwt <<= 1;
    } while (nfft_nwt <= nfft);
    return 0;
}


int mp_sqrt(int n, int radix, int in[], int out[], 
        int tmp1[], int tmp2[], int nfft, 
        double tmp1fft[], double tmp2fft[], int ip[], double w[])
{
    void mp_load_0(int n, int radix, int out[]);
    int mp_get_nfft_init(int radix, int nfft_max);
    void mp_sqrt_init(int n, int radix, int in[], int out[], int out_rev[]);
    int mp_sqrt_newton(int n, int radix, int in[], int inout[], 
            int inout_rev[], int tmp[], int nfft, double tmp1fft[], 
            double tmp2fft[], int ip[], double w[], int *n_tmp1fft);
    int n_nwt, nfft_nwt, thr, prc, n_tmp1fft;
    
    if (in[0] < 0) {
        return -1;
    } else if (in[0] == 0) {
        mp_load_0(n, radix, out);
        return 0;
    }
    nfft_nwt = mp_get_nfft_init(radix, nfft);
    n_nwt = nfft_nwt + 2;
    if (n_nwt > n) {
        n_nwt = n;
    }
    mp_sqrt_init(n_nwt, radix, in, out, tmp1);
    n_tmp1fft = 0;
    thr = 8;
    do {
        n_nwt = nfft_nwt + 2;
        if (n_nwt > n) {
            n_nwt = n;
        }
        prc = mp_sqrt_newton(n_nwt, radix, in, out, 
                tmp1, tmp2, nfft_nwt, tmp1fft, tmp2fft, 
                ip, w, &n_tmp1fft);
        if (thr * nfft_nwt >= nfft) {
            thr = 0;
            if (2 * prc <= n_nwt - 2) {
                nfft_nwt >>= 1;
            }
        } else {
            if (3 * prc < n_nwt - 2) {
                nfft_nwt >>= 1;
            }
        }
        nfft_nwt <<= 1;
    } while (nfft_nwt <= nfft);
    return 0;
}


/* -------- mp_inv child routines -------- */


int mp_get_nfft_init(int radix, int nfft_max)
{
    int nfft_init;
    double r;
    
    r = radix;
    nfft_init = 1;
    do {
        r *= r;
        nfft_init <<= 1;
    } while (DBL_EPSILON * r < 1 && nfft_init < nfft_max);
    return nfft_init;
}


void mp_inv_init(int n, int radix, int in[], int out[])
{
    void mp_unexp_d2mp(int n, int radix, double din, int out[]);
    double mp_unexp_mp2d(int n, int radix, int in[]);
    int outexp;
    double din;
    
    out[0] = in[0];
    outexp = -in[1];
    din = 1.0 / mp_unexp_mp2d(n, radix, &in[2]);
    while (din < 1) {
        din *= radix;
        outexp--;
    }
    out[1] = outexp;
    mp_unexp_d2mp(n, radix, din, &out[2]);
}


void mp_sqrt_init(int n, int radix, int in[], int out[], int out_rev[])
{
    void mp_unexp_d2mp(int n, int radix, double din, int out[]);
    double mp_unexp_mp2d(int n, int radix, int in[]);
    int outexp;
    double din;
    
    out[0] = 1;
    out_rev[0] = 1;
    outexp = in[1];
    din = mp_unexp_mp2d(n, radix, &in[2]);
    if (outexp % 2 != 0) {
        din *= radix;
        outexp--;
    }
    outexp /= 2;
    din = sqrt(din);
    if (din < 1) {
        din *= radix;
        outexp--;
    }
    out[1] = outexp;
    mp_unexp_d2mp(n, radix, din, &out[2]);
    outexp = -outexp;
    din = 1.0 / din;
    while (din < 1) {
        din *= radix;
        outexp--;
    }
    out_rev[1] = outexp;
    mp_unexp_d2mp(n, radix, din, &out_rev[2]);
}


void mp_unexp_d2mp(int n, int radix, double din, int out[])
{
    int j, x;
    
    for (j = 0; j < n; j++) {
        x = (int) din;
        if (x >= radix) {
            x = radix - 1;
            din = radix;
        }
        din = radix * (din - x);
        out[j] = x;
    }
}


double mp_unexp_mp2d(int n, int radix, int in[])
{
    int j;
    double d1_radix, dout;
    
    d1_radix = 1.0 / radix;
    dout = 0;
    for (j = n - 1; j >= 0; j--) {
        dout = d1_radix * dout + in[j];
    }
    return dout;
}


int mp_inv_newton(int n, int radix, int in[], int inout[], 
        int tmp1[], int tmp2[], int nfft, double tmp1fft[], 
        double tmp2fft[], int ip[], double w[])
{
    void mp_load_1(int n, int radix, int out[]);
    void mp_round(int n, int radix, int m, int inout[]);
    void mp_add(int n, int radix, int in1[], int in2[], int out[]);
    void mp_sub(int n, int radix, int in1[], int in2[], int out[]);
    void mp_mulh(int n, int radix, int in1[], int in2[], int out[], 
            int nfft, double in1fft[], double outfft[], 
            int ip[], double w[]);
    void mp_mulh_use_in1fft(int n, int radix, double in1fft[], 
            int shift, int in2[], int out[], int nfft, double outfft[], 
            int ip[], double w[]);
    int n_h, shift, prc;
    
    shift = (nfft >> 1) + 1;
    n_h = n / 2 + 1;
    if (n_h < n - shift) {
        n_h = n - shift;
    }
    /* ---- tmp1 = inout * (upper) in (half to normal precision) ---- */
    mp_round(n, radix, shift, inout);
    mp_mulh(n, radix, inout, in, tmp1, 
            nfft, tmp1fft, tmp2fft, ip, w);
    /* ---- tmp2 = 1 - tmp1 ---- */
    mp_load_1(n, radix, tmp2);
    mp_sub(n, radix, tmp2, tmp1, tmp2);
    /* ---- tmp2 -= inout * (lower) in (half precision) ---- */
    mp_mulh_use_in1fft(n, radix, tmp1fft, shift, in, tmp1, 
            nfft, tmp2fft, ip, w);
    mp_sub(n_h, radix, tmp2, tmp1, tmp2);
    /* ---- get precision ---- */
    prc = -tmp2[1];
    if (tmp2[0] == 0) {
        prc = nfft + 1;
    }
    /* ---- tmp2 *= inout (half precision) ---- */
    mp_mulh_use_in1fft(n_h, radix, tmp1fft, 0, tmp2, tmp2, 
            nfft, tmp2fft, ip, w);
    /* ---- inout += tmp2 ---- */
    if (tmp2[0] != 0) {
        mp_add(n, radix, inout, tmp2, inout);
    }
    return prc;
}


int mp_sqrt_newton(int n, int radix, int in[], int inout[], 
        int inout_rev[], int tmp[], int nfft, double tmp1fft[], 
        double tmp2fft[], int ip[], double w[], int *n_tmp1fft)
{
    void mp_round(int n, int radix, int m, int inout[]);
    void mp_add(int n, int radix, int in1[], int in2[], int out[]);
    void mp_sub(int n, int radix, int in1[], int in2[], int out[]);
    void mp_idiv_2(int n, int radix, int in[], int out[]);
    void mp_mulh(int n, int radix, int in1[], int in2[], int out[], 
            int nfft, double in1fft[], double outfft[], 
            int ip[], double w[]);
    void mp_squh(int n, int radix, int in[], int out[], 
            int nfft, double inoutfft[], int ip[], double w[]);
    void mp_squh_use_in1fft(int n, int radix, double inoutfft[], int out[], 
            int nfft, int ip[], double w[]);
    int n_h, nfft_h, shift, prc;
    
    nfft_h = nfft >> 1;
    shift = nfft_h + 1;
    if (nfft_h < 2) {
        nfft_h = 2;
    }
    n_h = n / 2 + 1;
    if (n_h < n - shift) {
        n_h = n - shift;
    }
    /* ---- tmp = inout_rev^2 (1/4 to half precision) ---- */
    mp_round(n_h, radix, (nfft_h >> 1) + 1, inout_rev);
    if (*n_tmp1fft != nfft_h) {
        mp_squh(n_h, radix, inout_rev, tmp, 
                nfft_h, tmp1fft, ip, w);
    } else {
        mp_squh_use_in1fft(n_h, radix, tmp1fft, tmp, 
                nfft_h, ip, w);
    }
    /* ---- tmp = inout_rev - inout * tmp (half precision) ---- */
    mp_round(n, radix, shift, inout);
    mp_mulh(n_h, radix, inout, tmp, tmp, 
            nfft, tmp1fft, tmp2fft, ip, w);
    mp_sub(n_h, radix, inout_rev, tmp, tmp);
    /* ---- inout_rev += tmp ---- */
    mp_add(n_h, radix, inout_rev, tmp, inout_rev);
    /* ---- tmp = in - inout^2 (half to normal precision) ---- */
    mp_squh_use_in1fft(n, radix, tmp1fft, tmp, 
            nfft, ip, w);
    mp_sub(n, radix, in, tmp, tmp);
    /* ---- get precision ---- */
    prc = in[1] - tmp[1];
    if (in[2] > tmp[2]) {
        prc++;
    }
    if (tmp[0] == 0) {
        prc = nfft + 1;
    }
    /* ---- tmp = tmp * inout_rev / 2 (half precision) ---- */
    mp_round(n_h, radix, shift, inout_rev);
    mp_mulh(n_h, radix, inout_rev, tmp, tmp, 
            nfft, tmp1fft, tmp2fft, ip, w);
    *n_tmp1fft = nfft;
    mp_idiv_2(n_h, radix, tmp, tmp);
    /* ---- inout += tmp ---- */
    if (tmp[0] != 0) {
        mp_add(n, radix, inout, tmp, inout);
    }
    return prc;
}


/* -------- mp_io routines -------- */


void mp_sprintf(int n, int log10_radix, int in[], char out[])
{
    int j, k, x, y, outexp, shift;
    
    if (in[0] < 0) {
        *out++ = '-';
    }
    x = in[2];
    shift = log10_radix;
    for (k = log10_radix; k > 0; k--) {
        y = x % 10;
        x /= 10;
        out[k] = '0' + y;
        if (y != 0) {
            shift = k;
        }
    }
    out[0] = out[shift];
    out[1] = '.';
    for (k = 1; k <= log10_radix - shift; k++) {
        out[k + 1] = out[k + shift];
    }
    outexp = log10_radix - shift;
    out += outexp + 2;
    for (j = 3; j <= n + 1; j++) {
        x = in[j];
        for (k = log10_radix - 1; k >= 0; k--) {
            y = x % 10;
            x /= 10;
            out[k] = '0' + y;
        }
        out += log10_radix;
    }
    *out++ = 'e';
    outexp += log10_radix * in[1];
    sprintf(out, "%d", outexp);
}


void mp_sscanf(int n, int log10_radix, char in[], int out[])
{
    char *s;
    int j, x, outexp, outexp_mod;
    
    while (*in == ' ') {
        in++;
    }
    out[0] = 1;
    if (*in == '-') {
        out[0] = -1;
        in++;
    } else if (*in == '+') {
        in++;
    }
    while (*in == ' ' || *in == '0') {
        in++;
    }
    outexp = 0;
    for (s = in; *s != '\0'; s++) {
        if (*s == 'e' || *s == 'E' || *s == 'd' || *s == 'D') {
            if (sscanf(++s, "%d", &outexp) != 1) {
                outexp = 0;
            }
            break;
        }
    }
    if (*in == '.') {
        do {
            outexp--;
            while (*++in == ' ');
        } while (*in == '0' && *in != '\0');
    } else if (*in != '\0') {
        s = in;
        while (*++s == ' ');
        while (*s >= '0' && *s <= '9' && *s != '\0') {
            outexp++;
            while (*++s == ' ');
        }
    }
    x = outexp / log10_radix;
    outexp_mod = outexp - log10_radix * x;
    if (outexp_mod < 0) {
        x--;
        outexp_mod += log10_radix;
    }
    out[1] = x;
    x = 0;
    j = 2;
    for (s = in; *s != '\0'; s++) {
        if (*s == '.' || *s == ' ') {
            continue;
        }
        if (*s < '0' || *s > '9') {
            break;
        }
        x = 10 * x + (*s - '0');
        if (--outexp_mod < 0) {
            if (j > n + 1) {
                break;
            }
            out[j++] = x;
            x = 0;
            outexp_mod = log10_radix - 1;
        }
    }
    while (outexp_mod-- >= 0) {
        x *= 10;
    }
    while (j <= n + 1) {
        out[j++] = x;
        x = 0;
    }
    if (out[2] == 0) {
        out[0] = 0;
        out[1] = 0;
    }
}


void mp_fprintf(int n, int log10_radix, int in[], FILE *fout)
{
    int j, k, x, y, outexp, shift;
    char out[256];
    
    if (in[0] < 0) {
        putc('-', fout);
    }
    x = in[2];
    shift = log10_radix;
    for (k = log10_radix; k > 0; k--) {
        y = x % 10;
        x /= 10;
        out[k] = '0' + y;
        if (y != 0) {
            shift = k;
        }
    }
    putc(out[shift], fout);
    putc('.', fout);
    for (k = 1; k <= log10_radix - shift; k++) {
        putc(out[k + shift], fout);
    }
    outexp = log10_radix - shift;
    for (j = 3; j <= n + 1; j++) {
        x = in[j];
        for (k = log10_radix - 1; k >= 0; k--) {
            y = x % 10;
            x /= 10;
            out[k] = '0' + y;
        }
        for (k = 0; k < log10_radix; k++) {
            putc(out[k], fout);
        }
    }
    putc('e', fout);
    outexp += log10_radix * in[1];
    sprintf(out, "%d", outexp);
    for (k = 0; out[k] != '\0'; k++) {
        putc(out[k], fout);
    }
}


