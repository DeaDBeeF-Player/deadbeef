#ifndef INCLUDED_OPTIONS_H
#define INCLUDED_OPTIONS_H


typedef struct OPTIONS
{
	int gfx_w, gfx_h;
}
OPTIONS;


OPTIONS opt;


#ifdef ALLEGRO_DOS

#define DEF_GFX_W 320
#define DEF_GFX_H 200
#define DEF_GFX_STR "320x200"

#else

#define DEF_GFX_W 640
#define DEF_GFX_H 480
#define DEF_GFX_STR "640x480"

#endif


void load_options(void);
void save_options(void);


#endif /* INCLUDED_OPTIONS_H */
