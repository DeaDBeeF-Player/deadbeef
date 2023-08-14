/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2021 Oleksiy Yakovenko and other contributors

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

#include <assert.h>
#include <stdlib.h>
#include <sys/time.h>
#include "medialibsource.h"
#include "medialibtree.h"

//#define DUMP_GENERATED_TREE 1

static DB_functions_t *deadbeef;

static char *artist_album_bc;
static char *title_bc;

static ml_tree_item_t *
_tree_item_alloc (uint64_t row_id) {
    ml_tree_item_t *item = calloc (1, sizeof (ml_tree_item_t));
    item->row_id = row_id;
    return item;
}

static void
get_subfolders_for_folder (ml_tree_item_t *folderitem, ml_collection_tree_node_t *folder, int selected) {
    if (!folderitem->text) {
        folderitem->text = deadbeef->metacache_add_string (folder->text);
    }

    ml_tree_item_t *tail = NULL;
    if (folder->children) {
        for (ml_collection_tree_node_t *c = folder->children; c; c = c->next) {
            ml_tree_item_t *subfolder = _tree_item_alloc(c->row_id);
            get_subfolders_for_folder (subfolder, c, selected);
            if (subfolder->num_children > 0) {
                if (tail) {
                    tail->next = subfolder;
                    tail = subfolder;
                }
                else {
                    folderitem->children = subfolder;
                    tail = subfolder;
                }
                folderitem->num_children++;
            }
            else {
                ml_free_list (NULL, (ddb_medialib_item_t *)subfolder);
            }
        }
    }
    if (folder->items) {
        for (ml_collection_track_ref_t *i = folder->items; i; i = i->next) {
            if (selected && !deadbeef->pl_is_selected(i->it)) {
                continue;
            }
            ml_tree_item_t *trackitem = _tree_item_alloc(i->row_id);
            ddb_tf_context_t ctx = {
                ._size = sizeof (ddb_tf_context_t),
                .flags = DDB_TF_CONTEXT_NO_MUTEX_LOCK,
                .it = i->it,
            };
            char text[1000];
            deadbeef->tf_eval (&ctx, title_bc, text, sizeof (text));

            trackitem->text = deadbeef->metacache_add_string (text);
            trackitem->track = i->it;
            deadbeef->pl_item_ref (i->it);

            if (tail) {
                tail->next = trackitem;
                tail = trackitem;
            }
            else {
                folderitem->children = trackitem;
                tail = trackitem;
            }
            folderitem->num_children++;
        }
    }
}

static ddb_playItem_t *
_create_sorted_tree(
    ddb_playlist_t *plt,
    ml_tree_item_t *parent,
    ddb_playItem_t *first,
    char * const * const tfs,
    char * const * const text_tfs,
    size_t tfs_count,
    int level,
    int selected
) {

    ddb_tf_context_t ctx = {0};
    ctx._size = sizeof (ddb_tf_context_t);
    ctx.flags = DDB_TF_CONTEXT_NO_MUTEX_LOCK | DDB_TF_CONTEXT_NO_DYNAMIC;
    ctx.plt = plt;
    ctx.iter = PL_MAIN;

    char group_text[1000] = "";
    ddb_playItem_t *next = NULL;
    if (first != NULL) {
        next = first;
        deadbeef->pl_item_ref(next);
    }
    else if (parent->track) {
        next = parent->track;
        deadbeef->pl_item_ref(next);
    }
    else {
        next = deadbeef->plt_get_first(plt, PL_MAIN);
    }

    if (next == NULL) {
        return NULL; // empty list
    }

    ddb_playItem_t *prev = NULL;
    ml_tree_item_t *group = NULL;

    ml_tree_item_t *tail = parent->children;
    while (tail && tail->next) {
        tail = tail->next;
    }

    do {
        if (selected && !deadbeef->pl_is_selected(next)) {
            ddb_playItem_t *it = deadbeef->pl_get_next(next, PL_MAIN);
            deadbeef->pl_item_unref(next);
            next = it;
            continue; // filter
        }
        char next_text[1000];
        ctx.it = next;

        int group_level = level;
        if (level == tfs_count - 1) {
            group_level--;
        }

        int res = deadbeef->tf_eval(&ctx, tfs[group_level], next_text, sizeof (next_text));
        if (res < 0) {
            // FIXME: error
        }

        size_t len = strlen(next_text);

        // Compare group text
        int is_first_item = group_text[0] == 0;
        int group_did_change = is_first_item || memcmp(group_text, next_text, len);
        int is_leaf = level == tfs_count - 1;
        if (is_first_item || group_did_change != is_leaf) {
            // Create new item
            memcpy(group_text, next_text, len+1);

            group = _tree_item_alloc(0); // FIXME: rowid
            deadbeef->tf_eval(&ctx, text_tfs[level], next_text, sizeof (next_text));
            group->text = deadbeef->metacache_add_string(next_text);
            deadbeef->pl_item_ref (next);
            group->track = next;

#if DUMP_GENERATED_TREE
            for (int indent = 0; indent < level; indent++) {
                printf ("    ");
            }
            printf("%s\n", next_text);
#endif

            if (tail == NULL) {
                parent->children = group;
            }
            else {
                tail->next = group;
            }
            tail = group;
            parent->num_children++;
        }

        if (!is_leaf) {
            // recurse into subgroups
            ddb_playItem_t *new_next = _create_sorted_tree(plt, group, next, tfs, text_tfs, tfs_count, level+1, selected);
            deadbeef->pl_item_unref(next);
            next = new_next;
        }

        if (next == NULL) {
            break;
        }

        if (is_leaf && (!group_did_change || is_first_item)) {
            if (prev != NULL) {
                deadbeef->pl_item_unref(prev);
            }

            ddb_playItem_t *it = next;
            next = deadbeef->pl_get_next(it, PL_MAIN);
            prev = it;
        }

        if (!is_leaf && level > 0) {
            break;
        }

        if (is_leaf && group_did_change && !is_first_item) {
            break;
        }

    } while (next != NULL);

    if (prev != NULL) {
        deadbeef->pl_item_unref(prev);
    }
    return next;
}

