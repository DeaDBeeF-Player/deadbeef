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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "statusnotifier.h"

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(fmt,...)

//Globals

static GDBusNodeInfo *introspection_data = NULL;
static int next_indicator_id = 1;

static const gchar introspection_xml[] =
        "<node>"
        "  <interface name='org.kde.StatusNotifierItem'>"
        "    <property type='s' name='Category' access='read'/>"
        "    <property type='s' name='Id' access='read'/>"
        "    <property type='s' name='Title' access='read'/>"
        "    <property type='s' name='Status' access='read'/>"
        "    <property type='u' name='WindowId' access='read'/>"
        "    <property type='o' name='Menu' access='read'/>"
        "    <property type='b' name='ItemIsMenu' access='read'/>"
        "    <property type='s' name='IconName' access='read'/>"
        "    <property type='a(iiay)' name='IconPixmap' access='read'/>"
        "    <property type='s' name='OverlayIconName' access='read'/>"
        "    <property type='a(iiay)' name='OverlayIconPixmap' access='read'/>"
        "    <property type='s' name='AttentionIconName' access='read'/>"
        "    <property type='a(iiay)' name='AttentionIconPixmap' access='read'/>"
        "    <property type='s' name='AttentionMovieName' access='read'/>"
        "    <property type='(sa(iiay)ss)' name='ToolTip' access='read'/>"
        "    <method name='ContextMenu'>"
        "      <annotation name='org.kde.DBus.Method.NoReply' value='true'/>"
        "      <arg type='i' name='x' direction='in'/>"
        "      <arg type='i' name='y' direction='in'/>"
        "    </method>"
        "    <method name='Activate'>"
        "      <annotation name='org.kde.DBus.Method.NoReply' value='true'/>"
        "      <arg type='i' name='x' direction='in'/>"
        "      <arg type='i' name='y' direction='in'/>"
        "    </method>"
        "    <method name='SecondaryActivate'>"
        "      <annotation name='org.kde.DBus.Method.NoReply' value='true'/>"
        "      <arg type='i' name='x' direction='in'/>"
        "      <arg type='i' name='y' direction='in'/>"
        "    </method>"
        "    <method name='Scroll'>"
        "      <annotation name='org.kde.DBus.Method.NoReply' value='true'/>"
        "      <arg type='i' name='delta' direction='in'/>"
        "      <arg type='s' name='orientation' direction='in'/>"
        "    </method>"
        "    <signal name='NewTitle'/>"
        "    <signal name='NewIcon'/>"
        "    <signal name='NewAttentionIcon'/>"
        "    <signal name='NewOverlayIcon'/>"
        "    <signal name='NewToolTip'/>"
        "    <signal name='NewStatus'/>"
        "  </interface>"
        "</node>";

//data structures

typedef struct _IconData {
    int w, h;
    guchar *pixdata;
    guint len;
} IconData;

typedef struct _ToolTipData {
    gchar *iconname;
    IconData *icondata;
    gchar *title;
    gchar *text;
} ToolTipData;

typedef struct _StatusNotifierItem {
    GDBusConnection *connection;
    gchar *service_name;
    guint owner_id;
    guint registration_id;
    guint watcher_id;

    gchar *category;
    gchar *id;
    gchar *title;
    gchar *status;
    unsigned int windowid;
    gchar *iconname;
    IconData *iconpixmap;
    gchar *overlayiconname;
    IconData *overlayiconpixmap;
    gchar *attentioniconname;
    IconData *attentioniconpixmap;
    gchar *attentionmoviename;
    ToolTipData tooltip;

    //events
    void (*on_context_menu)(StatusNotifierItem *this, int x, int y);
    void (*on_activate)(StatusNotifierItem *this, int x, int y);
    void (*on_secondary_activate)(StatusNotifierItem *this, int x, int y);
    void (*on_scroll)(StatusNotifierItem *this, int delta,
            SN_SCROLLDIR orientation);
    void (*on_registration_error)(StatusNotifierItem *this, void *data);

    void *regerr_data;
    void (*destroy_regerr_data)(void *data);
} StatusNotifierItem;

static char* SN_CATEGORY_STR[] = { "ApplicationStatus", "Communications",
        "SystemServices", "Hardware", NULL };

static char* SN_STATUS_STR[] = { "Passive", "Active", "NeedsAttention", NULL };

#define nn(X) ((X)==NULL) ? "" : (X)

