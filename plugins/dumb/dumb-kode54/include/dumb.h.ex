/* dumb.h: the user header file for DUMB. Include this in any of your files
 * where you wish to use the DUMB functions and variables.
 */

#ifndef DUMB_H
#define DUMB_H


#define DAT_DUH DAT_ID('D','U','H',' ')


typedef struct DUH DUH;


int install_dumb(int *errno_ptr, int (*atexit_ptr)(void (*func)(void)));
void remove_dumb(void);

DUH *load_duh(const char *filename);
void unload_duh(DUH *duh);

void register_dat_duh(void);

typedef void *(*DUH_LOAD_SIGNAL)(DUH *duh, PACKFILE *file);
typedef float *(*DUH_RENDER_SAMPLES)(DUH *duh, void *data);
typedef void (*DUH_FREE_SAMPLES)(float *samples);
typedef void (*DUH_UNLOAD_SIGNAL)(void *data);

void register_signal_type(
	long type,
	DUH_LOAD_SIGNAL    load_signal,
	DUH_RENDER_SAMPLES render_samples,
	DUH_FREE_SAMPLES   free_samples,
	DUH_UNLOAD_SIGNAL  unload_signal
);


#endif /* DUMB_H */
