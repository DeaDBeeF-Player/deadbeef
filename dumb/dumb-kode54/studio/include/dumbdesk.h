#ifndef INCLUDED_DUMBDESK_H
#define INCLUDED_DUMBDESK_H


#include "dumbgui.h"
#include "dumbmenu.h"


typedef struct DESKGUI
{
	struct DESKGUI *next;

	GUI *gui;

	const char *name;
}
DESKGUI;


typedef struct GUI_DESKTOP_PARAM
{
	const char *title;
	GUI_MENU_PARAM *menu_bar;
}
GUI_DESKTOP_PARAM;


#define DESKTOP_CHANGED_TITLE GUI_OTHER
#define DESKTOP_CHANGED_REST (GUI_OTHER << 1)
#define DESKTOP_CHANGED_ALL (DESKTOP_CHANGED_TITLE | DESKTOP_CHANGED_REST)

typedef struct GUI_DESKTOP_DATA
{
	const char *title;
	GUI_MENU_PARAM *menu_bar;
	DESKGUI *deskgui;
}
GUI_DESKTOP_DATA;


extern GUI_COMMANDS gui_desktop_commands;


#endif /* INCLUDED_DUMBDESK_H */