//private methods

static void sn_emit_signal(StatusNotifierItem *this, char *name) {
    if (this->connection)
        g_dbus_connection_emit_signal(this->connection,
        NULL, "/StatusNotifierItem", "org.kde.StatusNotifierItem", name,
        NULL,
        NULL);
}

static void sn_setstr(gchar **var, const gchar *value) {
    if (*var)
        g_free(*var);
    *var = g_strdup(value);
}

static void sn_rgba_to_argb(IconData *id) {
    guchar *data = (id->pixdata);
    guchar *end = (id->pixdata + id->len);
    while (data < end) {
        guchar alpha = data[3];
        data[3] = data[2]; //B
        data[2] = data[1]; //G
        data[1] = data[0]; //R
        data[0] = alpha;   //A
        data += 4;
    }
}

static void sn_get_pixel_rowstride(IconData *id, GdkPixbuf* pixbuf,
        int rowstride) {
    int newsize = id->w * id->h * 4;
    guchar *buf = g_malloc(newsize);
    guchar *src = gdk_pixbuf_get_pixels(pixbuf);
    guchar *end = src + id->len;
    while (src < end) {
        memmove(buf, src, id->w);
        buf += id->w;
        src += rowstride;
    }
    id->len = newsize;
    id->pixdata = buf;
}

static void sn_seticondata(IconData **data, GdkPixbuf* pixbuf) {
    if (*data) {
        if ((*data)->pixdata)
            g_free((*data)->pixdata);
        g_free(*data);
        *data = NULL;
    }
    if (!pixbuf)
        return;

    g_assert(gdk_pixbuf_get_colorspace(pixbuf) == GDK_COLORSPACE_RGB);
    g_assert(gdk_pixbuf_get_bits_per_sample(pixbuf) == 8);
    g_assert(gdk_pixbuf_get_has_alpha(pixbuf));
    g_assert(gdk_pixbuf_get_n_channels(pixbuf) == 4);

    int rowstride = gdk_pixbuf_get_rowstride(pixbuf);

    IconData *id = g_malloc(sizeof(IconData));
    id->w = gdk_pixbuf_get_width(pixbuf);
    id->h = gdk_pixbuf_get_height(pixbuf);
    id->len = rowstride*id->h;
    if (rowstride > (id->w * 4))
        sn_get_pixel_rowstride(id, pixbuf, rowstride);
    else
        id->pixdata = g_memdup(gdk_pixbuf_get_pixels(pixbuf), id->len);
    sn_rgba_to_argb(id);

    *data = id;
}

//public methods

void sn_set_title(StatusNotifierItem *this, const gchar *title) {
    sn_setstr(&this->title, title);
    sn_emit_signal(this, "NewTitle");
}

void sn_set_status(StatusNotifierItem *this, SN_STATUS status) {
    sn_setstr(&this->status, SN_STATUS_STR[status]);
    sn_emit_signal(this, "NewStatus");
}

void sn_set_icon_name(StatusNotifierItem *this, const gchar *iconname) {
    sn_setstr(&this->iconname, iconname);
    sn_emit_signal(this, "NewIcon");
}

void sn_set_icon_pixmap(StatusNotifierItem *this, GdkPixbuf* pixbuf) {
    sn_seticondata(&this->iconpixmap, pixbuf);
    sn_emit_signal(this, "NewIcon");
}

void sn_set_attention_icon_name(StatusNotifierItem *this, const gchar *iconname) {
    sn_setstr(&this->attentioniconname, iconname);
    sn_emit_signal(this, "NewAttentionIcon");
}

void sn_set_attention_icon_pixmap(StatusNotifierItem *this, GdkPixbuf* pixbuf) {
    sn_seticondata(&this->attentioniconpixmap, pixbuf);
    sn_emit_signal(this, "NewAttentionIcon");
}

void sn_set_overlay_icon_name(StatusNotifierItem *this, const gchar *iconname) {
    sn_setstr(&this->overlayiconname, iconname);
    sn_emit_signal(this, "NewOverlayIcon");
}

void sn_set_overlay_icon_pixmap(StatusNotifierItem *this, GdkPixbuf* pixbuf) {
    sn_seticondata(&this->overlayiconpixmap, pixbuf);
    sn_emit_signal(this, "NewOverlayIcon");
}

