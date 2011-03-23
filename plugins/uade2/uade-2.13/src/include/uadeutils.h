#ifndef _UADE_UTILS_H_
#define _UADE_UTILS_H_

#include <stdint.h>

#define uade_error(fmt, args...) do { \
    fprintf(stderr, "%s:%d: %s: " fmt, __FILE__, __LINE__, __func__, ## args); \
    abort(); \
  } while (0)

static inline uint16_t read_be_u16(void *s)
{
  uint16_t x;
  uint8_t *ptr = (uint8_t *) s;
  x = ptr[1] + (ptr[0] << 8);
  return x;
}

static inline uint32_t read_be_u32(void *s)
{
  uint32_t x;
  uint8_t *ptr = (uint8_t *) s;
  x = (ptr[0] << 24) + (ptr[1] << 16) + (ptr[2] << 8) + ptr[3];
  return x;
}

#endif
