static const int spot_size = 3;
static const int margin_left = 20;
static const int margin_bottom = 10;
static const int bands = 18;
static int btn_size = 7;

const string[] freqs = {
    "32","80","110","160","220","315","450","630","900",
    "1.3k","1.8k","2.5k","3.6k","5k","7k","10k","14k","20k"
};

namespace Deadbeef {
    public class Graphic : Gtk.DrawingArea
    {
        public signal void on_changed (double[] values);

        class Point
        {
            public double x;
            public double y;
        }

        private List <Point> points = new List <Point> ();
        private unowned List <Point> current_point = null;

        private Gdk.Color back_color = Gdk.Color() {red = 0, green = 0, blue = 0};
        private Gdk.Color fore_bright_color = Gdk.Color() {red = 0xffff, green = 0x7e00, blue = 0};
        private Gdk.Color fore_dark_color = Gdk.Color() {red = 0x7800, green = 0x3b00, blue = 0};

        private Pango.Context pango_ctx;
        private Pango.FontDescription font_desc;

        private double[] values = new double [bands];
        private int mouse_y;
        
        private bool snap = false;
        private bool aa_mode = true;

        Gdk.Cursor moving_cursor = new Gdk.Cursor (Gdk.CursorType.FLEUR);
        Gdk.Cursor pointer_cursor = new Gdk.Cursor (Gdk.CursorType.LEFT_PTR);

        public Graphic ()
        {
            add_events (Gdk.EventMask.BUTTON_PRESS_MASK
                | Gdk.EventMask.BUTTON_RELEASE_MASK
                | Gdk.EventMask.LEAVE_NOTIFY_MASK
                | Gdk.EventMask.POINTER_MOTION_MASK);

            modify_bg (Gtk.StateType.NORMAL, back_color);

            font_desc = Pango.FontDescription.from_string ("fixed 4");
            pango_ctx = new Pango.Context();
            pango_ctx.set_font_description (font_desc);
            recalc_values();
        }
        
        private void
        toggle_snap ()
        {
            if (snap)
                snap = false;
            else
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
                snap = true;
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

        private double
        cubic (double y0, double y1, double y2, double y3, double mu)
        {
           double a0,a1,a2,a3,mu2;

           mu2 = mu*mu;
           a0 = y3 - y2 - y0 + y1;
           a1 = y0 - y1 - a0;
           a2 = y2 - y0;
           a3 = y1;

           return (a0*mu*mu2+a1*mu2+a2*mu+a3);
        }

        public override bool
        expose_event (Gdk.EventExpose event)
        {
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
                Gdk.draw_line (d, gc, (int)((i+1)*step)+margin_left, 0, (int)((i+1)*step)+margin_left, height - margin_bottom);
            }

            double vstep = (double)(height-margin_bottom) / 4;
            for (i=1; i < 4; i++)
                Gdk.draw_line (d, gc, margin_left, (int)(i*vstep), width, (int)(i*vstep));
            
            gc.set_rgb_fg_color (fore_bright_color);

            //drawing freqs:
            Pango.Layout l = create_pango_layout (null);
            var ctx = l.get_context ();
            var fd = ctx.get_font_description ();
            fd.set_size (4);
            fd.set_family_static ("fixed");
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
                string tmp = "%.1f".printf (db);
                l.set_text (tmp, (int)tmp.len());
                Gdk.draw_layout (d, gc, margin_left-1, mouse_y-3, l);
            }

            l.set_text ("-20db", 5);
            Gdk.draw_layout (d, gc, margin_left-1, height-margin_bottom-4, l);

            l.set_text ("20db", 4);
            Gdk.draw_layout (d, gc, margin_left-1, 1, l);

            l.set_text ("0db", 4);
            Gdk.draw_layout (d, gc, margin_left-1, (height-margin_bottom)/2-3, l);

            //drawing dropdown button:
            d.draw_rectangle (gc, snap, 1, height-(btn_size+2), btn_size, btn_size);

            d.draw_rectangle (gc, false, margin_left, 0, width-margin_left-1, height-margin_bottom-1);
            gc.set_line_attributes (2, Gdk.LineStyle.SOLID, Gdk.CapStyle.NOT_LAST, Gdk.JoinStyle.MITER);

            gc.set_clip_rectangle ({margin_left+1, 1, width-margin_left-2, height-margin_bottom-2});

            //drawing bars:
            gc.set_rgb_fg_color (fore_dark_color);

            int bar_w = 11;
            if (step < bar_w)
                bar_w = (int)step-1;


            for (i = 0; i < bands; i++)
            {
                int count = (int)((height-margin_bottom) * (1-values[i]) / 6)+1;
                for (int j = 0; j < count; j++)
                    d.draw_rectangle (
                        gc,
                        true,
                        (int)((i+1)*step)+margin_left - bar_w/2,
                        height-margin_bottom-j*6 - 6,
                        bar_w,
                        4);
            }

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

            Cairo.Context cairo = null;

            if (aa_mode)
            {
                cairo = Gdk.cairo_create (d);
            }

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
                    int pts = (int)((double)((xs[i+1] - xs[i]) * allocation.width) / 5.0);
                    //step = 5.0;
                    step = (double)(xs[i+1] - xs[i])/(double)pts;
                    //stdout.printf ("%d\n", pts);

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
                        gp = abs_to_screen (ii*step+xs[i], y);

                        if (gp.y < 2) gp.y = 2;
                        if (gp.y > height-margin_bottom-2) gp.y = height-margin_bottom-2;

                        //Gdk.draw_point (d, gc, gp.x, gp.y);
                        if (aa_mode)
                            cairo.line_to (gp.x, gp.y);
                        else
                            Gdk.draw_line (d, gc, prev_x, prev_y, gp.x, gp.y);
                        prev_x = gp.x;
                        prev_y = gp.y;
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

            //drawing mouse coordinates:
            gc.set_line_attributes (1, Gdk.LineStyle.ON_OFF_DASH, Gdk.CapStyle.NOT_LAST, Gdk.JoinStyle.MITER);
            Gdk.draw_line (d, gc, 0, mouse_y, width, mouse_y);
            
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
            return val * k + d;
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
            if (idx < bands)
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

                        //points.append (point);
                        //current_point = point;
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

        /* Mouse button got pressed over widget */
        public override bool
        button_press_event (Gdk.EventButton event)
        {
            if (event.x > margin_left &&
                event.y < allocation.height-margin_bottom)
            {
                handle_curve_click (event);
            }
            return false;
        }

        /* Mouse button got released */
        public override bool button_release_event (Gdk.EventButton event)
        {
            if (event.x < btn_size &&
                event.y > allocation.height-btn_size
                )
            {
                toggle_snap ();
            }
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
            
            //if (points == null)
                //return false;

            if (event.x <= margin_left ||
                event.y >= allocation.height-margin_bottom)
                return false;

            if (0 != (event.state & Gdk.ModifierType.BUTTON1_MASK))
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

                    if (current_point.data.y > 1) current_point.data.y = 1;
                    if (current_point.data.y < 0) current_point.data.y = 0;
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
                mouse_y = (int)event.y;
                queue_draw ();
            }
            return false;
        }

        public static Graphic inst = null;

/*        public static int
        main (string[] args)
        {
            Gtk.init (ref args);
            var wnd = new Gtk.Window (Gtk.WindowType.TOPLEVEL);
            wnd.destroy.connect (Gtk.main_quit);

            var gr = new Graphic ();
            Graphic.inst = gr;
            wnd.add (gr);
            gr.show();
            wnd.show();

            Gtk.main();
            return 0;
        }*/
    }
}