void sn_set_attention_movie_name(StatusNotifierItem *this,
        const gchar *iconname) {
    sn_setstr(&this->attentionmoviename, iconname);
    sn_emit_signal(this, "NewAttentionIcon");
}

void sn_set_tooltip(StatusNotifierItem *this, const gchar *iconname,
        GdkPixbuf *pixbuf, const gchar *title, const gchar *text) {
    sn_setstr(&this->tooltip.iconname, iconname);
    sn_seticondata(&this->tooltip.icondata, pixbuf);
    sn_setstr(&this->tooltip.title, title);
    sn_setstr(&this->tooltip.text, text);
    sn_emit_signal(this, "NewToolTip");
}

void sn_set_tooltip_iconname(StatusNotifierItem *this, const gchar *iconname) {
    sn_setstr(&this->tooltip.iconname, iconname);
    sn_emit_signal(this, "NewToolTip");
}

void sn_set_tooltip_icondata(StatusNotifierItem *this, GdkPixbuf *pixbuf) {
    sn_seticondata(&this->tooltip.icondata, pixbuf);
    sn_emit_signal(this, "NewToolTip");
}

void sn_set_tooltip_title(StatusNotifierItem *this, const gchar *title) {
    sn_setstr(&this->tooltip.title, title);
    sn_emit_signal(this, "NewToolTip");
}

void sn_set_tooltip_text(StatusNotifierItem *this, const gchar *text) {
    sn_setstr(&this->tooltip.text, text);
    sn_emit_signal(this, "NewToolTip");
}

const gchar* sn_get_id(StatusNotifierItem *this) {
    return this->id;
}

const gchar* sn_get_title(StatusNotifierItem *this) {
    return this->title;
}

const gchar* sn_get_icon_name(StatusNotifierItem *this) {
    return this->iconname;
}

const gboolean sn_has_icon_pixmap(StatusNotifierItem *this) {
    return this->iconpixmap != NULL;
}

const gchar* sn_get_attention_icon_name(StatusNotifierItem *this) {
    return this->attentioniconname;
}

const gboolean sn_has_attention_icon_pixmap(StatusNotifierItem *this) {
    return this->attentioniconpixmap != NULL;
}

const gchar* sn_get_overlay_icon_name(StatusNotifierItem *this) {
    return this->overlayiconname;
}

const gboolean sn_has_overlay_icon_pixmap(StatusNotifierItem *this) {
    return this->overlayiconpixmap != NULL;
}

const gchar* sn_get_attention_movie_name(StatusNotifierItem *this) {
    return this->attentionmoviename;
}

const gchar* sn_get_tooltip_iconname(StatusNotifierItem *this) {
    return this->tooltip.iconname;
}

const gboolean sn_has_tooltip_icondata(StatusNotifierItem *this) {
    return this->tooltip.icondata != NULL;
}

const gchar* sn_get_tooltip_title(StatusNotifierItem *this) {
    return this->tooltip.title;
}

const gchar* sn_get_tooltip_text(StatusNotifierItem *this) {
    return this->tooltip.text;
}

//forward
static void on_bus_acquired(GDBusConnection *connection, const gchar *name,
        gpointer user_data);
static void
on_name_acquired(GDBusConnection *connection, const gchar *name,
        gpointer user_data);
static void
on_name_lost(GDBusConnection *connection, const gchar *name, gpointer user_data);

void sn_register_item(StatusNotifierItem *this) {
    if (!introspection_data)
        introspection_data = g_dbus_node_info_new_for_xml(introspection_xml,
                NULL);
    //if there is only 1 instance, refcount is 2: it is released only by explicit
    //call to sn_finalize()
    g_dbus_node_info_ref(introspection_data);

    g_assert(introspection_data != NULL);

    trace("StatusNotifier: calling g_bus_own_name()\n");

    this->owner_id = g_bus_own_name(G_BUS_TYPE_SESSION, this->service_name,
            G_BUS_NAME_OWNER_FLAGS_NONE, on_bus_acquired, on_name_acquired,
            on_name_lost, this,
            NULL);
}

void sn_unregister(StatusNotifierItem *this) {
    if (this->watcher_id) {
        g_bus_unwatch_name(this->watcher_id);
        this->watcher_id = 0;
    }
    if (this->registration_id && this->connection) {
        g_dbus_connection_unregister_object(this->connection,
                this->registration_id);
        this->registration_id = 0;
    }
    this->connection = NULL;
    if (this->owner_id) {
        g_bus_unown_name(this->owner_id);
        this->owner_id = 0;
        g_dbus_node_info_unref(introspection_data);
    }
}

