#include <stdlib.h>
#include <allegro.h>
#include "options.h"



void load_options(void)
{
	{
		char ef[256], cf[256];

		get_executable_name(ef, 256);
		replace_filename(cf, ef, "studio.ini", 256);

		set_config_file(cf);
	}

	opt.gfx_w = get_config_int("options", "gfx_w", DEF_GFX_W);
	opt.gfx_h = get_config_int("options", "gfx_h", DEF_GFX_H);

	atexit(&save_options);
}



void save_options(void)
{
	set_config_int("options", "gfx_w", opt.gfx_w);
	set_config_int("options", "gfx_h", opt.gfx_h);
}

