#include <allegro.h>
#include "guiproc.h"


void draw_bevel(int l, int t, int r, int b, int tl, int m, int br)
{
	int n;
	for (n = 0; n < BEVEL_SIZE; n++) {
		hline(screen, l + n, t + n, r - 1 - n, tl);
		putpixel(screen, r - n, t + n, m);
		vline(screen, l + n, t + 1 + n, b - 1 - n, tl);
		vline(screen, r - n, t + 1 + n, b - 1 - n, br);
		putpixel(screen, l + n, b - n, m);
		hline(screen, l + 1 + n, b - n, r - n, br);
	}
}
