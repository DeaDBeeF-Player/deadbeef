#ifndef __THREADING_H
#define __THREADING_H

#include <stdint.h>

void thread_start (void (*fn)(uintptr_t ctx), uintptr_t ctx);
uintptr_t mutex_create (void);
void mutex_free (uintptr_t mtx);
int mutex_lock (uintptr_t mtx);
int mutex_unlock (uintptr_t mtx);

#endif

