/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2010 Alexey Yakovenko <waker@users.sourceforge.net>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

namespace Ddb {

    public class CellEditableTextView : Gtk.CellEditable, Gtk.TextView {
        public bool editing_canceled = false;
        public string tree_path;

//        private bool on_key_press_event (Gdk.EventKey event) {
        private override bool key_press_event (Gdk.EventKey event) {
            stdout.printf ("key_press_event\n");
            bool res = true;
            if (event.keyval == Gdk.KeySyms.Return) {
                if ((event.state & Gdk.ModifierType.CONTROL_MASK) != 0) {
                    res = base.key_press_event (event);
                }
                else {
                    editing_done ();
                    remove_widget ();
                    return true;
                }
            }
            else if (event.keyval == Gdk.KeySyms.Escape) {
                editing_canceled = true;
                editing_done ();
                remove_widget ();
                return true;
            }
            else {
                res = base.key_press_event (event);
            }
            /**** seems like auto-adjusting height is impossible when inside of cellrenderer
            // test buffer height
            Gtk.TextBuffer buf = get_buffer ();
            Gtk.TextIter begin, end;
            buf.get_iter_at_offset (out begin, 0);
            buf.get_iter_at_offset (out end, -1);
            int y_begin, y_end;
            int h_begin, h_end;
            get_line_yrange (begin, out y_begin, out h_begin);
            get_line_yrange (end, out y_end, out h_end);
            stdout.printf ("new y: %d %d, %d %d\n", y_begin, h_begin, y_end, h_end);
            if (allocation.height < y_end) {
                stdout.printf ("set_size_request %d\n", y_end + h_end);
                set_size_request (-1, y_end + h_end);
            }
            */

            return res;
        }

        public void start_editing (Gdk.Event event) {
            // override keypress
//            key_press_event.connect (on_key_press_event);
        }
    }

    public class CellRendererTextMultiline : Gtk.CellRendererText
    {
        private CellEditableTextView entry;
        private ulong focus_out_id;

        private static void gtk_cell_renderer_text_editing_done (CellEditableTextView entry, CellRendererTextMultiline self) {
            entry.disconnect (self.focus_out_id);
            self.stop_editing (entry.editing_canceled);
            Gtk.TextBuffer buf = entry.get_buffer ();
            Gtk.TextIter begin, end;
            buf.get_iter_at_offset (out begin, 0);
            buf.get_iter_at_offset (out end, -1);
            string new_text = buf.get_text (begin, end, true);
            self.edited (entry.tree_path, new_text);
        }

        private static bool gtk_cell_renderer_focus_out_event (CellEditableTextView entry, Gdk.Event event, CellRendererTextMultiline self) {
            stdout.printf ("focus_out_event\n");
            entry.editing_canceled = true;
            entry.remove_widget ();
            return false;
        }


        public override unowned Gtk.CellEditable start_editing (Gdk.Event event, Gtk.Widget widget, string path, Gdk.Rectangle background_area, Gdk.Rectangle cell_area, Gtk.CellRendererState flags) {
            if (!editable) {
                return (Gtk.CellEditable)null;
            }
            entry = new CellEditableTextView ();
            entry.tree_path = path;
            Gtk.TextBuffer buf = new Gtk.TextBuffer (null);
            if (text != null) {
                buf.set_text (text, -1);
            }
            entry.set_buffer (buf);
            Signal.connect (entry, "editing-done", (GLib.Callback)gtk_cell_renderer_text_editing_done, this);
            focus_out_id = Signal.connect_after (entry, "focus-out-event", (GLib.Callback)gtk_cell_renderer_focus_out_event, this);
            entry.set_size_request (cell_area.width, cell_area.height);
            entry.show ();

            return (Gtk.CellEditable)entry;
        }
    }
}
