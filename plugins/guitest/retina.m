#include <Cocoa/Cocoa.h>
#include <gtk/gtk.h>
#include <gdk/gdkquartz.h>
#include "retina.h"

int
is_retina (GtkWidget *w) {
    NSWindow *window = gdk_quartz_window_get_nswindow (gtk_widget_get_window (w));
    if ([window respondsToSelector:@selector(backingScaleFactor)]) {
        float s = [window backingScaleFactor];
        return s > 1.0f;
    }
    return 0; 
}
