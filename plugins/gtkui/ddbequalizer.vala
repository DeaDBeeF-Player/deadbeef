static const int spot_size = 3;
static const int bands = 18;

const string[] freqs = {
    "55 Hz","77 Hz","110 Hz","156 Hz","220 Hz","311 Hz","440 Hz","622 Hz","880 Hz",
    "1.2 kHz","1.8 kHz","2.5 kHz","3.5 kHz","5 kHz","7 kHz","10 kHz","14 kHz","20 kHz"
};

namespace Ddb {
    public class Equalizer : Gtk.DrawingArea
    {
        public signal void on_changed (double[] values);

        class Point
        {
            public double x;
            public double y;
        }

        private List <Point> points = new List <Point> ();
        private unowned List <Point> current_point = null;

//        private Gdk.Color back_color;
//        private Gdk.Color fore_bright_color;
//        private Gdk.Color fore_dark_color;

        private double[] values = new double [bands];
        private double preamp;

        private int mouse_y;
        
        private bool snap = false;
        private bool aa_mode = false;
        private bool draw_envelope = false;

        private bool curve_hook = false;
        private bool preamp_hook = false;

        private Gtk.Menu menu = null;

        private int margin_bottom = -1;
        private int margin_left = -1;

        Gdk.Cursor moving_cursor = new Gdk.Cursor (Gdk.CursorType.FLEUR);
//        Gdk.Cursor updown_cursor = new Gdk.Cursor (Gdk.CursorType.DOUBLE_ARROW);
        Gdk.Cursor pointer_cursor = new Gdk.Cursor (Gdk.CursorType.LEFT_PTR);

        construct
        {
            add_events (Gdk.EventMask.BUTTON_PRESS_MASK
                | Gdk.EventMask.BUTTON_RELEASE_MASK
                | Gdk.EventMask.LEAVE_NOTIFY_MASK
                | Gdk.EventMask.POINTER_MOTION_MASK);

            modify_bg (Gtk.StateType.NORMAL, get_style ().fg[Gtk.StateType.NORMAL]);

            recalc_values();
            margin_bottom = (int)(Pango.units_to_double (get_style ().font_desc.get_size ())* Gdk.Screen.get_default ().get_resolution () / 72 + 4);
            margin_left = margin_bottom * 4;
            preamp = 0.5;

            set_snap (true);

            menu = new Gtk.Menu ();

            var checkitem = new Gtk.CheckMenuItem.with_label ("Antialiasing");
            checkitem.show();
            checkitem.toggled.connect (aa_mode_changed);
            menu.append (checkitem);

            var mode_item = new Gtk.MenuItem();
            mode_item.show ();
            mode_item.label = "mode";
            menu.append (mode_item);

            var mode_menu = new Gtk.Menu ();

            var group = new GLib.SList <Gtk.RadioMenuItem> ();

            var thesame_item = new Gtk.RadioMenuItem.with_label (group, "thesame");
            thesame_item.show();
            mode_menu.append (thesame_item);

            var waker_item = new Gtk.RadioMenuItem.with_label_from_widget (thesame_item, "waker");
            waker_item.show();
            waker_item.toggled.connect (mode_changed);
            mode_menu.append (waker_item);

            mode_item.set_submenu (mode_menu);
        }

        public void
        aa_mode_changed (Gtk.CheckMenuItem item)
        {
            aa_mode = item.active;
            queue_draw ();
        }

        public void
        mode_changed (Gtk.CheckMenuItem item)
        {
            set_snap (item.active);
        }

        private void
        set_snap (bool new_snap)
        {
            snap = new_snap;

            if (snap)
            {
                double step = 1.0 / (double)(bands+1);

                if (points.length() > 0)
                {
                    unowned List <Point> iter;
                    for (iter = points.next; iter != null; iter = iter.next)
                        points.remove_link (iter.prev);
                    points.remove_link (points);
                }

                for (int i = 0; i < bands; i++)
                {
                    Point point = new Point ();
                    point.x = ((double)i+1)*step;
                    point.y = values[i];
                    points.prepend (point);
                }
                points.reverse ();
            }
        }
        
