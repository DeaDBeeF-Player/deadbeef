#include <stdlib.h>
#include <allegro.h>

#include "main.h"
#include "subclip.h"
#include "dumbgui.h"
#include "guiproc.h"
#include "dumbdesk.h"
#include "options.h"



static void gui_desktop_create(GUI *gui, void *param)
{
	GUI_DESKTOP_PARAM *desktop = param;

	GUI_DESKTOP_DATA *data = gui->data = malloc(sizeof(*data));

	if (!data)
		return;

	data->title = desktop->title;
	data->menu_bar = desktop->menu_bar;
	data->deskgui = NULL;
}



static void gui_desktop_destroy(GUI *gui)
{
	GUI_DESKTOP_DATA *data = gui->data;

	DESKGUI *deskgui = data->deskgui;

	while (deskgui) {
		DESKGUI *next = deskgui->next;
		gui_destroy(deskgui->gui);
		free(deskgui);
		deskgui = next;
	}

	free(data);
}



static void gui_desktop_key(GUI *gui, int k)
{
	switch (k >> 8) {
		case KEY_N:
			{
				GUI_DESKTOP_PARAM param2 = {"well.duh", NULL};
				GUI_DESKTOP_DATA *data = gui->data;
				DESKGUI *next = data->deskgui;
				data->deskgui = malloc(sizeof(*data->deskgui));
				if (!data->deskgui) break;
				data->deskgui->gui = gui_create(
					&gui_desktop_commands,
					gui,
					10 + rand() % 50, 30 + rand() % 50, 50 + rand() % 600, 50 + rand() % 350,
					&param2
				);
				if (!data->deskgui->gui) {
					free(data->deskgui);
					break;
				}
				data->deskgui->name = param2.title;
				data->deskgui->next = next;
				gui_set_active(data->deskgui->gui);
			}
	}
}



static void gui_desktop_update(GUI *gui)
{
	GUI_DESKTOP_DATA *data = gui->data;

	DESKGUI *deskgui = data->deskgui;

	while (deskgui) {
		gui_update(deskgui->gui);
		deskgui = deskgui->next;
	}
}



static void draw_desktop_itself(void *g)
{
	GUI *gui = g;
	GUI_DESKTOP_DATA *data = gui->data;

	if (data->deskgui) {

		/* Unlink child GUI. */
		DESKGUI *deskgui = data->deskgui;
		GUI *subgui = deskgui->gui;

		data->deskgui = deskgui->next;

		/* Child GUI outer bevel */
		draw_bevel(
			subgui->x - BORDER_SIZE - BEVEL_SIZE,
			subgui->y - BORDER_SIZE - BEVEL_SIZE,
			subgui->x + subgui->w + BORDER_SIZE + BEVEL_SIZE - 1,
			subgui->y + subgui->h + BORDER_SIZE + BEVEL_SIZE - 1,
			HIGHLIGHT, MIDTONE, SHADOW
		);

		/* Child GUI border */
		rectfill(screen, subgui->x - BORDER_SIZE, subgui->y - BORDER_SIZE, subgui->x + subgui->w + BORDER_SIZE - 1, subgui->y - 1, MIDTONE);
		rectfill(screen, subgui->x - BORDER_SIZE, subgui->y, subgui->x - 1, subgui->y + subgui->h - 1, MIDTONE);
		rectfill(screen, subgui->x + subgui->w, subgui->y, subgui->x + subgui->w + BORDER_SIZE - 1, subgui->y + subgui->h - 1, MIDTONE);
		rectfill(screen, subgui->x - BORDER_SIZE, subgui->y + subgui->h, subgui->x + subgui->w + BORDER_SIZE - 1, subgui->y + subgui->h + BORDER_SIZE - 1, MIDTONE);

		/* Child GUI */
		subclip(subgui->x, subgui->y, subgui->x + subgui->w, subgui->y + subgui->h,
			&gui_draw_void, subgui);

		/* Draw self, and other child GUIs, around this child GUI. */
		subclip(0, 0, opt.gfx_w, subgui->y - BORDER_SIZE - BEVEL_SIZE, &draw_desktop_itself, gui);
		subclip(0, subgui->y - BORDER_SIZE - BEVEL_SIZE, subgui->x - BORDER_SIZE - BEVEL_SIZE, subgui->y + subgui->h + BORDER_SIZE + BEVEL_SIZE, &draw_desktop_itself, gui);
		subclip(subgui->x + subgui->w + BORDER_SIZE + BEVEL_SIZE, subgui->y - BORDER_SIZE - BEVEL_SIZE, opt.gfx_w, subgui->y + subgui->h + BORDER_SIZE + BEVEL_SIZE, &draw_desktop_itself, gui);
		subclip(0, subgui->y + subgui->h + BORDER_SIZE + BEVEL_SIZE, opt.gfx_w, opt.gfx_h, &draw_desktop_itself, gui);

		/* Link child GUI back in. */
		data->deskgui = deskgui;

		return;
	}

	if (gui->flags & DESKTOP_CHANGED_REST)
		/* Desktop area */
		clear_to_color(screen, DESKTOP);
}