StatusNotifierItem *sn_create_with_iconname(const gchar *id,
        SN_CATEGORY category, const gchar *iconname) {
    trace("StatusNotifier: sn_create_with_iconname()\n");

    StatusNotifierItem *this = malloc(sizeof(StatusNotifierItem));
    memset(this, 0, sizeof(StatusNotifierItem));

    this->service_name = g_strdup_printf("org.kde.StatusNotifierItem-%d-%d",
            getpid(), next_indicator_id++);

    this->category = g_strdup(SN_CATEGORY_STR[category]);
    this->id = g_strdup(id);
    this->iconname = g_strdup(iconname);
    this->status = g_strdup("Passive");

    return this;
}

StatusNotifierItem *sn_create_with_icondata(const gchar *id,
        SN_CATEGORY category, GdkPixbuf *icondata) {
    trace("StatusNotifier: sn_create_with_icondata()\n");

    StatusNotifierItem *this = malloc(sizeof(StatusNotifierItem));
    memset(this, 0, sizeof(StatusNotifierItem));

    this->service_name = g_strdup_printf("org.kde.StatusNotifierItem-%d-%d",
            getpid(), next_indicator_id++);

    this->category = g_strdup(SN_CATEGORY_STR[category]);
    this->id = g_strdup(id);
    sn_seticondata(&this->iconpixmap, icondata);
    this->status = g_strdup("Passive");

    return this;
}

void sn_destroy(gpointer data) {
    trace("StatusNotifier: sn_destroy()\n");

    StatusNotifierItem *this = (StatusNotifierItem *) data;

    sn_unregister(this);
    sn_setstr(&this->service_name, NULL);

    sn_setstr(&this->category, NULL);
    sn_setstr(&this->id, NULL);
    sn_setstr(&this->title, NULL);
    sn_setstr(&this->status, NULL);
    sn_setstr(&this->iconname, NULL);
    sn_seticondata(&this->iconpixmap, NULL);
    sn_setstr(&this->overlayiconname, NULL);
    sn_seticondata(&this->overlayiconpixmap, NULL);
    sn_setstr(&this->attentioniconname, NULL);
    sn_seticondata(&this->attentioniconpixmap, NULL);
    sn_setstr(&this->attentionmoviename, NULL);
    sn_setstr(&this->tooltip.iconname, NULL);
    sn_seticondata(&this->tooltip.icondata, NULL);
    sn_setstr(&this->tooltip.title, NULL);
    sn_setstr(&this->tooltip.text, NULL);
}

void sn_finalize() {
    if (introspection_data)
        g_dbus_node_info_unref(introspection_data);
    introspection_data = NULL;
}

void sn_hook_on_context_menu(StatusNotifierItem *this, cb_context_menu cb) {
    this->on_context_menu = cb;
}

void sn_hook_on_activate(StatusNotifierItem *this, cb_activate cb) {
    this->on_activate = cb;
}

void sn_hook_on_secondary_activate(StatusNotifierItem *this,
        cb_secondary_activate cb) {
    this->on_secondary_activate = cb;
}

void sn_hook_on_scroll(StatusNotifierItem *this, cb_scroll cb) {
    this->on_scroll = cb;
}

void sn_hook_on_registration_error(StatusNotifierItem *this,
        cb_registration_error cb, void *data,
        cb_destroy_regerr_data userdata_del_cb) {
    this->on_registration_error = cb;
    this->regerr_data = data;
    this->destroy_regerr_data = userdata_del_cb;
}

//callbacks

static void sn_context_menu(StatusNotifierItem *this, int x, int y) {
    trace("StatusNotifier: sn_context_menu(%d,%d)\n",x,y);
    if (this->on_context_menu)
        this->on_context_menu(this, x, y);
}

static void sn_activate(StatusNotifierItem *this, int x, int y) {
    trace("StatusNotifier: sn_activate(%d,%d)\n",x,y);
    if (this->on_activate)
        this->on_activate(this, x, y);
}

static void sn_secondary_activate(StatusNotifierItem *this, int x, int y) {
    trace("StatusNotifier: sn_secondary_activate(%d,%d)\n",x,y);
    if (this->on_secondary_activate)
        this->on_secondary_activate(this, x, y);
}

