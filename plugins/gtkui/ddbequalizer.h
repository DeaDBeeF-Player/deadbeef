/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2015 Oleksiy Yakovenko and other contributors

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

#ifndef __DDBEQUALIZER_H__
#define __DDBEQUALIZER_H__

#include <glib.h>
#include <gtk/gtk.h>
#include <float.h>
#include <math.h>

G_BEGIN_DECLS


#define DDB_TYPE_EQUALIZER (ddb_equalizer_get_type ())
#define DDB_EQUALIZER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), DDB_TYPE_EQUALIZER, DdbEqualizer))
#define DDB_EQUALIZER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), DDB_TYPE_EQUALIZER, DdbEqualizerClass))
#define DDB_IS_EQUALIZER(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DDB_TYPE_EQUALIZER))
#define DDB_IS_EQUALIZER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), DDB_TYPE_EQUALIZER))
#define DDB_EQUALIZER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), DDB_TYPE_EQUALIZER, DdbEqualizerClass))

typedef struct _DdbEqualizer DdbEqualizer;
typedef struct _DdbEqualizerClass DdbEqualizerClass;
typedef struct _DdbEqualizerPrivate DdbEqualizerPrivate;

struct _DdbEqualizer {
	GtkDrawingArea parent_instance;
	DdbEqualizerPrivate * priv;
};

struct _DdbEqualizerClass {
	GtkDrawingAreaClass parent_class;
};


GType ddb_equalizer_get_type (void) G_GNUC_CONST;
void ddb_equalizer_set_band (DdbEqualizer* self, gint band, gdouble v);
gdouble ddb_equalizer_get_band (DdbEqualizer* self, gint band);
void ddb_equalizer_set_preamp (DdbEqualizer* self, gdouble v);
gdouble ddb_equalizer_get_preamp (DdbEqualizer* self);
DdbEqualizer* ddb_equalizer_new (void);
DdbEqualizer* ddb_equalizer_construct (GType object_type);


G_END_DECLS

#endif
