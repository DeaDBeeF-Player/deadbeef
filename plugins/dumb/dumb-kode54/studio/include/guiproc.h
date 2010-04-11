#ifndef INCLUDED_GUIPROC_H
#define INCLUDED_GUIPROC_H


#define THE_SIZE 2


#if THE_SIZE == 2

#define BEVEL_SIZE 3
#define BORDER_SIZE 3
#define TITLE_HEIGHT 21

#define THE_FONT FONT2

#elif THE_SIZE == 1

#define BEVEL_SIZE 2
#define BORDER_SIZE 2
#define TITLE_HEIGHT 13

#define THE_FONT FONT1

#else

#define BEVEL_SIZE 1
#define BORDER_SIZE 1
#define TITLE_HEIGHT 7

#define THE_FONT FONT0

#endif


#define HIGHLIGHT 15
#define MIDTONE 7
#define SHADOW 8
#define TITLE_BG_FOCUS 9
#define TITLE_FG_FOCUS 14
#define TITLE_BG 8
#define TITLE_FG 7
#define MENU_BG_FOCUS 1
#define MENU_FG_FOCUS 15
#define MENU_BG MIDTONE
#define MENU_FG 0
#define DESKTOP 4


void draw_bevel(int l, int t, int r, int b, int tl, int m, int br);


#endif /* INCLUDED_GUIPROC_H */