        private Gdk.Point
        abs_to_screen (double x, double y)
        {
            return Gdk.Point () {
                x = (int)(x * (this.allocation.width-margin_left))+margin_left,
                y = (int)(y * (this.allocation.height-margin_bottom))
            };
        }

        private void
        abs_to_screen_d (double x, double y, out double sx, out double sy)
        {
            sx = (int)(x * (this.allocation.width-margin_left))+margin_left;
            sy = (int)(y * (this.allocation.height-margin_bottom));
        }

/*        private double
        cubic (double y0, double y1, double y2, double y3, double mu)
        {
           double a0,a1,a2,a3,mu2;

           mu2 = mu*mu;
           a0 = y3 - y2 - y0 + y1;
           a1 = y0 - y1 - a0;
           a2 = y2 - y0;
           a3 = y1;

           return (a0*mu*mu2+a1*mu2+a2*mu+a3);
        }*/

        private double
        cubic (double y0, double y1, double y2, double y3, double mu)
        {
            return 0.5 *((2 * y1) +
                (-y0 + y2) * mu +
                (2*y0 - 5*y1 + 4*y2 - y3) * mu*mu +
                (-y0 + 3*y1- 3*y2 + y3) * mu*mu*mu);
        }

        public override bool
        expose_event (Gdk.EventExpose event)
        {
            Gdk.Color fore_bright_color = Gtkui.get_bar_foreground_color ();
            Gdk.Color fore_dark_color = Gtkui.get_bar_foreground_color ();

            int width = this.allocation.width;
            int height = this.allocation.height;

            Gdk.Point[] gpoints = new Gdk.Point [points.length()+2];
            gpoints[0] = {margin_left, (height-margin_bottom) / 2};
            int i = 1;
            foreach (var p in this.points)
            {
                gpoints[i] = abs_to_screen (p.x, p.y);
                if (gpoints[i].x >= width)
                    gpoints[i].x = width - 1;
                i++;
            }
            gpoints[i] = {width-1, (height-margin_bottom) / 2};

            Gdk.Drawable d = get_window();
            var gc = d.create_gc (Gdk.GCValues(), 0);

            gc.set_rgb_fg_color (fore_dark_color);
            //drawing grid:
            double step = (double)(width - margin_left) / (double)(bands+1);
            for (i = 0; i < bands; i++)
            {
                //does anyone know why this method is static?
                Gdk.draw_line (d, gc,
                    (int)((i+1)*step)+margin_left,
                    0,
                    (int)((i+1)*step)+margin_left,
                    height - margin_bottom);
            }

            //double vstep = 1.0 / (double)(height-margin_bottom);
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
            var fd = ctx.get_font_description ();
            ctx.set_font_description (fd);
            for (i = 0; i < bands; i++)
            {
                l.set_text (freqs[i], (int)freqs[i].len());
                Gdk.draw_layout (d, gc, (int)((i+1)*step-5)+margin_left, height-margin_bottom+2, l);
            }
            
            //drawing db's:
            l.set_width (margin_left-1);
            l.set_alignment (Pango.Alignment.RIGHT);

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
            Gdk.draw_layout (d, gc, margin_left-1, height-margin_bottom-6, l);

            val = scale(0);
            tmp = "%s%.1fdB".printf (val > 0 ? "+" : "", val);
            l.set_text (tmp, (int)tmp.len());
            Gdk.draw_layout (d, gc, margin_left-1, 1, l);

            l.set_text ("0dB", 4);
            Gdk.draw_layout (d, gc, margin_left-1, (int)((1-preamp)*(height-margin_bottom))-3, l);

            l.set_text ("preamp", 6);
            l.set_alignment (Pango.Alignment.LEFT);
            Gdk.draw_layout (d, gc, 1, height-margin_bottom+2, l);

            d.draw_rectangle (gc, false, margin_left, 0, width-margin_left-1, height-margin_bottom-1);
            gc.set_line_attributes (2, Gdk.LineStyle.SOLID, Gdk.CapStyle.NOT_LAST, Gdk.JoinStyle.MITER);
            
            //draw preamp
            gc.set_clip_rectangle ({0, (int)(preamp * (height-margin_bottom)), 11, height});

            gc.set_rgb_fg_color (fore_dark_color);
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
            gc.set_rgb_fg_color (fore_dark_color);

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

            if (draw_envelope) {
                //drawing curve:
                gc.set_rgb_fg_color (fore_bright_color);
                Gdk.Point gp;
                uint pcount = points.length();
                double[] ys = new double [pcount];
                double[] xs = new double [pcount];
                i=0;
                foreach (var p in this.points)
                {
                    gp = abs_to_screen (p.x, p.y);
                    d.draw_rectangle (gc, true, gp.x-spot_size, gp.y-spot_size, spot_size*2, spot_size*2);
                    xs[i] = p.x;
                    ys[i] = p.y;
                    i++;
                }

                Cairo.Context cairo = aa_mode ? Gdk.cairo_create (d) : null;

                int prev_x = 0;
                int prev_y = 0;

                if (pcount > 0)
                {
                    gp = abs_to_screen (xs[0], ys[0]);
                    if (aa_mode)
                        cairo.move_to (margin_left, gp.y);
                    else
                        Gdk.draw_line (d, gc, margin_left, gp.y, gp.x, gp.y);
                    prev_x = gp.x;
                    prev_y = gp.y;
                }

                if (pcount >= 2)
                {
                    for (i = 0; i < pcount-1; i++)
                    {
                        //stdout.printf ("%d\n", (int)((xs[i+1]-xs[i])*width));
                        if ((int)((xs[i+1]-xs[i])*width) <= 5)
                        {
                            Gdk.Point gp2 = abs_to_screen (xs[i], ys[i]);
                            gp = abs_to_screen (xs[i+1], ys[i+1]);
                            Gdk.draw_line (d, gc, gp2.x, gp2.y, gp.x, gp.y);
                            prev_x = gp2.x;
                            prev_y = gp2.y;
                            continue;
                        }
                        //int pts = (int)((double)((xs[i+1] - xs[i]) * allocation.width) / 5.0);
                        //step = (double)(xs[i+1] - xs[i])/(double)pts;

                        double dx = (xs[i+1] - xs[i])*width;
                        double dy = (ys[i+1] - ys[i])*height;
                        int pts = (int)(GLib.Math.sqrt (dx*dx + dy*dy) / 5.0);
                        //stdout.printf ("%f %f %d\n", dx, dy, pts);
                        step = (double)(xs[i+1] - xs[i])/(double)pts;

                        for (int ii = 0; ii <= pts; ii++)
                        {
                            double y;

                            if (i == 0 && i == pcount-2) //case when we have only two points
                                y = cubic (ys[0], ys[0], ys[1], ys[1], (double)ii/(double)pts);

                            else if (i == 0)
                                y = cubic (ys[0], ys[0], ys[1], ys[2], (double)ii/(double)pts);

                            else if (i == pcount-2)
                                y = cubic (ys[i-1], ys[i], ys[i+1], ys[i+1], (double)ii/(double)pts);

                            else
                                y = cubic (ys[i-1], ys[i], ys[i+1], ys[i+2], (double)ii/(double)pts);
                            if (y < 0) y = 0;
                            if (y > 1) y = 1;

                            if (aa_mode)
                            {
                                double sx, sy;
                                abs_to_screen_d (ii*step+xs[i], y, out sx, out sy);
                                cairo.line_to (sx, sy);
                                //                            prev_x = gp.x;
                                //                            prev_y = gp.y;
                            }
                            else
                            {
                                gp = abs_to_screen (ii*step+xs[i], y);

                                if (gp.y < 2) gp.y = 2;
                                if (gp.y > height-margin_bottom-2) gp.y = height-margin_bottom-2;

                                Gdk.draw_point (d, gc, gp.x, gp.y);
                                //Gdk.draw_line (d, gc, prev_x, prev_y, gp.x, gp.y);
                                prev_x = gp.x;
                                prev_y = gp.y;
                            }
                        }
                    }
                }
                if (pcount > 0)
                {
                    //                gp = abs_to_screen (xs[0], ys[0]);
                    //                cairo.move_to (margin_left, gp.y);
                    //                Gdk.draw_line (d, gc, margin_left, gp.y, gp.x, gp.y);

                    gp = abs_to_screen (xs[pcount-1], ys[pcount-1]);
                    if (aa_mode)
                        cairo.line_to (width-1, gp.y);
                    else
                        Gdk.draw_line (d, gc, gp.x, gp.y, width-1, gp.y);
                }
                if (aa_mode)
                {
                    cairo.set_source_rgb (
                            (double)fore_bright_color.red / (double)0xffff,
                            (double)fore_bright_color.green / (double)0xffff,
                            (double)fore_bright_color.blue / (double)0xffff);
                    cairo.stroke();
                }
                if (pcount == 0)
                {
                    Gdk.draw_line (d, gc, margin_left, (height-margin_bottom)/2, width-1, (height-margin_bottom)/2);
                }
            }

            //drawing mouse coordinates:
            gc.set_line_attributes (1, Gdk.LineStyle.ON_OFF_DASH, Gdk.CapStyle.NOT_LAST, Gdk.JoinStyle.MITER);
            Gdk.draw_line (d, gc, margin_left+1, mouse_y, width, mouse_y);
            
            return false;
        }
        
