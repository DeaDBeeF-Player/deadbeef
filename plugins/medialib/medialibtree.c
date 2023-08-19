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
_tree_item_alloc (const char *path) {
    ml_tree_item_t *item = calloc (1, sizeof (ml_tree_item_t));
    if (path != NULL) {
        item->path = deadbeef->metacache_add_string(path);
    }
    return item;
}

static ddb_playItem_t *
_create_sorted_tree(
    ddb_playlist_t *plt,
    ml_tree_item_t *parent,
    int selected,
    ddb_playItem_t *first,
    char * const * const tfs,
    char * const * const text_tfs,
    size_t tfs_count,
    int level
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

        deadbeef->tf_eval(&ctx, tfs[group_level], next_text, sizeof (next_text));

        size_t len = strlen(next_text);

        // Compare group text
        int is_first_item = group_text[0] == 0;
        int group_did_change = is_first_item || memcmp(group_text, next_text, len);
        int is_leaf = level == tfs_count - 1;
        if (is_first_item || group_did_change != is_leaf) {
            // Create new item
            memcpy(group_text, next_text, len+1);

            size_t parent_len = parent->path ? strlen(parent->path) : 0;
            size_t node_len = strlen(next_text);
            size_t buffer_len = parent_len + node_len + 2;
            char *tree_path = malloc(buffer_len);
            snprintf(tree_path, buffer_len, "%s/%s", parent->path ?: "", next_text);

            group = _tree_item_alloc(tree_path);
            free (tree_path);

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
            ddb_playItem_t *new_next = _create_sorted_tree(plt, group, selected, next, tfs, text_tfs, tfs_count, level+1);
            deadbeef->pl_item_unref(next);
            next = new_next;
        }

        if (next == NULL) {
            break;
        }

        if (is_leaf && (!group_did_change || is_first_item)) {
            ddb_playItem_t *it = next;
            next = deadbeef->pl_get_next(it, PL_MAIN);
            deadbeef->pl_item_unref(it);
        }

        if (!is_leaf && level > 0) {
            break;
        }

        if (is_leaf && group_did_change && !is_first_item) {
            break;
        }

    } while (next != NULL);

    return next;
}

static void
_create_tf_tree(medialib_source_t *source, ml_tree_item_t *root, int selected, const char **tfs, int tfs_count) {
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

    deadbeef->plt_sort_v2(source->ml_playlist, PL_MAIN, -1, tf_sort, DDB_SORT_ASCENDING);

    _create_sorted_tree(source->ml_playlist, root, selected, NULL, bcs, text_bcs, tfs_count, 0);

    for (int i = 0; i < tfs_count; i++) {
        deadbeef->tf_free(bcs[i]);
        deadbeef->tf_free(text_bcs[i]);
    }
    free (bcs);
    free (text_bcs);
    free (tf_sort);
}

static bool
_path_equal_to_depth(const char *path1, const char *path2, int depth) {
    depth++;
    while (*path1 == *path2) {
        if (*path1 == '/') {
            if (--depth < 0) {
                return true;
            }
        }
        path1++;
        path2++;
    }
    return false;
}

static bool
_is_last_path_component(const char *path, int depth) {
    depth++;
    while (*path) {
        if (*path == '/') {
            if (--depth < 0) {
                path++;
                break;
            }
        }
        path++;
    }

    return *path == 0;
}

static bool
_get_path_component(const char *path, int depth, char *output, size_t output_size) {
    while (*path) {
        if (*path == '/') {
            if (--depth < 0) {
                path++;
                break;
            }
        }
        path++;
    }

    if (*path == 0) {
        return false;
    }

    while (*path && *path != '/' && output_size > 0) {
        *output++ = *path++;
        output_size--;
    }

    if (output_size == 0) {
        return false;
    }

    *output = 0;

    return true;
}

