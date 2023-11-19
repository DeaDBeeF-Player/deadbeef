//
//  medialibmanager.h
//  DeaDBeeF
//
//  Created by Oleksiy Yakovenko on 12/08/2021.
//  Copyright © 2021 Oleksiy Yakovenko. All rights reserved.
//

#ifndef medialibmanager_h
#define medialibmanager_h

#include <deadbeef/deadbeef.h>

ddb_mediasource_source_t *
gtkui_medialib_get_source (void);

void
gtkui_medialib_free (void);

void
gtkui_medialib_preset_set (const char *preset);

char *
gtkui_medialib_preset_get (void);

#endif /* medialibmanager_h */