        //FIXME: I'm not sure returning value thru instance property is good
        private bool
        get_point_at (double x, double y)
        {
            bool ret = false;

            unowned List <Point> iter;

            double ss_x = (double)spot_size / (double)allocation.width;
            double ss_y = (double)spot_size / (double)allocation.height;

            for (iter = points; iter != null; iter = iter.next)
            {
                if (GLib.Math.fabs (iter.data.x - x) <= ss_x &&
                    GLib.Math.fabs (iter.data.y - y) <= ss_y)
                {
                    current_point = iter;
                    ret = true;
                    break;
                }
            }
            return ret;
        }

        private inline double
        scale (double val)
        {
            double k = -40;
            double d = 20;
            return (val+preamp-0.5) * k + d;
        }

        private void
        recalc_values ()
        {
            uint pcount = points.length();
            double[] ys = new double [pcount];
            double[] xs = new double [pcount];
            int i=0;
            foreach (var p in this.points)
            {
                xs[i] = p.x;
                ys[i] = p.y;
                i++;
            }

            if (pcount == 0)
            {
                for (i=0; i < bands; i++)
                    values[i] = 0.5;
            }
            else if (pcount == 1)
            {
                for (i=0; i < bands; i++)
                    values[i] = ys[0];
            }
            else
            {
                int pi = 0;
                for (i = 0; i < bands; i++)
                {
                    double x = (double)(i+1)/(double)(bands+1);
                    double y = 0;

                    if (xs[pi] > x) //before first point
                    {
                        values[i] = ys[pi];
                        continue;
                    }

                    if ((xs[pi+1] < x) && (pi < pcount-1)) //passed to next point
                        pi++;

                    if (pi == pcount-1) //after last point
                    {
                        values[i] = ys[pcount-1];
                        continue;
                    }

                    if (pi == 0 && pi == pcount-2) //two-points case
                        y = cubic (ys[pi], ys[pi], ys[pi+1], ys[pi+1],
                            (x - xs[pi])/(xs[pi+1] - xs[pi]));

                    else if (pi == 0)
                        y = cubic (ys[pi], ys[pi], ys[pi+1], ys[pi+2],
                            (x - xs[pi])/(xs[pi+1] - xs[pi]));

                    else if (pi == pcount-2)
                        y = cubic (ys[pi-1], ys[pi], ys[pi+1], ys[pi+1],
                            (x - xs[pi])/(xs[pi+1] - xs[pi]));

                    else
                        y = cubic (ys[pi-1], ys[pi], ys[pi+1], ys[pi+2],
                            (x - xs[pi])/(xs[pi+1] - xs[pi]));
                    if (y < 0) y = 0;
                    if (y > 1) y = 1;
                    values[i] = y;
                }
            }
            double[] scaled_values = new double[bands];
            for (i = 0; i < bands; i++)
                scaled_values[i] = scale (values[i]);
            on_changed (scaled_values);
        }

