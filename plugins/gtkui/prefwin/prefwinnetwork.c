/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2021 Alexey Yakovenko and other contributors

    This software is provided 'as-is', without any express or implied
    warranty.  In no event will the authors be held liable for any damages
    arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.

    3. This notice may not be removed or altered from any source distribution.
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include "../support.h"
#include <gtk/gtk.h>
#include "ctmapping.h"
#include "gtkui.h"
#include "prefwin.h"
#include "prefwinnetwork.h"

void
prefwin_init_network_tab (GtkWidget *_prefwin) {
    GtkWidget *w = _prefwin;

    prefwin_set_toggle_button("pref_network_enableproxy", deadbeef->conf_get_int ("network.proxy", 0));
    gtk_entry_set_text (GTK_ENTRY (lookup_widget (w, "pref_network_proxyaddress")), deadbeef->conf_get_str_fast ("network.proxy.address", ""));
    gtk_entry_set_text (GTK_ENTRY (lookup_widget (w, "pref_network_proxyport")), deadbeef->conf_get_str_fast ("network.proxy.port", "8080"));
    GtkComboBox *combobox = GTK_COMBO_BOX (lookup_widget (w, "pref_network_proxytype"));
    const char *type = deadbeef->conf_get_str_fast ("network.proxy.type", "HTTP");
    if (!strcasecmp (type, "HTTP")) {
        prefwin_set_combobox (combobox, 0);
    }
    else if (!strcasecmp (type, "HTTP_1_0")) {
        prefwin_set_combobox (combobox, 1);
    }
    else if (!strcasecmp (type, "SOCKS4")) {
        prefwin_set_combobox (combobox, 2);
    }
    else if (!strcasecmp (type, "SOCKS5")) {
        prefwin_set_combobox (combobox, 3);
    }
    else if (!strcasecmp (type, "SOCKS4A")) {
        prefwin_set_combobox (combobox, 4);
    }
    else if (!strcasecmp (type, "SOCKS5_HOSTNAME")) {
        prefwin_set_combobox (combobox, 5);
    }
    gtk_entry_set_text (GTK_ENTRY (lookup_widget (w, "proxyuser")), deadbeef->conf_get_str_fast ("network.proxy.username", ""));
    gtk_entry_set_text (GTK_ENTRY (lookup_widget (w, "proxypassword")), deadbeef->conf_get_str_fast ("network.proxy.password", ""));

    char ua[100];
    deadbeef->conf_get_str ("network.http_user_agent", "deadbeef", ua, sizeof (ua));
    prefwin_set_entry_text("useragent", ua);
}

