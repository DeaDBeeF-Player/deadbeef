//
//  custombindings.c
//  deadbeef
//
//  Created by waker on 30/07/15.
//  Copyright (c) 2015 Alexey Yakovenko. All rights reserved.
//

#include "custombindings.h"
#include "types.h"

extern DB_functions_t *deadbeef;

int
js_impl_plt_get_title (duk_context *ctx) {
    ddb_playlist_t* arg0 = js_init_ddb_playlist_t_ptr_argument (ctx, 0);

    char buffer[200];
    int ret = deadbeef->plt_get_title (arg0, buffer, sizeof (buffer));

    js_return_string_value (ctx, buffer);
    return 1;
}

