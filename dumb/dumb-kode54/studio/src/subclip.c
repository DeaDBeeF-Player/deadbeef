#include <allegro.h>

#include "subclip.h"



void subclip(int l, int t, int r, int b, void (*proc)(void *data), void *data)
{
	int cl = screen->cl;
	int ct = screen->ct;
	int cr = screen->cr;
	int cb = screen->cb;

	if (l < cl) l = cl;
	if (t < ct) t = ct;
	if (r > cr) r = cr;
	if (b > cb) b = cb;

	if (r > l && b > t) {
		screen->cl = l;
		screen->ct = t;
		screen->cr = r;
		screen->cb = b;

		(*proc)(data);

		screen->cl = cl;
		screen->ct = ct;
		screen->cr = cr;
		screen->cb = cb;
	}
}

