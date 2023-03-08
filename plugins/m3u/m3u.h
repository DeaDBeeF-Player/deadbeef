//
//  m3u.h
//  deadbeef
//
//  Created by Oleksiy Yakovenko on 07/02/2021.
//  Copyright © 2021 Oleksiy Yakovenko. All rights reserved.
//

#ifndef m3u_h
#define m3u_h

#include <deadbeef/deadbeef.h>

#ifdef __cplusplus
extern "C" {
#endif

DB_plugin_t *
m3u_load (DB_functions_t *api);

DB_playItem_t *
load_m3u_from_buffer(DB_playItem_t *after, const char *buffer, int64_t sz, int (*cb)(DB_playItem_t *, void *), const char *fname, int *pabort, ddb_playlist_t *plt, void *user_data);

#ifdef __cplusplus
}
#endif

#endif /* m3u_h */
