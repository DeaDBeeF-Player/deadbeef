/*
    SuperEQ GTK Widget for for DeaDBeeF
    Copyright (C) 2010 Viktor Semykin <thesame.ml@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

// sripped down and polished by Alexey Yakovenko <waker@users.sourceforge.net>

const string[] freqs = {
    "55 Hz","77 Hz","110 Hz","156 Hz","220 Hz","311 Hz","440 Hz","622 Hz","880 Hz",
    "1.2 kHz","1.8 kHz","2.5 kHz","3.5 kHz","5 kHz","7 kHz","10 kHz","14 kHz","20 kHz"
};

namespace Ddb {
    public class Equalizer : Gtk.DrawingArea
    {
        public signal void on_changed ();

        private double[] values = new double [bands];
        private double preamp = 0.5;

        private int mouse_y;
        
        private bool curve_hook = false;
        private bool preamp_hook = false;

        private int margin_bottom = -1;
        private int margin_left = -1;
        static const int spot_size = 3;
        static const int bands = 18;

//        Gdk.Cursor moving_cursor = new Gdk.Cursor (Gdk.CursorType.FLEUR);
//        Gdk.Cursor updown_cursor = new Gdk.Cursor (Gdk.CursorType.double_ARROW);
        Gdk.Cursor pointer_cursor = new Gdk.Cursor (Gdk.CursorType.LEFT_PTR);

        construct
        {
            add_events (Gdk.EventMask.BUTTON_PRESS_MASK
                | Gdk.EventMask.BUTTON_RELEASE_MASK
                | Gdk.EventMask.LEAVE_NOTIFY_MASK
                | Gdk.EventMask.POINTER_MOTION_MASK);

            margin_bottom = (int)(Pango.units_to_double (get_style ().font_desc.get_size ()) * Gdk.Screen.get_default ().get_resolution () / 72 + 4);
            margin_left = margin_bottom * 4;

            color_changed ();
        }

        public override bool
        expose_event (Gdk.EventExpose event)
        {
            Gdk.Color fore_bright_color = Gtkui.get_bar_foreground_color ();

            Gdk.Color c1 = fore_bright_color;
            Gdk.Color c2 = Gtkui.get_bar_background_color ();
            Gdk.Color fore_dark_color = c2;
            fore_dark_color.red += (int16)((c1.red - c2.red) * 0.5);
            fore_dark_color.green += (int16)((c1.green - c2.green) * 0.5);
            fore_dark_color.blue += (int16)((c1.blue - c2.blue) * 0.5);

            int width = allocation.width;
            int height = allocation.height;

            Gdk.Drawable d = get_window();
            var gc = d.create_gc (Gdk.GCValues(), 0);

            gc.set_rgb_fg_color (fore_dark_color);
            //drawing grid:
            double step = (double)(width - margin_left) / (double)(bands+1);
            int i;
            for (i = 0; i < bands; i++)
            {
                //does anyone know why this method is static?
                Gdk.draw_line (d, gc,
                    (int)((i+1)*step)+margin_left,
                    0,
                    (int)((i+1)*step)+margin_left,
                    height - margin_bottom);
            }

            double vstep = (double)(height-margin_bottom);
            for (double di=0; di < 2; di += 0.25)
            {
                Gdk.draw_line (d, gc,
                    margin_left,
                    (int)((di-preamp)*vstep),
                    width,
                    (int)((di-preamp)*vstep));
            }

            gc.set_rgb_fg_color (fore_bright_color);

            //drawing freqs:
            Pango.Layout l = create_pango_layout (null);
            var ctx = l.get_context ();

            var fd = get_style ().font_desc.copy ();

//            var fd = ctx.get_font_description ();
            fd.set_size ((int)(get_style ().font_desc.get_size () * 0.7));
            ctx.set_font_description (fd);
            for (i = 0; i < bands; i++)
            {
                l.set_text (freqs[i], (int)freqs[i].len());
                Pango.Rectangle ink, log;
                l.get_pixel_extents (out ink, out log);
                int offs = 2;
                if ((i % 2) != 0) {
                    offs += 2;
                }
                Gdk.draw_layout (d, gc, (int)((i+1)*step)+margin_left - ink.width/2, height-margin_bottom + offs, l);
            }
            fd.set_size ((int)(get_style ().font_desc.get_size ()));
            ctx.set_font_description (fd);
            
            //drawing db's:
            l.set_width (margin_left-1);
            l.set_alignment (Pango.Alignment.RIGHT);

            int fontsize = (int)(Pango.units_to_double (fd.get_size ()) * Gdk.Screen.get_default ().get_resolution () / 72);

            if ((mouse_y != -1) && (mouse_y < height - margin_bottom))
            {
                double db = scale((double)(mouse_y-1) / (double)(height - margin_bottom - 2));
                string tmp = "%s%.1fdB".printf (db > 0 ? "+" : "", db);
                l.set_text (tmp, (int)tmp.len());
                Gdk.draw_layout (d, gc, margin_left-1, mouse_y-3, l);
            }
            
            string tmp;
            double val = scale(1);
            tmp = "%s%.1fdB".printf (val > 0 ? "+" : "", val);
            l.set_text (tmp, (int)tmp.len());
            Gdk.draw_layout (d, gc, margin_left-1, height-margin_bottom-fontsize, l);

            val = scale(0);
            tmp = "%s%.1fdB".printf (val > 0 ? "+" : "", val);
            l.set_text (tmp, (int)tmp.len());
            Gdk.draw_layout (d, gc, margin_left-1, 1, l);

            l.set_text ("+0dB", 4);
            Gdk.draw_layout (d, gc, margin_left-1, (int)((1-preamp)*(height-margin_bottom))-fontsize/2, l);

            l.set_text ("preamp", 6);
            l.set_alignment (Pango.Alignment.LEFT);
            Gdk.draw_layout (d, gc, 1, height-margin_bottom+2, l);

            d.draw_rectangle (gc, false, margin_left, 0, width-margin_left-1, height-margin_bottom-1);
            gc.set_line_attributes (2, Gdk.LineStyle.SOLID, Gdk.CapStyle.NOT_LAST, Gdk.JoinStyle.MITER);
            
            //draw preamp
            gc.set_clip_rectangle ({0, (int)(preamp * (height-margin_bottom)), 11, height});

            gc.set_rgb_fg_color (fore_bright_color);
            int count = (int)((height-margin_bottom) / 6)+1;
            for (int j = 0; j < count; j++)
                d.draw_rectangle (
                    gc,
                    true,
                    1,
                    height-margin_bottom-j*6 - 6,
                    11,
                    4);

            gc.set_clip_rectangle ({margin_left+1, 1, width-margin_left-2, height-margin_bottom-2});

            //drawing bars:
            gc.set_rgb_fg_color (fore_bright_color);

            int bar_w = 11;
            if (step < bar_w)
                bar_w = (int)step-1;


            for (i = 0; i < bands; i++)
            {
                gc.set_clip_rectangle ({
                    (int)((i+1)*step)+margin_left - bar_w/2,
                    (int)(values[i] * (height-margin_bottom)),
                    11,
                    height});
                count = (int)((height-margin_bottom) * (1-values[i]) / 6)+1;
                for (int j = 0; j < count; j++)
                    d.draw_rectangle (
                        gc,
                        true,
                        (int)((i+1)*step)+margin_left - bar_w/2,
                        height-margin_bottom-j*6 - 6,
                        bar_w,
                        4);
            }
            gc.set_clip_rectangle ({0, 0, width, height});

            //drawing mouse coordinates:
            gc.set_line_attributes (1, Gdk.LineStyle.ON_OFF_DASH, Gdk.CapStyle.NOT_LAST, Gdk.JoinStyle.MITER);
            Gdk.draw_line (d, gc, margin_left+1, mouse_y, width, mouse_y);
            
            return false;
        }
 
        private inline double
        scale (double val)
        {
            double k = -40;
            double d = 20;
            return (val + preamp - 0.5) * k + d;
        }

        private bool
        in_curve_area (double x, double y)
        {
            return
                x > margin_left &&
                x < allocation.width-1 &&
                y > 1 &&
                y < allocation.height-margin_bottom;
        }

        private void
        update_eq_drag (double x, double y) {
            double band_width = (double)(allocation.width - margin_left) / (double)(bands+1);
            int band = (int)GLib.Math.floor ((x - margin_left) / band_width - 0.5);
            if (band < 0) {
                band = 0;
            }
            if (band >= bands) {
                band = band-1;
            }
            if (band >= 0 && band < bands) {
                values[band] = y / (double)(allocation.height - margin_bottom);
                if (values[band] > 1) {
                    values[band] = 1;
                }
                else if (values[band] < 0) {
                    values[band] = 0;
                }
                on_changed ();
            }
        }

        /* Mouse button got pressed over widget */
        public override bool
        button_press_event (Gdk.EventButton event)
        {
            if (in_curve_area ((int)event.x, (int)event.y))
            {
                curve_hook = true;
                update_eq_drag ((int)event.x, (int)event.y);
                mouse_y = (int)event.y;
                queue_draw ();
                return false;
            }

            if (event.x <= 11 &&
                event.y > 1 &&
                event.y <= allocation.height-margin_bottom &&
                event.button == 1
                )
            {
                preamp = event.y / (double)(allocation.height - margin_bottom);
                on_changed ();
                preamp_hook = true;
                mouse_y = (int)event.y;
                queue_draw ();
            }

            return false;
        }

        /* Mouse button got released */
        public override bool
        button_release_event (Gdk.EventButton event)
        {
            curve_hook = false;
            preamp_hook = false;
            get_window().set_cursor (pointer_cursor);
            return false;
        }

        public override bool
        leave_notify_event (Gdk.EventCrossing event)
        {
            mouse_y = -1;
            queue_draw();
            return false;
        }

        /* Mouse pointer moved over widget */
        public override bool
        motion_notify_event (Gdk.EventMotion event) {
            double y = event.y / (double)(allocation.height - margin_bottom);
            if (y < 0) y = 0;
            if (y > 1) y = 1;

            if (preamp_hook)
            {
                preamp = y;
                on_changed ();
                queue_draw();
                return false;
            }
            
            if (!in_curve_area ((int)event.x, (int)event.y))
                mouse_y = -1;
            else
                mouse_y = (int)event.y;

            if (curve_hook)
            {
                update_eq_drag ((int)event.x, (int)event.y);
                mouse_y = (int)event.y;
            }
            queue_draw ();
            return false;
        }

        public void
        set_band (int band, double v) {
            values[band] = 1 - (v + 20.0) / 40.0;
        }

        public double
        get_band (int band) {
            return ((1 - values[band]) * 40.0) - 20.0;
        }

        public void
        set_preamp (double v) {
            preamp = 1 - (v + 20.0) / 40.0;
        }

        public double
        get_preamp () {
            return ((1 - preamp) * 40.0) - 20.0;
        }

        public void
        color_changed () {
            Gtkui.init_theme_colors ();
            modify_bg (Gtk.StateType.NORMAL, Gtkui.get_bar_background_color ());
        }
    }
}
