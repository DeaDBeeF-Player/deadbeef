using Deadbeef;

Graphic gr = null;

public static bool
redraw ()
{
    gr.queue_draw ();
    return true;
}

public static int
main (string[] args)
{
    Gtk.init (ref args);
    var wnd = new Gtk.Window (Gtk.WindowType.TOPLEVEL);
    wnd.destroy.connect (Gtk.main_quit);

    gr = new Graphic ();
    Graphic.inst = gr;
    wnd.add (gr);
    gr.show();
    wnd.show();

    Timeout.add (50, redraw);

    Gtk.main();
    return 0;
}

