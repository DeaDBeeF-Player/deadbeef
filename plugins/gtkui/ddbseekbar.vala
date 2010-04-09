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
    public class Seekbar : Gtk.Widget
    {
        construct
        {
        }

        public override void realize () {
            set_has_window (false);
            base.realize ();
        }

        public override void unrealize () {
            base.unrealize ();
        }

        public override void size_request (out Gtk.Requisition requisition) {
            // leave at default for now
        }

        public override bool expose_event (Gdk.EventExpose event) {
            seekbar_draw (base);
            return true;
        }

        public override bool button_press_event (Gdk.EventButton event) {
            return on_seekbar_button_press_event (this, event);
        }

        public override bool button_release_event (Gdk.EventButton event) {
            return on_seekbar_button_release_event (this, event);
        }

        public override bool motion_notify_event (Gdk.EventMotion event) {
            return on_seekbar_motion_notify_event (this, event);
        }
    }

}

