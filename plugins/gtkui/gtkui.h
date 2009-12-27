#ifndef __GTKUI_H
#define __GTKUI_H

extern DB_functions_t *deadbeef;

struct _GSList;

void
gtkui_add_dirs (struct _GSList *lst);

void
gtkui_add_files (struct _GSList *lst);

void
gtkui_open_files (struct _GSList *lst);

void
gtkui_receive_fm_drop (char *mem, int length, int drop_y);

#endif