static void
_create_tf_tree(medialib_source_t *source, ml_tree_item_t *root, const char **tfs, int tfs_count, int selected) {
    size_t tf_sort_size = 0;

    char **bcs = calloc (tfs_count, sizeof (char *));
    char **text_bcs = calloc (tfs_count, sizeof (char *));

    for (int i = 0; i < tfs_count; i++) {
        text_bcs[i] = deadbeef->tf_compile(tfs[i]);

        // bcs should contain current + all "parent" bcs
        size_t bc_len = 0;
        for (int j = 0; j <= i; j++) {
            bc_len += strlen(tfs[j]);
        }
        bc_len++;

        char *bc = calloc (1, bc_len);
        char *p = bc;
        for (int j = 0; j <= i; j++) {
            size_t len = strlen(tfs[j]);
            memcpy(p, tfs[j], len);
            p += len;
        }
        *p = 0;

        bcs[i] = deadbeef->tf_compile(bc);
        free (bc);

        tf_sort_size += strlen(tfs[i]);
    }
    tf_sort_size++;

    char *tf_sort = calloc (1, tf_sort_size);
    char *curr = tf_sort;
    for (int i = 0; i < tfs_count; i++) {
        size_t len = strlen(tfs[i]);
        memcpy(curr, tfs[i], len);
        curr += len;
    }
    *curr = 0;

    char *tf_sort_bc = deadbeef->tf_compile(tf_sort);

    deadbeef->plt_sort_v2(source->ml_playlist, PL_MAIN, -1, tf_sort, DDB_SORT_ASCENDING);

    _create_sorted_tree(source->ml_playlist, root, NULL, bcs, text_bcs, tfs_count, 0, selected);

    for (int i = 0; i < tfs_count; i++) {
        deadbeef->tf_free(bcs[i]);
    }
    free (bcs);
    free (tf_sort);
    deadbeef->tf_free(tf_sort_bc);
}

ml_tree_item_t *
_create_item_tree_from_collection(ml_collection_t *coll, const char *filter, medialibSelector_t index, medialib_source_t *source) {
    int selected = 0;
    if (filter && source->ml_playlist) {
        deadbeef->plt_search_reset (source->ml_playlist);
        deadbeef->plt_search_process2 (source->ml_playlist, filter, 1);
        selected = 1;
    }

    struct timeval tm1, tm2;
    gettimeofday (&tm1, NULL);

    ml_tree_item_t *root = _tree_item_alloc(0);
    root->text = deadbeef->metacache_add_string ("All Music");

    if (source->ml_playlist == NULL) {
        return root;
    }

    if (index == SEL_FOLDERS) {
        get_subfolders_for_folder(root, &source->db.folders.root, selected);
    }
    else if (index == SEL_ARTISTS) {
        // list of albums for artist
        const char *tfs[] = {
            "$if2(%album artist%,\\<?\\>)",
            "[%album artist% - ]%album%",
            "[%tracknumber%. ]%title%"
        };

        _create_tf_tree(source, root, tfs, 3, selected);
    }
    else if (index == SEL_GENRES) {
        // list of albums for genre
        const char *tfs[] = {
            "$if2(%genre%,\\<?\\>)",
            "[%album artist% - ]%album%",
            "[%tracknumber%. ]%title%"
        };

        _create_tf_tree(source, root, tfs, 3, selected);
    }
    else if (index == SEL_ALBUMS) {
        // list of tracks for album
        const char *tfs[] = {
            "[%album artist% - ]%album%",
            "[%tracknumber%. ]%title%"
        };

        _create_tf_tree(source, root, tfs, 2, selected);
    }

    // cleanup
    gettimeofday (&tm2, NULL);
    long ms = (tm2.tv_sec*1000+tm2.tv_usec/1000) - (tm1.tv_sec*1000+tm1.tv_usec/1000);

    fprintf (stderr, "tree build time: %f seconds\n", ms / 1000.f);
    return root;
}

void
ml_free_list (ddb_mediasource_source_t source, ddb_medialib_item_t *_list) {
    ml_tree_item_t *list = (ml_tree_item_t *)_list;
    while (list) {
        ml_tree_item_t *next = list->next;
        if (list->children) {
            ml_free_list(source, (ddb_medialib_item_t *)list->children);
            list->children = NULL;
        }
        if (list->track) {
            deadbeef->pl_item_unref (list->track);
        }
        if (list->text) {
            deadbeef->metacache_remove_string (list->text);
        }

        free (list);
        list = next;
    }
}

void
ml_tree_init (DB_functions_t *_deadbeef) {
    deadbeef = _deadbeef;
    artist_album_bc = deadbeef->tf_compile ("[%album artist% - ]%album%");
    title_bc = deadbeef->tf_compile ("[%tracknumber%. ]%title%");
}

void
ml_tree_free (void) {
    if (artist_album_bc) {
        deadbeef->tf_free (artist_album_bc);
        artist_album_bc = NULL;
    }

    if (title_bc) {
        deadbeef->tf_free (title_bc);
        title_bc = NULL;
    }
}
