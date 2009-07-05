#ifndef __MESSAGEPUMP_H
#define __MESSAGEPUMP_H

#include <stdint.h>

int messagepump_init (void);
void messagepump_free (void);
int messagepump_push (uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2);
int messagepump_pop (uint32_t *id, uintptr_t *ctx, uint32_t *p1, uint32_t *p2);
//int messagepump_hasmessages (void);

#endif // __MESSAGEPUMP_H
