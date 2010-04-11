#ifndef INCLUDED_DUMBMENU_H
#define INCLUDED_DUMBMENU_H


#include "dumbgui.h"


typedef void (*GUI_MENU_ACTIVATOR)(void *param);


typedef struct GUI_MENU_ENTRY
{
	char *text; /* If the first char is '\t', draw an arrow on the right. */
	GUI_MENU_ACTIVATOR activator;
	void *param;
}
GUI_MENU_ENTRY;


typedef struct GUI_MENU_PARAM
{
	int n_entries;
	GUI_MENU_ENTRY *entry;
}
GUI_MENU_PARAM;


#define GUI_MENU_REDRAW_ALL GUI_OTHER

typedef struct GUI_MENU_DATA
{
	GUI_MENU_PARAM *menu;
	int sel;
	int lastsel; /* When this is different from 'sel', rows 'lastsel' and 'sel' need redrawing. */
}
GUI_MENU_DATA;


extern GUI_COMMANDS gui_menu_commands;

void gui_menu_get_size(GUI_MENU_PARAM *menu, int *w, int *h);


#endif /* INCLUDED_DUMBMENU_H */
