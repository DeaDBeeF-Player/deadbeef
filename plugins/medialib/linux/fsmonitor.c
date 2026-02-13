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

#include "fsmonitor.h"

#include <dirent.h>
#include <dispatch/dispatch.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/inotify.h>

typedef struct node {
    int wd;
    char *path;
    struct node *next;
} node_t;

struct ddb_fsmonitor_s {
    int fd;
    dispatch_queue_t queue;
    dispatch_source_t readSource;

    node_t *nodes;

    ddb_fsmonitor_callback_t cb;
    void *userdata;
};

#define DDB_FSMON_MASK \
(IN_CREATE | IN_DELETE | IN_MODIFY | \
IN_MOVED_FROM | IN_MOVED_TO | \
IN_ATTRIB | IN_DELETE_SELF | IN_MOVE_SELF)

static node_t *
node_find(ddb_fsmonitor_t *m, int wd) {
    for (node_t *n = m->nodes; n; n = n->next)
        if (n->wd == wd) return n;
    return NULL;
}

static void
node_add(ddb_fsmonitor_t *m, int wd, const char *path) {
    node_t *n = malloc(sizeof(*n));
    n->wd = wd;
    n->path = strdup(path);
    n->next = m->nodes;
    m->nodes = n;
}

static void
node_remove(ddb_fsmonitor_t *m, int wd) {
    node_t **pp = &m->nodes;
    while (*pp) {
        if ((*pp)->wd == wd) {
            node_t *dead = *pp;
            *pp = dead->next;
            free(dead->path);
            free(dead);
            return;
        }
        pp = &(*pp)->next;
    }
}

static void
add_recursive(ddb_fsmonitor_t *m, const char *path) {
    int wd = inotify_add_watch(m->fd, path, DDB_FSMON_MASK);
    if (wd < 0) return;

    node_add(m, wd, path);

    DIR *d = opendir(path);
    if (!d) return;

    struct dirent *e;
    while ((e = readdir(d))) {
        if (e->d_type == DT_DIR &&
            strcmp(e->d_name, ".") &&
            strcmp(e->d_name, "..")) {

            char buf[PATH_MAX];
            snprintf(buf, sizeof(buf), "%s/%s", path, e->d_name);
            add_recursive(m, buf);
        }
    }

    closedir(d);
}

static void
handle_events(ddb_fsmonitor_t *m) {
    char buf[8192];

    ssize_t len;
    while ((len = read(m->fd, buf, sizeof(buf))) > 0) {

        size_t i = 0;
        while (i < len) {
            struct inotify_event *ev =
            (struct inotify_event *)&buf[i];

            node_t *node = node_find(m, ev->wd);

            // New directory → add watch
            if (node &&
                (ev->mask & IN_CREATE) &&
                (ev->mask & IN_ISDIR)) {

                char p[PATH_MAX];
                snprintf(p, sizeof(p), "%s/%s",
                         node->path, ev->name);
                add_recursive(m, p);
            }

            // Watch removed / directory gone
            if (ev->mask & IN_IGNORED ||
                ev->mask & IN_DELETE_SELF ||
                ev->mask & IN_MOVE_SELF) {

                node_remove(m, ev->wd);
            }

            // Notify user
            if (m->cb)
                m->cb(m->userdata);

            i += sizeof(struct inotify_event) + ev->len;
        }
    }
}

ddb_fsmonitor_t *
ddb_fsmonitor_create(const char **paths,
                     size_t count,
                     ddb_fsmonitor_callback_t cb,
                     void *userdata)
{
    ddb_fsmonitor_t *m = calloc(1, sizeof(*m));

    m->fd = inotify_init1(IN_NONBLOCK);
    m->queue = dispatch_queue_create("ddb.fsmonitor", 0);
    m->cb = cb;
    m->userdata = userdata;

    for (size_t i = 0; i < count; i++)
        add_recursive(m, paths[i]);

    m->readSource =
    dispatch_source_create(DISPATCH_SOURCE_TYPE_READ,
                           m->fd, 0, m->queue);

    dispatch_source_set_event_handler(m->readSource, ^{
        handle_events(m);
    });

    dispatch_resume(m->readSource);

    return m;
}

void
ddb_fsmonitor_free(ddb_fsmonitor_t *m) {
    if (!m) return;

    dispatch_source_cancel(m->readSource);
    close(m->fd);

    node_t *n = m->nodes;
    while (n) {
        node_t *next = n->next;
        free(n->path);
        free(n);
        n = next;
    }

    free(m);
}
