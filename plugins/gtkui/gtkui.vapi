[CCode (cprefix = "Gtkui", lower_case_cprefix = "gtkui_")]
namespace Gtkui {
	[CCode (cheader_filename = "drawing.h")]
        public static unowned Gdk.Color get_bar_foreground_color ();
        public static unowned Gdk.Color get_bar_background_color ();
        public static unowned void init_theme_colors ();
}

