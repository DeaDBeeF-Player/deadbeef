/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2023 Oleksiy Yakovenko and other contributors

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

#ifndef mlcellrendererpixbuf_h
#define mlcellrendererpixbuf_h

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct {
    void *ctx;
    GdkPixbuf *(*cell_did_became_visible) (void *ctx, const char *path);
} MlCellRendererPixbufDelegate;

#define ML_TYPE_CELL_RENDERER_PIXBUF (ml_cell_renderer_pixbuf_get_type ())

#define ML_DECLARE_TYPE(ModuleObjName, module_obj_name, MODULE, OBJ_NAME, ParentName)          \
    GType module_obj_name##_get_type (void);                                                   \
    typedef struct _##ModuleObjName ModuleObjName;                                             \
    typedef struct {                                                                           \
        ParentName##Class parent_class;                                                        \
    } ModuleObjName##Class;                                                                    \
                                                                                               \
    G_GNUC_UNUSED static inline ModuleObjName *MODULE##_##OBJ_NAME (gpointer ptr) {            \
        return G_TYPE_CHECK_INSTANCE_CAST (ptr, module_obj_name##_get_type (), ModuleObjName); \
    }                                                                                          \
    G_GNUC_UNUSED static inline gboolean MODULE##_IS_##OBJ_NAME (gpointer ptr) {               \
        return G_TYPE_CHECK_INSTANCE_TYPE (ptr, module_obj_name##_get_type ());                \
    }

ML_DECLARE_TYPE (MlCellRendererPixbuf, ml_cell_renderer_pixbuf, ML, CELL_RENDERER_PIXBUF, GtkCellRenderer)

MlCellRendererPixbuf *
ml_cell_renderer_pixbuf_new (MlCellRendererPixbufDelegate *delegate);

G_END_DECLS

#endif /* mlcellrendererpixbuf_h */