        private void
        snap_move (double x, double y)
        {
            double step = 1.0 / (double)(bands+1);
            int idx = (int)((x-step/2)/step);
            if (idx < bands && idx >= 0)
            {
                current_point = points.nth (idx);
                current_point.data.y = y;
            }
        }

        private void
        handle_curve_click (Gdk.EventButton event)
        {
            double x = (double)(event.x - margin_left) / (double)(allocation.width - margin_left);
            double y = event.y / (double)(allocation.height - margin_bottom);
            
            if (event.button == 1)
            {
                /* Handling left button: moving points */
                if (snap)
                    snap_move (x, y);
                else
                {
                    if (!get_point_at (x, y))
                    {
                        var point = new Point();
                        if (points == null)
                        {
                            points.append (point);
                            current_point = points;
                        }
                        else if (points.data.x > x)
                        {
                            points.prepend (point);
                            current_point = points;
                        }
                        else
                        {
                            var found = false;
                            for (unowned List <Point> i = points; i.next != null; i = i.next)
                                if (i.data.x < x && i.next.data.x > x)
                                {
                                    points.insert_before (i.next, point);
                                    current_point = i.next;
                                    found = true;
                                    break;
                                }
                            if (!found)
                            {
                                points.append (point);
                                current_point = points.last();
                            }
                        }
                    }
                    current_point.data.x = x;
                    current_point.data.y = y;
                }
                recalc_values();
                get_window().set_cursor (moving_cursor);
                queue_draw ();
            }
            else if (event.button == 3)
            {
                /* Handling right button: removing points */
                if (snap)
                    return;
                if (get_point_at (x, y))
                {
                    points.remove (current_point.data);
                    recalc_values();
                    queue_draw ();
                }
                queue_draw();
            }
        }
        
