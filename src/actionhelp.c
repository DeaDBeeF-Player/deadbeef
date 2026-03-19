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

    const int NAME_COLUMN = 40;
    const int INITIAL_BUF = 4096;

    size_t capacity = INITIAL_BUF;
    size_t length = 0;

    char *buf = malloc(capacity);
    if (!buf) {
        return NULL;
    }

    buf[0] = 0;

#define APPEND(...) do { \
int needed = snprintf(NULL, 0, __VA_ARGS__); \
if (length + needed + 1 > capacity) { \
capacity = (length + needed + 1) * 2; \
buf = realloc(buf, capacity); \
} \
length += snprintf(buf + length, capacity - length, __VA_ARGS__); \
} while (0)

    APPEND(_("Title"));

    for (int i = 5; i < NAME_COLUMN; i++) {
        APPEND(" ");
    }

    APPEND("|  %s\n", _("Name"));

    int total_width = NAME_COLUMN + 40;

    for (int i = 0; i < total_width; i++) {
        if (i == NAME_COLUMN) {
            APPEND("|");
        } else {
            APPEND("-");
        }
    }

    APPEND("\n");

    // Build an alphabetically-sorted list of actions from all plugins

    int cap_list = 64, count = 0;
    DB_plugin_action_t **list = malloc(sizeof(*list) * cap_list);

    if (!list) {
        free(buf);
        return NULL;
    }

    for (int i = 0; plugins[i]; i++) {
        if (!plugins[i]->get_actions) {
            continue;
        }

        DB_plugin_action_t *a = plugins[i]->get_actions(NULL);

        while (a) {
            if (count == cap_list) {
                cap_list *= 2;
                list = realloc(list, sizeof(*list) * cap_list);
            }

            list[count++] = a;
            a = a->next;
        }
    }

    if (count == 0) {
        free(list);
        return buf;
    }

    qsort(list, count, sizeof(*list), action_alphasort);

    // Build action tree based on '/' separators

    const char *prev = "";

    for (int i = 0; i < count; i++) {
        const char *cur = list[i]->title;
        const char *p_prev = prev;
        const char *p_cur  = cur;
        int depth = 0;

        while (*p_cur) {
            const char *c_start = p_cur;
            int c_len = 0;

            while (*p_cur) {
                if (p_cur[0] == '\\' && p_cur[1] == '/') {
                    p_cur += 2;
                    c_len++;
                    continue;
                }

                if (*p_cur == '/') {
                    break;
                }

                p_cur++;
                c_len++;
            }

            int match = 0;

            if (*p_prev) {
                const char *p_start = p_prev;
                int p_len = 0;

                while (*p_prev) {
                    if (p_prev[0] == '\\' && p_prev[1] == '/') {
                        p_prev += 2;
                        p_len++;
                        continue;
                    }

                    if (*p_prev == '/') {
                        break;
                    }

                    p_prev++;
                    p_len++;
                }

                if (c_len == p_len && strncmp(c_start, p_start, c_len) == 0) {
                    match = 1;
                }

                if (*p_prev == '/') {
                    p_prev++;
                }
            }

            if (!match) {
                const char *s = c_start;
                int level = depth;

                while (1) {
                    int indent = level * 4;

                    for (int k = 0; k < indent; k++) {
                        APPEND(" ");
                    }

                    const char *tmp = s;
                    int seg_len = 0;

                    while (*tmp) {
                        if (tmp[0] == '\\' && tmp[1] == '/') {
                            tmp += 2;
                            seg_len++;
                            continue;
                        }

                        if (*tmp == '/') {
                            break;
                        }

                        tmp++;
                        seg_len++;
                    }

                    {
                        const char *p = s;
                        int remaining = seg_len;

                        char *title = calloc(remaining + 1, 1);
                        char *titlep = title;

                        while (remaining > 0) {
                            if (p[0] == '\\' && p[1] == '/') {
                                *titlep++ = '/';
                                p += 2;
                                remaining--;
                            } else {
                                *titlep++ = *p++;
                                remaining--;
                            }
                        }
                        *titlep = 0;
                        APPEND("%s", title);
                        free(title);
                        title = NULL;
                        titlep = NULL;
                    }

                    if (*tmp == 0) {
                        int pos = indent + seg_len;

                        if (pos < NAME_COLUMN) {
                            for (int k = pos; k < NAME_COLUMN; k++) {
                                APPEND(" ");
                            }
                        } else {
                            APPEND(" ");
                        }

                        APPEND("|  %s", list[i]->name);
                    }

                    APPEND("\n");

                    if (*tmp == '/') {
                        s = tmp + 1;
                        level++;
                    } else {
                        break;
                    }
                }

                break;
            }

            depth++;

            if (*p_cur == '/') {
                p_cur++;
            } else {
                break;
            }
        }

        prev = cur;
    }

    free(list);
    return buf;

#undef APPEND
}
