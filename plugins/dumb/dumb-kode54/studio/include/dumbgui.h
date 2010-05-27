#ifndef INCLUDED_DUMBGUI_H
#define INCLUDED_DUMBGUI_H


/* GUI_FINISHED_WITH: Set this in your update, key or mouse function when you
 *                    don't need the object any more. It will be destroyed
 *                    before redrawing or any subsequent updates take place.
 * GUI_OTHER:         Any powers of two greater than or equal to this are
 *                    free for use by objects for specific purposes.
 */
#define GUI_FINISHED_WITH 1
#define GUI_OTHER         2


typedef struct GUI GUI;


/* GUI_CREATE:    Create a GUI object.
 * GUI_DESTROY:   Destroy the GUI object.
 * GUI_KEY:       Respond to a key.
 * GUI_UPDATE:    Process any real-time activity.
 * GUI_DRAW:      Draw any parts of the GUI that have changed. This may be
 *                called more than once with different clipping rectangles,
 *                so don't clear any flags here.
 * GUI_DRAWN:     Clear any redraw flags.
 * GUI_CHANGED_*: Set flags to redraw parts of the object as necessary for
 *                the change that has occurred.
 */
typedef void (*GUI_CREATE)(GUI *gui, void *param);
typedef void (*GUI_DESTROY)(GUI *gui);
typedef void (*GUI_KEY)(GUI *gui, int k);
typedef void (*GUI_UPDATE)(GUI *gui);
typedef void (*GUI_DRAW)(GUI *gui);
typedef void (*GUI_DRAWN)(GUI *gui);
typedef void (*GUI_CHANGED_ALL)(GUI *gui);
typedef void (*GUI_CHANGED_ACTIVE)(GUI *gui);


typedef struct GUI_COMMANDS
{
	GUI_CREATE create;
	GUI_DESTROY destroy;
	GUI_KEY key;
	GUI_UPDATE update;
	GUI_DRAW draw;
	GUI_DRAWN drawn;
	GUI_CHANGED_ALL changed_all;
	GUI_CHANGED_ACTIVE changed_active;
}
GUI_COMMANDS;


struct GUI
{
	GUI_COMMANDS *com;

	GUI *parent;

	int x, y, w, h;
	int flags;

	void *data;
};


extern GUI *gui_active;

void gui_set_active(GUI *gui);

GUI *gui_create(GUI_COMMANDS *com, GUI *parent, int x, int y, int w, int h, void *param);
void gui_destroy(GUI *gui);
void gui_key(GUI *gui, int k);
void gui_update(GUI *gui);
void gui_draw(GUI *gui);
void gui_draw_void(void *gui);
void gui_drawn(GUI *gui);
void gui_changed_all(GUI *gui);
void gui_changed_active(GUI *gui);


#endif /* INCLUDED_DUMBGUI_H */

