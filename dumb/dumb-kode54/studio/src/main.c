#include <stdlib.h>
#include <errno.h>
#include <allegro.h>
#include "dumb.h"

#include "options.h"
#include "guitop.h"
#include "dumbdesk.h"



DATAFILE *dat;



static int load_studio_datafile(void)
{
	char ef[256], df[256];

	get_executable_name(ef, 256);
	replace_filename(df, ef, "studio.dat", 256);

	dat = load_datafile(df);

	if (!dat)
		return 1;

	return 0;
}



static GUI_MENU_ENTRY main_menu_bar_entries[] = {
	{"File", NULL, NULL},
	{"Edit", NULL, NULL},
	{"View", NULL, NULL}
};

static GUI_MENU_PARAM main_menu_bar = {3, main_menu_bar_entries};

static GUI_DESKTOP_PARAM desktop_param = {"DUMB Studio v0.001", &main_menu_bar};



int main(void)//(int argc, char *argv[])
{
	int old_gfx_w, old_gfx_h;
	char old_allegro_error[ALLEGRO_ERROR_SIZE];

	if (allegro_init())
		return 1;

	if (install_timer()) {
		allegro_message("Unable to initialise timer\n");
		return 1;
	}

	if (install_keyboard()) {
		allegro_message("Unable to initialise keyboard\n");
		return 1;
	}

	/*
	if (install_dumb(&errno, &atexit)) {
		allegro_message("Unable to initialise the DUMB library\n");
		return 1;
	}

	register_sigtype_sample();
	register_sigtype_combining();
	register_sigtype_stereopan();
	register_sigtype_sequence();
	*/

	initialise_guitop();

	if (load_studio_datafile()) {
		allegro_message("Unable to load studio.dat\n");
		return 1;
	}

	load_options();

	old_gfx_w = opt.gfx_w;
	old_gfx_h = opt.gfx_h;

	while (set_gfx_mode(GFX_AUTODETECT, opt.gfx_w, opt.gfx_h, 0, 0)) {
		if (opt.gfx_w == DEF_GFX_W && opt.gfx_h == DEF_GFX_H) {
			if (opt.gfx_w == old_gfx_w && opt.gfx_h == old_gfx_h) {
				allegro_message(
					"Unable to set graphics mode "DEF_GFX_STR"\n%s\n",
					allegro_error
				);
			} else {
				allegro_message(
					"Unable to set graphics mode %dx%d\n%s\n"
					"Unable to revert to graphics mode "DEF_GFX_STR"\n%s\n",
					old_gfx_w, old_gfx_h,
					old_allegro_error,
					allegro_error
				);
			}
			return 1;
		}

		opt.gfx_w = DEF_GFX_W;
		opt.gfx_h = DEF_GFX_H;
		ustrncpy(old_allegro_error, allegro_error, ALLEGRO_ERROR_SIZE);
	}

	text_mode(-1);

	run_desktop(&desktop_param);

	return 0;
}
END_OF_MAIN();

