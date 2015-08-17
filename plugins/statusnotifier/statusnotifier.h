/*
    StatusNotifierItem support library for GTK
    Copyright (C) 2015 Giulio Bernardi

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

#ifndef STATUSNOTIFIER_H
#define STATUSNOTIFIER_H

#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

typedef enum {
	ApplicationStatus,
	Communications,
	SystemServices,
	Hardware
} SN_CATEGORY;

typedef enum {
	Passive,
	Active,
	NeedsAttention
} SN_STATUS;

typedef enum {
	Vertical,
	Horizontal
} SN_SCROLLDIR;

typedef struct _StatusNotifierItem StatusNotifierItem;

typedef void (*cb_context_menu) (StatusNotifierItem *this, int x, int y);
typedef void (*cb_activate) (StatusNotifierItem *this, int x, int y);
typedef void (*cb_secondary_activate) (StatusNotifierItem *this, int x, int y);
typedef void (*cb_scroll) (StatusNotifierItem *this, int delta, SN_SCROLLDIR orientation);
typedef void (*cb_registration_error) (StatusNotifierItem *this, void *data);
typedef void (*cb_destroy_regerr_data) (void *data);


StatusNotifierItem *sn_create_with_iconname(const gchar *id, SN_CATEGORY category, const gchar *iconname);
StatusNotifierItem *sn_create_with_icondata(const gchar *id, SN_CATEGORY category, GdkPixbuf *icondata);
void sn_destroy (gpointer data);
void sn_finalize();

void sn_set_title(StatusNotifierItem *this, const gchar *title);
void sn_set_status(StatusNotifierItem *this, SN_STATUS status);
void sn_set_icon_name(StatusNotifierItem *this, const gchar *iconname);
void sn_set_icon_pixmap(StatusNotifierItem *this, GdkPixbuf* pixbuf);
void sn_set_attention_icon_name(StatusNotifierItem *this, const gchar *iconname);
void sn_set_attention_icon_pixmap(StatusNotifierItem *this, GdkPixbuf* pixbuf);
void sn_set_overlay_icon_name(StatusNotifierItem *this, const gchar *iconname);
void sn_set_overlay_icon_pixmap(StatusNotifierItem *this, GdkPixbuf* pixbuf);
void sn_set_attention_movie_name(StatusNotifierItem *this, const gchar *iconname);
void sn_set_tooltip(StatusNotifierItem *this,
		const gchar *iconname,
		GdkPixbuf *pixbuf,
		const gchar *title,
		const gchar *text);
void sn_set_tooltip_iconname(StatusNotifierItem *this, const gchar *iconname);
void sn_set_tooltip_icondata(StatusNotifierItem *this, GdkPixbuf *pixbuf);
void sn_set_tooltip_title(StatusNotifierItem *this, const gchar *title);
void sn_set_tooltip_text(StatusNotifierItem *this, const gchar *text);

const gchar* sn_get_id(StatusNotifierItem *this);
const gchar* sn_get_title(StatusNotifierItem *this);
const gchar* sn_get_icon_name(StatusNotifierItem *this);
const gboolean sn_has_icon_pixmap(StatusNotifierItem *this);
const gchar* sn_get_attention_icon_name(StatusNotifierItem *this);
const gboolean sn_has_attention_icon_pixmap(StatusNotifierItem *this);
const gchar* sn_get_overlay_icon_name(StatusNotifierItem *this);
const gboolean sn_has_overlay_icon_pixmap(StatusNotifierItem *this);
const gchar* sn_get_attention_movie_name(StatusNotifierItem *this);
const gchar* sn_get_tooltip_iconname(StatusNotifierItem *this);
const gboolean sn_has_tooltip_icondata(StatusNotifierItem *this);
const gchar* sn_get_tooltip_title(StatusNotifierItem *this);
const gchar* sn_get_tooltip_text(StatusNotifierItem *this);

void sn_register_item(StatusNotifierItem *this);
void sn_unregister(StatusNotifierItem *this);

void sn_hook_on_context_menu(StatusNotifierItem *this, cb_context_menu cb);
void sn_hook_on_activate(StatusNotifierItem *this, cb_activate cb);
void sn_hook_on_secondary_activate(StatusNotifierItem *this, cb_secondary_activate cb);
void sn_hook_on_scroll(StatusNotifierItem *this, cb_scroll cb);
void sn_hook_on_registration_error(StatusNotifierItem *this, cb_registration_error cb, void *data, cb_destroy_regerr_data userdata_del_cb);


#endif
