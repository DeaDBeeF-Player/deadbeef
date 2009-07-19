#include <allegro.h>

#include "options.h"
#include "guitop.h"
#include "guiproc.h"
#include "dumbdesk.h"



int the_time;

volatile int true_time;
static int max_time;



static void timer_handler(void)
{
	if (true_time < max_time)
		true_time++;
}
END_OF_STATIC_FUNCTION(timer_handler);



#define MAX_SKIP 20



/*
Every GUI has a reference to its parent - except the top one.

Main loop:
   Test keyboard.
      Esc makes parent GUI active.
      Other keys go to active GUI, which can do as it pleases.
   Test mouse.
      Mouse clicks go to the top GUI.
         The top GUI may pass them down recursively.
            A GUI may be marked as mouse-active, for dragging.
   Update top GUI.
      Update functions are called recursively.
         This can be used for animation for example.
   If there's time, draw top GUI.
      Draw functions only draw areas marked for redrawing.
         Marked areas are unmarked once they have been drawn.
      Draw functions are called recursively.
         If a child has moved:
            It will call its parent for the vacated areas.*

The child will have been removed from the parent's list while
it was drawn, so recursive cycles cannot occur.
*/



void run_desktop(GUI_DESKTOP_PARAM *param)
{
	GUI *gui = gui_create(
		&gui_desktop_commands,
		NULL,
		0, 0, opt.gfx_w, opt.gfx_h,
		param
	);

	if (!gui)
		return;

	gui_active = gui;

	true_time = the_time = 0;
	max_time = MAX_SKIP;

	install_int_ex(timer_handler, BPS_TO_TIMER(100));

	for (;;) {
		gui_draw(gui);
		gui_drawn(gui);

		while (the_time >= true_time)
			yield_timeslice();

		while (the_time < true_time) {
			the_time++;

			/*
			Test keyboard.
				Esc makes parent GUI active.
				Other keys go to active GUI, which can do as it pleases.
			*/
			while (keypressed()) {
				int k = readkey();
				if (k >> 8 == KEY_ESC) {
					if (!gui_active->parent) {
						remove_int(timer_handler);
						return;
					}
					gui_set_active(gui_active->parent);
				} else
					gui_key(gui_active, k);
			}

			/*
			Test mouse.
				Mouse clicks go to the top GUI.
					The top GUI may pass them down recursively.
						A GUI may be marked as mouse-active, for dragging.
			*/

			/*
			Update top GUI.
				Update functions are called recursively.
					This can be used for animation for example.
			*/
			gui_update(gui);
		}

		max_time = the_time + MAX_SKIP;
	}

	gui_destroy(gui);
}



void initialise_guitop(void)
{
	LOCK_FUNCTION(timer_handler);
	LOCK_VARIABLE(true_time);
	LOCK_VARIABLE(max_time);
}