static int desktop_x;
static int desktop_y;
static GUI *desktop_gui;



static void draw_desktop_title_bar(void *t)
{
	const char *title = t;
	clear_to_color(screen, desktop_gui == gui_active ? TITLE_BG_FOCUS : TITLE_BG);
	textout(screen, dat[THE_FONT].dat, title, desktop_x, desktop_y, desktop_gui == gui_active ? TITLE_FG_FOCUS : TITLE_FG);
}



static void draw_menu_bar(void *data)
{
	GUI_MENU_PARAM *menu_bar = data;

	clear_to_color(screen, MENU_BG);

	if (menu_bar) {
		int i;
		GUI_MENU_ENTRY *entry = menu_bar->entry;
		for (i = 0; i < menu_bar->n_entries; i++) {
			// If the first char is '\t', draw an arrow on the right.
			textout(screen, dat[THE_FONT].dat, entry->text,
				desktop_x + BORDER_SIZE, desktop_y + BORDER_SIZE, MENU_FG);
			desktop_x += BORDER_SIZE + text_length(dat[THE_FONT].dat, entry->text) + BORDER_SIZE;
			entry++;
		}
	} // else hide the damn thing?
}



static void gui_desktop_draw(GUI *gui)
{
	GUI_DESKTOP_DATA *data = gui->data;

	subclip(
		gui->x + BEVEL_SIZE,
		gui->y + TITLE_HEIGHT + TITLE_HEIGHT + BEVEL_SIZE,
		gui->x + gui->w - BEVEL_SIZE,
		gui->y + gui->h - BEVEL_SIZE,
		&draw_desktop_itself, gui
	);

	if (gui->flags & DESKTOP_CHANGED_TITLE) {
		/* Title bar */
		desktop_x = gui->x + BORDER_SIZE;
		desktop_y = gui->y + BORDER_SIZE;
		desktop_gui = gui;

		subclip(gui->x, gui->y, gui->x + gui->w, gui->y + TITLE_HEIGHT,
			&draw_desktop_title_bar, (void *)data->title);
	}

	if (gui->flags & DESKTOP_CHANGED_REST) {
		/* Separation between title bar and inner bevel */
		//rectfill(screen, gui->x, gui->y + TITLE_HEIGHT, gui->x + gui->w - 1, gui->y + TITLE_HEIGHT + BORDER_SIZE - 1, MIDTONE);

		/* Menu bar */
		desktop_x = gui->x;
		desktop_y = gui->y + TITLE_HEIGHT;

		subclip(
			gui->x, gui->y + TITLE_HEIGHT,
			gui->x + gui->w, gui->y + TITLE_HEIGHT + TITLE_HEIGHT,
			&draw_menu_bar, data->menu_bar
		);

		/* Inner bevel */
		draw_bevel(gui->x, gui->y + TITLE_HEIGHT + TITLE_HEIGHT, gui->x + gui->w - 1, gui->y + gui->h - 1, SHADOW, MIDTONE, HIGHLIGHT);
	}
}



static void gui_desktop_drawn(GUI *gui)
{
	GUI_DESKTOP_DATA *data = gui->data;

	if (data->deskgui) {
		/* Mark child GUIs as drawn. */
		DESKGUI *deskgui = data->deskgui;
		while (deskgui) {
			gui_drawn(deskgui->gui);
			deskgui = deskgui->next;
		}
	}

	/* Mark self as drawn. */
	gui->flags &= ~DESKTOP_CHANGED_ALL;
}



static void gui_desktop_changed_all(GUI *gui)
{
	gui->flags |= DESKTOP_CHANGED_ALL;

	{
		GUI_DESKTOP_DATA *data = gui->data;
		DESKGUI *deskgui = data->deskgui;
		while (deskgui) {
			gui_changed_all(deskgui->gui);
			deskgui = deskgui->next;
		}
	}
}



static void gui_desktop_changed_active(GUI *gui)
{
	gui->flags |= DESKTOP_CHANGED_TITLE;
}



GUI_COMMANDS gui_desktop_commands = {
	&gui_desktop_create,
	&gui_desktop_destroy,
	&gui_desktop_key,
	&gui_desktop_update,
	&gui_desktop_draw,
	&gui_desktop_drawn,
	&gui_desktop_changed_all,
	&gui_desktop_changed_active
};
