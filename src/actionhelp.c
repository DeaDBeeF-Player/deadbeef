/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2026 Oleksiy Yakovenko and other contributors

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <deadbeef/deadbeef.h>
#include "plugins.h"
#include "actionhelp.h"
#include "gettext.h"

static int
action_alphasort(const void *a, const void *b)
{
    const DB_plugin_action_t *A = *(DB_plugin_action_t **)a;
    const DB_plugin_action_t *B = *(DB_plugin_action_t **)b;
    return strcmp(A->title, B->title);
}

char *
build_actions_string(void) {
    DB_plugin_t **plugins = plug_get_list();

    const int MAX_DEPTH = 32;
    const int NAME_COLUMN = 40;
    const int INITIAL_BUF = 4096;

    /* ---------- dynamic buffer ---------- */
    size_t buf_capacity = INITIAL_BUF;
    size_t buf_len = 0;
    char *buf = malloc(buf_capacity);
    if (!buf) {
        return NULL;
    }
    buf[0] = 0;

    /* ---------- helper to append to buffer ---------- */
#define APPEND(...) do { \
int needed = snprintf(NULL, 0, __VA_ARGS__); \
if (buf_len + needed + 1 > buf_capacity) { \
buf_capacity = (buf_len + needed + 1) * 2; \
buf = realloc(buf, buf_capacity); \
} \
buf_len += snprintf(buf + buf_len, buf_capacity - buf_len, __VA_ARGS__); \
} while(0)

    /* ---------- header ---------- */
    APPEND("\n");
    APPEND(_("List of all actions.\n"));
    APPEND(_("Perform actions using \"--action=NAME\" command line argument.\n\n"));
    APPEND(_("Title"));
    int header_pos = 5;
    for (int i = header_pos; i < NAME_COLUMN; i++) {
        APPEND(" ");
    }
    APPEND("|  ");
    APPEND(_("Name"));
    APPEND("\n");

    /* ---------- full-width divider ---------- */
    int total_width = NAME_COLUMN + 40; // approximate total width
    for (int i = 0; i < total_width; i++) {
        APPEND(i == NAME_COLUMN ? "|" : "-");
    }
    APPEND("\n");

    /* ---------- collect actions ---------- */
    int capacity = 64, count = 0;
    DB_plugin_action_t **list = malloc(sizeof(DB_plugin_action_t *) * capacity);
    if (!list) {
        free(buf);
        return NULL;
    }

    for (int pi = 0; plugins[pi] != NULL; pi++) {
        DB_plugin_t *plugin = plugins[pi];
        if (!plugin->get_actions) {
            continue;
        }
        DB_plugin_action_t *action = plugin->get_actions(NULL);
        while (action) {
            if (count == capacity) {
                capacity *= 2;
                list = realloc(list, sizeof(DB_plugin_action_t*) * capacity);
            }
            list[count++] = action;
            action = action->next;
        }
    }

    if (count == 0) {
        free(list);
        return buf;
    }

    /* ---------- sort ---------- */
    qsort(list, count, sizeof(DB_plugin_action_t *), action_alphasort);

    /* ---------- tree printing ---------- */
    char prev_parts[MAX_DEPTH][256];
    int prev_depth = 0;

    for (int ai = 0; ai < count; ai++) {
        const char *src = list[ai]->title;
        char parts[MAX_DEPTH][256];
        int depth = 0;

        /* split path handling escaped \/ */
        while (*src && depth < MAX_DEPTH) {
            int idx = 0;
            while (*src) {
                if (src[0] == '\\' && src[1] == '/') {
                    parts[depth][idx++] = '/';
                    src += 2;
                    continue;
                }
                if (*src == '/') {
                    break;
                }
                parts[depth][idx++] = *src++;
            }
            parts[depth][idx] = 0;
            depth++;
            if (*src == '/') {
                src++;
            }
            else {
                break;
            }
        }

        int shared = 0;
        while (shared < depth
               && shared < prev_depth
               && strcmp(parts[shared], prev_parts[shared]) == 0) {
            shared++;
        }

        for (int level = shared; level < depth; level++) {
            int indent = level * 4;
            for (int i = 0; i < indent; i++) {
                APPEND(" ");
            }
            APPEND("%s", parts[level]);
            int current_pos = indent + (int)strlen(parts[level]);
            if (level == depth - 1) {
                if (current_pos < NAME_COLUMN) {
                    for (int i = current_pos; i < NAME_COLUMN; i++) {
                        APPEND(" ");
                    }
                } else {
                    APPEND(" ");
                }
                APPEND("|  %s", list[ai]->name);
            }
            APPEND("\n");
            strcpy(prev_parts[level], parts[level]);
        }
        prev_depth = depth;
    }

    free(list);
    return buf;

#undef APPEND
}

