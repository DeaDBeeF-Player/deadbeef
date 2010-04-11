#include <stdlib.h>
#include <allegro.h>

#include "main.h"
#include "dumbgui.h"
#include "guiproc.h"
#include "subclip.h"
#include "dumbmenu.h"



static void gui_menu_create(GUI *gui, void *param)
{
	GUI_MENU_PARAM *menu = param;

	GUI_MENU_DATA *data = gui->data = malloc(sizeof(*data));

	if (!data)
		return;

	data->menu = menu;
	data->sel = -1;
	/* data->lastsel will be initialised by gui_menu_drawn(). */
}



static void gui_menu_destroy(GUI *gui)
{
	free(gui->data);
}



static void gui_menu_key(GUI *gui, int k)
{
	GUI_MENU_DATA *data = gui->data;

	switch (k >> 8) {
		case KEY_UP:
		case KEY_8_PAD:
			if (data->sel <= 0) data->sel = data->menu->n_entries;
			data->sel--;
			break;
		case KEY_DOWN:
		case KEY_2_PAD:
			data->sel++;
			if (data->sel >= data->menu->n_entries) data->sel = 0;
			break;
		case KEY_RIGHT:
		case KEY_6_PAD:
		case KEY_ENTER:
			if (data->sel >= 0) {
				GUI_MENU_ENTRY *entry = &data->menu->entry[data->sel];
				if (k >> 8 != KEY_RIGHT || entry->text[0] == '\t')
					(*entry->activator)(entry->param);
			}
			// To do: defer 'right' to parent menus, making sure it doesn't reopen the same submenu
			break;
		case KEY_ESC:
			gui->flags |= GUI_FINISHED_WITH;
			break;
	}
}



static void gui_menu_update(GUI *gui)
{
	// Remove this function?
	(void)gui;
}



typedef struct GUI_MENU_DRAW_ENTRY_DATA
{
	GUI *gui;
	GUI_MENU_DATA *data;
	int i, y;
}
GUI_MENU_DRAW_ENTRY_DATA;



static void gui_menu_draw_entry_proc(void *data)
{
	GUI_MENU_DRAW_ENTRY_DATA *d = data;
	char *text = d->data->menu->entry[d->i].text;

	clear_to_color(screen, d->i == d->data->sel ? MENU_BG_FOCUS : MENU_BG);

	if (text[0] == '\t') {
		text++;
		// Draw an arrow
	}

	textout(screen, dat[THE_FONT].dat, text, d->gui->x + BORDER_SIZE, d->y + BORDER_SIZE,
		d->i == d->data->sel ? MENU_FG_FOCUS : MENU_FG);
}



static void gui_menu_draw(GUI *gui)
{
	GUI_MENU_DRAW_ENTRY_DATA d;
	d.gui = gui;
	d.data = gui->data;

	if (gui->flags & GUI_MENU_REDRAW_ALL) {
		// Redraw the whole menu
		d.y = gui->y;
		for (d.i = 0; d.i < d.data->menu->n_entries; d.i++) {
			subclip(gui->x, d.y, gui->x + gui->w, d.y + TITLE_HEIGHT, &gui_menu_draw_entry_proc, &d);
			d.y += TITLE_HEIGHT;
		}
	} else if (d.data->sel != d.data->lastsel) {
		if (d.data->lastsel >= 0) {
			// Redraw the 'lastsel' row
			d.i = d.data->lastsel;
			d.y = gui->y + d.i * TITLE_HEIGHT;
			subclip(gui->x, d.y, gui->x + gui->w, d.y + TITLE_HEIGHT, &gui_menu_draw_entry_proc, &d);
		}
		if (d.data->sel >= 0) { // Necessary?
			// Redraw the 'sel' row
			d.i = d.data->sel;
			d.y = gui->y + d.i * TITLE_HEIGHT;
			subclip(gui->x, d.y, gui->x + gui->w, d.y + TITLE_HEIGHT, &gui_menu_draw_entry_proc, &d);
		}
	}
}



static void gui_menu_drawn(GUI *gui)
{
	GUI_MENU_DATA *data = gui->data;

	gui->flags &= ~GUI_MENU_REDRAW_ALL;
	data->lastsel = data->sel;
}



static void gui_menu_changed_all(GUI *gui)
{
	gui->flags |= GUI_MENU_REDRAW_ALL;
}



static void gui_menu_changed_active(GUI *gui)
{
	// Remove this function?
	(void)gui;
}



GUI_COMMANDS gui_menu_commands = {
	&gui_menu_create,
	&gui_menu_destroy,
	&gui_menu_key,
	&gui_menu_update,
	&gui_menu_draw,
	&gui_menu_drawn,
	&gui_menu_changed_all,
	&gui_menu_changed_active
};



void gui_menu_get_size(GUI_MENU_PARAM *menu, int *w, int *h)
{
	int i;

	*w = 2*BORDER_SIZE + TITLE_HEIGHT; /* A reasonable minimum width */
	*h = 2*BORDER_SIZE;

	for (i = 0; i < menu->n_entries; i++) {
		char *text = menu->entry[i].text;
		int wi = 2*BORDER_SIZE;
		if (text[0] == '\t') {
			text++;
			wi += 10; // Fix this later
		}
		wi += text_length(dat[THE_FONT].dat, text);
		if (wi > *w) *w = wi;
		*h += TITLE_HEIGHT;
	}
}