static void sn_scroll(StatusNotifierItem *this, int delta,
        const gchar *orientation) {
    trace("StatusNotifier: sn_scroll(%d,%s)\n",delta,orientation);
    if (this->on_scroll) {
        SN_SCROLLDIR dir =
                g_ascii_strncasecmp(orientation, "vertical", 9) == 0 ?
                        Vertical : Horizontal;
        this->on_scroll(this, delta, dir);
    }
}

static void sn_finalize_regerr_data(StatusNotifierItem *this) {
    if (this->destroy_regerr_data)
        this->destroy_regerr_data(this->regerr_data);
    this->regerr_data = NULL;
    this->destroy_regerr_data = NULL;
}

static void sn_raise_registration_error(StatusNotifierItem *this) {
    if (this->on_registration_error)
        this->on_registration_error(this, this->regerr_data);
    sn_finalize_regerr_data(this);
}

static void handle_method_call(GDBusConnection *connection, const gchar *sender,
        const gchar *object_path, const gchar *interface_name,
        const gchar *method_name, GVariant *parameters,
        GDBusMethodInvocation *invocation, gpointer user_data) {
    trace("StatusNotifier: handle_method_call()\n");
    StatusNotifierItem *instance = (StatusNotifierItem *) user_data;
    if (g_strcmp0(method_name, "ContextMenu") == 0) {
        gint x, y;
        g_variant_get(parameters, "(ii)", &x, &y);
        sn_context_menu(instance, x, y);
    } else if (g_strcmp0(method_name, "Activate") == 0) {
        gint x, y;
        g_variant_get(parameters, "(ii)", &x, &y);
        sn_activate(instance, x, y);
    } else if (g_strcmp0(method_name, "SecondaryActivate") == 0) {
        gint x, y;
        g_variant_get(parameters, "(ii)", &x, &y);
        sn_secondary_activate(instance, x, y);
    } else if (g_strcmp0(method_name, "Scroll") == 0) {
        gint delta;
        const gchar *orientation;
        g_variant_get(parameters, "(i&s)", &delta, &orientation);
        sn_scroll(instance, delta, orientation);
    } else {
        fprintf(stderr, "StatusNotifier: Unknown dbus method '%s', ignoring\n",
                method_name);
    }
}

static GVariant* icondata_to_variant(IconData* id) {
    GVariant *ret = NULL;

    GVariantBuilder *bld;
    bld = g_variant_builder_new(G_VARIANT_TYPE("a(iiay)"));
    if (id) {
        GVariant *pixdata = g_variant_new_from_data(G_VARIANT_TYPE("ay"),
                id->pixdata, id->len, TRUE, NULL, NULL);
        g_variant_builder_add(bld, "(ii@ay)", id->w, id->h, pixdata);
    }
    ret = g_variant_new("a(iiay)", bld);
    g_variant_builder_unref(bld);

    return ret;
}

static GVariant *tooltip_to_variant(ToolTipData *tt) {
    return g_variant_new("(s@a(iiay)ss)", nn(tt->iconname),
            icondata_to_variant(tt->icondata), nn(tt->title), nn(tt->text));
}

static GVariant *
handle_get_property(GDBusConnection *connection, const gchar *sender,
        const gchar *object_path, const gchar *interface_name,
        const gchar *property_name, GError **error, gpointer user_data) {
    trace("StatusNotifier: handle_get_property(\"%s\")\n", property_name);
    GVariant *ret;

    StatusNotifierItem *instance = (StatusNotifierItem *) user_data;

    ret = NULL;
    if (g_strcmp0(property_name, "Category") == 0)
        ret = g_variant_new_string(nn(instance->category));
    else if (g_strcmp0(property_name, "Id") == 0)
        ret = g_variant_new_string(nn(instance->id));
    else if (g_strcmp0(property_name, "Title") == 0)
        ret = g_variant_new_string(nn(instance->title));
    else if (g_strcmp0(property_name, "Status") == 0)
        ret = g_variant_new_string(nn(instance->status));
    else if (g_strcmp0(property_name, "WindowId") == 0)
        ret = g_variant_new_uint32(instance->windowid);
    else if (g_strcmp0(property_name, "Menu") == 0)
        ret = g_variant_new_object_path ("/MenuBar"); // FIXME: this needs to be an actual dbusmenu object path
    else if (g_strcmp0(property_name, "ItemIsMenu") == 0)
        ret = g_variant_new_boolean(FALSE);
    else if (g_strcmp0(property_name, "IconName") == 0)
        ret = g_variant_new_string(nn(instance->iconname));
    else if (g_strcmp0(property_name, "IconPixmap") == 0)
        ret = icondata_to_variant(instance->iconpixmap);
    else if (g_strcmp0(property_name, "OverlayIconName") == 0)
        ret = g_variant_new_string(nn(instance->overlayiconname));
    else if (g_strcmp0(property_name, "OverlayIconPixmap") == 0)
        ret = icondata_to_variant(instance->overlayiconpixmap);
    else if (g_strcmp0(property_name, "AttentionIconName") == 0)
        ret = g_variant_new_string(nn(instance->attentioniconname));
    else if (g_strcmp0(property_name, "AttentionIconPixmap") == 0)
        ret = icondata_to_variant(instance->attentioniconpixmap);
    else if (g_strcmp0(property_name, "AttentionMovieName") == 0)
        ret = g_variant_new_string(nn(instance->attentionmoviename));
    else if (g_strcmp0(property_name, "ToolTip") == 0)
        ret = tooltip_to_variant(&(instance->tooltip));
    else {
        fprintf(stderr,
                "StatusNotifier: Unknown dbus property '%s', returning NULL\n",
                property_name);
    }

    return ret;
}

