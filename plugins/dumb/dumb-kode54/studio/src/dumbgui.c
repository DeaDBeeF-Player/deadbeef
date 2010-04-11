#include <stdlib.h>

#include "dumbgui.h"



GUI *gui_active;



void gui_set_active(GUI *gui)
{
	gui_changed_active(gui_active);
	gui_active = gui;
	gui_changed_active(gui_active);
}



GUI *gui_create(GUI_COMMANDS *com, GUI *parent, int x, int y, int w, int h, void *param)
{
	GUI *gui = malloc(sizeof(*gui));

	if (!gui)
		return NULL;

	gui->com = com;

	gui->parent = parent;

	gui->x = x;
	gui->y = y;
	gui->w = w;
	gui->h = h;

	gui->flags = 0;

	if (com->create) {
		(*com->create)(gui, param);

		if (!gui->data) {
			free(gui);
			return NULL;
		}
	} else
		gui->data = NULL;

	gui_changed_all(gui);

	return gui;
}



void gui_destroy(GUI *gui)
{
	if (gui) {
		if (gui->com->destroy)
			(*gui->com->destroy)(gui);

		free(gui);
	}
}



void gui_key(GUI *gui, int k)
{
	if (gui && gui->com->key)
		(*gui->com->key)(gui, k);
}



void gui_update(GUI *gui)
{
	if (gui && gui->com->update)
		(*gui->com->update)(gui);
}



void gui_draw(GUI *gui)
{
	if (gui && gui->com->draw)
		(*gui->com->draw)(gui);
}



/* For use with subclip(). */
void gui_draw_void(void *gui)
{
	gui_draw(gui);
}



void gui_drawn(GUI *gui)
{
	if (gui && gui->com->drawn)
		(*gui->com->drawn)(gui);
}



void gui_changed_all(GUI *gui)
{
	if (gui && gui->com->changed_all)
		(*gui->com->changed_all)(gui);
}



void gui_changed_active(GUI *gui)
{
	if (gui && gui->com->changed_active)
		(*gui->com->changed_active)(gui);
}

