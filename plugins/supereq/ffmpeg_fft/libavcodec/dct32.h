#ifndef DCT_32_H
#define DCT_32_H

#define FIXHR(x)       ((float)(x))
#define MULH3(x, y, s) ((s)*(y)*(x))
#define INTFLOAT float

void dct32(INTFLOAT *out, const INTFLOAT *tab);

#endif
