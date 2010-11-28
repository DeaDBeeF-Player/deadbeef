#ifndef __EQU_H
#define __EQU_H

#ifdef __cplusplus
extern "C" {
#endif

#define ENABLE_REAL 1
#define ENABLE_INT 0

typedef float REAL;
typedef struct {
    REAL *lires,*lires1,*lires2,*rires,*rires1,*rires2,*irest;
    REAL *fsamples;
    REAL *ditherbuf;
    int ditherptr;
    volatile int chg_ires,cur_ires;
    int winlen,winlenbit,tabsize,nbufsamples;
#if ENABLE_INT
    short *inbuf;
#endif
#if ENABLE_REAL
    REAL *finbuf;
#endif
    REAL *outbuf;
    int maxamp;
    int dither;
    int enable;
} SuperEqState;

void *paramlist_alloc (void);
void paramlist_free (void *);
void equ_makeTable(SuperEqState *state, float *lbc,float *rbc,void *param,float fs);
int equ_modifySamples(SuperEqState *state, char *buf,int nsamples,int nch,int bps);
int equ_modifySamples_float (SuperEqState *state, char *buf,int nsamples,int nch);
void equ_clearbuf(SuperEqState *state);
void equ_init(SuperEqState *state, int wb);
void equ_quit(SuperEqState *state);

#ifdef __cplusplus
}
#endif

#endif
