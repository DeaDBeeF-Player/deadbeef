#ifndef _UADE_MD5_H_
#define _UADE_MD5_H_

#include <sys/types.h>
#include <stdint.h>

#define MD5_HASHBYTES 16

typedef struct MD5Context {
        uint32_t buf[4];
	uint32_t bits[2];
	unsigned char in[64];
} MD5_CTX;

void   MD5Init(MD5_CTX *context);
void   MD5Update(MD5_CTX *context, unsigned char const *buf,
	       unsigned len);
void   MD5Final(unsigned char digest[MD5_HASHBYTES], MD5_CTX *context);
void   MD5Transform(uint32_t buf[4], uint32_t const in[16]);

#endif /* !_UADE_MD5_H_ */
