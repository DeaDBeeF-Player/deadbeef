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

	void (*free) (struct mp4p_atom_s *atom);
} mp4p_atom_t;

void
mp4p_atom_free (mp4p_atom_t *atom);

mp4p_atom_t *
mp4p_open (const char *fname);

#endif /* mp4parser_h */
