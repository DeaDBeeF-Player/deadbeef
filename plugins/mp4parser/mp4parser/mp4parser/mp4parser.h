#ifndef mp4parser_h
#define mp4parser_h

#include <stdint.h>

typedef struct mp4p_atom_s {
	uint64_t pos;
	uint32_t size;
	char type[4];
	void *data;
	struct mp4p_atom_s *subatoms;
	struct mp4p_atom_s *next;

	void (*free) (void *data);
} mp4p_atom_t;

typedef struct {
	char major_brand[4];
	char version[4];
	char compat_brand_1[4];
	char compat_brand_2[4];
} mp4p_mtyp_t;

void
mp4p_atom_free (mp4p_atom_t *atom);

mp4p_atom_t *
mp4p_open (const char *fname);

#endif /* mp4parser_h */