static ddb_playItem_t *
_create_sorted_folder_tree(ddb_playlist_t *plt, ml_tree_item_t *parent, int selected, const char *track_tf_bc, ddb_playItem_t *first, int level) {
    // setup iteration
    char group_path[1000] = "";
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

    ml_tree_item_t *group = NULL;

    ml_tree_item_t *tail = parent->children;
    while (tail && tail->next) {
        tail = tail->next;
    }

    int group_did_change = 1;

    ddb_tf_context_t ctx = {0};
    ctx._size = sizeof (ddb_tf_context_t);
    ctx.flags = DDB_TF_CONTEXT_NO_MUTEX_LOCK | DDB_TF_CONTEXT_NO_DYNAMIC;
    ctx.plt = plt;
    ctx.iter = PL_MAIN;

    for (;;) {
        if (selected && !deadbeef->pl_is_selected(next)) {
            ddb_playItem_t *it = deadbeef->pl_get_next(next, PL_MAIN);
            deadbeef->pl_item_unref(next);
            next = it;
            continue; // filter
        }

        // get current path
        const char *path = deadbeef->pl_find_meta_raw(next, ":URI");
        char next_text[1000];

        bool should_create_node = false;
        bool is_leaf = false;

        if (_is_last_path_component(path, level)) {
            // create node, next, return
            should_create_node = true;
            is_leaf = true;
        }
        else {
            should_create_node = group_did_change;
        }

        if (should_create_node) {
            // new group
            strcpy (group_path, path);

            if (is_leaf) {
                ctx.it = next;
                deadbeef->tf_eval(&ctx, track_tf_bc, next_text, sizeof (next_text));
            }
            else {
                bool res = _get_path_component(path, level, next_text, sizeof (next_text));
                if (!res) {
                    strcpy(next_text, "<?>");
                }
            }

            size_t parent_len = parent->path ? strlen(parent->path) : 0;
            size_t node_len = strlen(next_text);
            size_t buffer_len = parent_len + node_len + 2;
            char *tree_path = malloc(buffer_len);
            snprintf(tree_path, buffer_len, "%s/%s", parent->path ?: "", next_text);

            group = _tree_item_alloc(tree_path);
            free (tree_path);

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

        if (is_leaf) {
            ddb_playItem_t *it = next;
            next = deadbeef->pl_get_next(it, PL_MAIN);
            deadbeef->pl_item_unref(it);
        }
        else {
            // recurse into subgroups
            ddb_playItem_t *new_next = _create_sorted_folder_tree(plt, group, selected, track_tf_bc, next, level+1);
            deadbeef->pl_item_unref(next);
            next = new_next;
        }

        if (next == NULL) {
            break;
        }

        // compare with previous path at level
        path = deadbeef->pl_find_meta_raw(next, ":URI");
        group_did_change = !_path_equal_to_depth(path, group_path, level);

        if (group_did_change) {
            break;
        }
    }

    return next;
}

static void
_create_folder_tree(medialib_source_t *source, ml_tree_item_t *root, int selected) {
    const char *sort_tf = "$directory_path(%path%)/[%album artist% - ]%album%/[%tracknumber%. ]%title%";
    deadbeef->plt_sort_v2(source->ml_playlist, PL_MAIN, -1, sort_tf, DDB_SORT_ASCENDING);

    char *track_tf_bc = deadbeef->tf_compile("[%tracknumber%. ]%title%");

    _create_sorted_folder_tree(source->ml_playlist, root, selected, track_tf_bc, NULL, 0);

    deadbeef->tf_free(track_tf_bc);

    // squash single-item tree nodes
    ml_tree_item_t *prev = NULL;
    for (ml_tree_item_t *head = root->children; head != NULL; prev = head, head = head->next) {
        while (head->num_children == 1) {
            ml_tree_item_t *new_head = head->children;
            new_head->next = head->next;

            deadbeef->pl_item_unref(head->track);
            deadbeef->metacache_remove_string(head->text);
            if (head->path) {
                deadbeef->metacache_remove_string(head->path);
            }
            free (head);
            head = new_head;
            if (prev != NULL) {
                prev->next = head;
            }
            else {
                root->children = head;
            }
        }
    }
}

ml_tree_item_t *
_create_item_tree_from_collection(const char *filter, medialibSelector_t index, medialib_source_t *source) {
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
    root->path = deadbeef->metacache_add_string(root->text);

    if (source->ml_playlist == NULL) {
        return root;
    }

    if (index == SEL_FOLDERS) {
        _create_folder_tree(source, root, selected);
    }
    else if (index == SEL_ARTISTS) {
        // list of albums for artist
        const char *tfs[] = {
            "$if2(%album artist%,\\<?\\>)",
            "[%album artist% - ]%album%",
            "[%tracknumber%. ]%title%"
        };

        _create_tf_tree(source, root, selected, tfs, 3);
    }
    else if (index == SEL_GENRES) {
        // list of albums for genre
        const char *tfs[] = {
            "$if2(%genre%,\\<?\\>)",
            "[%album artist% - ]%album%",
            "[%tracknumber%. ]%title%"
        };

        _create_tf_tree(source, root, selected, tfs, 3);
    }
    else if (index == SEL_ALBUMS) {
        // list of tracks for album
        const char *tfs[] = {
            "[%album artist% - ]%album%",
            "[%tracknumber%. ]%title%"
        };

        _create_tf_tree(source, root, selected, tfs, 2);
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
        if (list->path) {
            deadbeef->metacache_remove_string (list->path);
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