static gboolean handle_set_property(GDBusConnection *connection,
        const gchar *sender, const gchar *object_path,
        const gchar *interface_name, const gchar *property_name,
        GVariant *value, GError **error, gpointer user_data) {
    g_set_error(error, G_IO_ERROR, G_IO_ERROR_READ_ONLY,
            "Properties are read only");

    return FALSE;
}

static void on_name_appeared(GDBusConnection *connection, const gchar *name,
        const gchar *name_owner, gpointer user_data) {
    trace("StatusNotifier: on_name_appeared(), '%s' is owned by '%s'\n",name,name_owner);
    StatusNotifierItem *item = (StatusNotifierItem *) user_data;
    GError *error = NULL;
    g_dbus_connection_call_sync(connection, name_owner,
            "/StatusNotifierWatcher", "org.kde.StatusNotifierWatcher",
            "RegisterStatusNotifierItem",
            g_variant_new("(s)", item->service_name),
            NULL, G_DBUS_CALL_FLAGS_NONE, -1,
            NULL, &error);
    if (error != NULL)
        sn_raise_registration_error(item);
    else {
        item->connection = connection;
        sn_finalize_regerr_data(item);
    }
}

static void on_name_vanished(GDBusConnection *connection, const gchar *name,
        gpointer user_data) {
    trace("StatusNotifier: No owner for '%s', notifier watcher not running?\n",name);
    StatusNotifierItem *item = (StatusNotifierItem *) user_data;
    if (!item->connection)
        sn_raise_registration_error(item);
    //else the watcher disappeared but it was there before: let's wait for it
    //to come back later.
}

static const GDBusInterfaceVTable interface_vtable = { handle_method_call,
        handle_get_property, handle_set_property };

static void on_bus_acquired(GDBusConnection *connection, const gchar *name,
        gpointer user_data) {
    trace("StatusNotifier: on_bus_acquired(), registering object\n");

    StatusNotifierItem *item = (StatusNotifierItem *) user_data;

    item->registration_id = g_dbus_connection_register_object(connection,
            "/StatusNotifierItem", introspection_data->interfaces[0],
            &interface_vtable, item,
            NULL,
            NULL);
    if (!item->registration_id) {
        sn_raise_registration_error(item);
        return;
    } trace("StatusNotifier: object registered to dbus\n");

    trace("StatusNotifier: registering to StatusNotifierWatcher\n");
    item->watcher_id = g_bus_watch_name(G_BUS_TYPE_SESSION,
            "org.kde.StatusNotifierWatcher", G_BUS_NAME_WATCHER_FLAGS_NONE,
            on_name_appeared, on_name_vanished, item,
            NULL);
}

static void on_name_acquired(GDBusConnection *connection, const gchar *name,
        gpointer user_data) {
    trace("StatusNotifier: on_name_acquired(), ignoring\n");
}

static void on_name_lost(GDBusConnection *connection, const gchar *name,
        gpointer user_data) {
    trace("StatusNotifier: Connection lost!\n");
    StatusNotifierItem *item = (StatusNotifierItem *) user_data;
    if (!item->connection)
        sn_raise_registration_error(item);
}
