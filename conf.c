/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009  Alexey Yakovenko

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "conf.h"

static DB_conf_item_t *conf_items;
static int changed = 0;

int
conf_load (void) {
    extern char dbconfdir[1024]; // $HOME/.config/deadbeef
    char str[1024];
    snprintf (str, 1024, "%s/config", dbconfdir);
    FILE *fp = fopen (str, "rt");
    if (!fp) {
        fprintf (stderr, "failed to load config file\n");
        return -1;
    }
    int line = 0;
    DB_conf_item_t *tail = NULL;
    while (fgets (str, 1024, fp) != NULL) {
        line++;
        if (str[0] == '#' || str[0] <= 0x20) {
            continue;
        }
        uint8_t *p = (uint8_t *)str;
        while (*p && *p > 0x20) {
            p++;
        }
        if (!*p) {
            fprintf (stderr, "error in config file line %d\n", line);
            continue;
        }
        *p = 0;
        p++;
        // skip whitespace
        while (*p && *p <= 0x20) {
            p++;
        }
        if (!*p) {
            fprintf (stderr, "error in config file line %d\n", line);
            continue;
        }
        char *value = p;
        // remove trailing trash
        while (*p && *p >= 0x20) {
            p++;
        }
        *p = 0;
        // new items are appended, to preserve order
        DB_conf_item_t *it = malloc (sizeof (DB_conf_item_t));
        memset (it, 0, sizeof (DB_conf_item_t));
        it->key = strdup (str);
        it->value = strdup (value);
        if (!tail) {
            conf_items = it;
        }
        else {
            tail->next = it;
        }
        tail = it;
    }
    fclose (fp);
    changed = 0;
    return 0;
}

int
conf_save (void) {
    extern char dbconfdir[1024]; // $HOME/.config/deadbeef
    char str[1024];
    snprintf (str, 1024, "%s/config", dbconfdir);
    FILE *fp = fopen (str, "w+t");
    if (!fp) {
        fprintf (stderr, "failed to open config file for writing\n");
        return -1;
    }
    for (DB_conf_item_t *it = conf_items; it; it = it->next) {
        fprintf (fp, "%s %s\n", it->key, it->value);
    }
    fclose (fp);
    return 0;
}

void
conf_free (void) {
    DB_conf_item_t *next = NULL;
    for (DB_conf_item_t *it = conf_items; it; it = next) {
        next = it->next;
        free (it->key);
        free (it->value);
        free (it);
    }
}

const char *
conf_get_str (const char *key, const char *def) {
    for (DB_conf_item_t *it = conf_items; it; it = it->next) {
        if (!strcasecmp (key, it->key)) {
            return it->value;
        }
    }
    return def;
}

float
conf_get_float (const char *key, float def) {
    const char *v = conf_get_str (key, NULL);
    return v ? atof (v) : def;
}

int
conf_get_int (const char *key, int def) {
    const char *v = conf_get_str (key, NULL);
    return v ? atoi (v) : def;
}

DB_conf_item_t *
conf_find (const char *group, DB_conf_item_t *prev) {
    int l = strlen (group);
    for (DB_conf_item_t *it = prev ? prev->next : conf_items; it; it = it->next) {
        if (!strncasecmp (group, it->key, l)) {
            return it;
        }
    }
    return NULL;
}

void
conf_set_str (const char *key, const char *val) {
    changed = 1;
    for (DB_conf_item_t *it = conf_items; it; it = it->next) {
        if (!strcasecmp (key, it->key)) {
            free (it->value);
            it->value = strdup (val);
            return;
        }
    }
    DB_conf_item_t *it = malloc (sizeof (DB_conf_item_t));
    memset (it, 0, sizeof (DB_conf_item_t));
    it->next = conf_items;
    it->key = strdup (key);
    it->value = strdup (val);
    conf_items = it;
}

void
conf_set_int (const char *key, int val) {
    char s[10];
    snprintf (s, sizeof (s), "%d", val);
    conf_set_str (key, s);
}

void
conf_set_float (const char *key, float val) {
    char s[10];
    snprintf (s, sizeof (s), "%0.7f", val);
    conf_set_str (key, s);
}

int
conf_ischanged (void) {
    return changed;
}

void
conf_setchanged (int c) {
    changed = c;
}
