//
//  ctmap.h
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 2/23/20.
//  Copyright © 2020 Alexey Yakovenko. All rights reserved.
//

#ifndef ctmapping_h
#define ctmapping_h

#define DDB_CTMAP_MAX_PLUGINS 5

typedef struct ddb_ctmap_s {
    char *ct;
    char *plugins[DDB_CTMAP_MAX_PLUGINS];
    struct ddb_ctmap_s *next;
} ddb_ctmap_t;

ddb_ctmap_t *
ddb_ctmap_init_from_string (const char *mapstr);

void
ddb_ctmap_free (ddb_ctmap_t *ctmap);

#endif /* ctmapping_h */
