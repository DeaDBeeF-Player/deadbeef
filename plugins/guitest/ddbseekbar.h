#ifndef __DDBSEEKBAR_H__
#define __DDBSEEKBAR_H__

#include <glib.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS


#define DDB_TYPE_SEEKBAR (ddb_seekbar_get_type ())
#define DDB_SEEKBAR(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), DDB_TYPE_SEEKBAR, DdbSeekbar))
#define DDB_SEEKBAR_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), DDB_TYPE_SEEKBAR, DdbSeekbarClass))
#define DDB_IS_SEEKBAR(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DDB_TYPE_SEEKBAR))
#define DDB_IS_SEEKBAR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), DDB_TYPE_SEEKBAR))
#define DDB_SEEKBAR_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), DDB_TYPE_SEEKBAR, DdbSeekbarClass))

typedef struct _DdbSeekbar DdbSeekbar;
typedef struct _DdbSeekbarClass DdbSeekbarClass;
typedef struct _DdbSeekbarPrivate DdbSeekbarPrivate;

struct _DdbSeekbar {
	GtkWidget parent_instance;
	DdbSeekbarPrivate * priv;
    int seekbar_moving;
    float seekbar_moved;
    float seektime_alpha;
    int seekbar_move_x;
    int textpos;
    int textwidth;
};

struct _DdbSeekbarClass {
	GtkWidgetClass parent_class;
};


GType
ddb_seekbar_get_type (void);

GtkWidget*
ddb_seekbar_new (void);

void
ddb_seekbar_init_signals (DdbSeekbar *sb, GtkWidget *evbox);

G_END_DECLS

#endif
