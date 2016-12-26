#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mp4parser.h"
#import <Cocoa/Cocoa.h>

int main(int argc, const char * argv[]) {
	//	mp4p_atom_t *mp4 = mp4p_open ("/Users/waker/temp/01. Tim Follin - Chronos - Title (AY & Beeper).m4a", NULL);
	mp4p_atom_t *mp4 = mp4p_open ("/Users/waker/mus/ru/Виктор Аргонов Project - 2032 - Легенда О Несбывшемся Грядущем (2007) (ALAC)/01. Виктор Аргонов Project - Увертюра.m4a", NULL);

	printf ("----\n");
	printf ("Find ftyp...\n");
	mp4p_atom_t *ftyp = mp4p_atom_find (mp4, "ftyp");
	printf ("Find moov/trak/mdia/minf/dinf/dref/url ...\n");
	mp4p_atom_t *url = mp4p_atom_find (mp4, "moov/trak/mdia/minf/dinf/dref/url ");

	if (mp4) {
		mp4p_atom_free (mp4);
	}

	NSApplicationMain (argc, argv);
    return 0;
}