        private bool
        in_curve_area (int x, int y)
        {
            return
                x > margin_left &&
                x < allocation.width-1 &&
                y > 1 &&
                y < allocation.height-margin_bottom;
        }

        /* Mouse button got pressed over widget */
        public override bool
        button_press_event (Gdk.EventButton event)
        {
            if (in_curve_area ((int)event.x, (int)event.y))
            {
                curve_hook = true;
                handle_curve_click (event);
                return false;
            }

            if (event.x <= 11 &&
                event.y > 1 &&
                event.y <= allocation.height-margin_bottom &&
                event.button == 1
                )
            {
                preamp = event.y / (double)(allocation.height - margin_bottom);
                preamp_hook = true;
            }

            if (event.button == 3)
            {
                //stdout.printf ("");
                menu.popup (null, null, null, event.button, Gtk.get_current_event_time());
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
        motion_notify_event (Gdk.EventMotion event)
        {
            double x = (double)(event.x - margin_left) / (double)(allocation.width - margin_left);
            double y = event.y / (double)(allocation.height - margin_bottom);
            if (y < 0) y = 0;
            if (y > 1) y = 1;

            if (preamp_hook)
            {
                preamp = y;
                queue_draw();
                return false;
            }
            
            if (!in_curve_area ((int)event.x, (int)event.y))
                mouse_y = -1;
            else
                mouse_y = (int)event.y;

            if (curve_hook)
            {
                if (snap)
                    snap_move (x, y);
                else
                {
                    current_point.data.x = x;

                    if ((current_point.prev != null) &&
                        current_point.prev.data.x > current_point.data.x)
                        current_point.data.x = current_point.prev.data.x;


                    if ((current_point.next != null) &&
                        current_point.next.data.x < current_point.data.x)
                        current_point.data.x = current_point.next.data.x;

                    current_point.data.y = y;

                    if (current_point.data.x > 1) current_point.data.x = 1;
                    if (current_point.data.x < 0) current_point.data.x = 0;
                }

                recalc_values();
                mouse_y = (int)event.y;
                queue_draw ();
            }
            else
            {
                if (!get_point_at (x, y))
                    get_window().set_cursor (pointer_cursor);
                else
                    get_window().set_cursor (moving_cursor);
                queue_draw ();
            }
            return false;
        }

//        public static Equalizer inst = null;

    }
}
