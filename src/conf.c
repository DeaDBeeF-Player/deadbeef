/*
  deadbeef config file manager
  http://deadbeef.sourceforge.net

  Copyright (C) 2009-2013 Oleksiy Yakovenko

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

  Oleksiy Yakovenko waker@users.sourceforge.net
*/
#ifdef HAVE_CONFIG_H
#    include "config.h"
#endif
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <inttypes.h>
#include <errno.h>
#include <unistd.h>
#include <limits.h>
#include <sys/stat.h>
#include "buffered_file_writer.h"
#include "conf.h"
#include "threading.h"
#include <deadbeef/common.h>

static DB_conf_item_t *conf_items;
static int changed;
static uintptr_t mutex;
static int disable_saving;

void
conf_init (void) {
    mutex = mutex_create ();
}

void
conf_lock (void) {
    mutex_lock (mutex);
}

void
conf_unlock (void) {
    mutex_unlock (mutex);
}

void
conf_free (void) {
    mutex_lock (mutex);
    DB_conf_item_t *next = NULL;
    for (DB_conf_item_t *it = conf_items; it; it = next) {
        next = it->next;
        conf_item_free (it);
    }
    conf_items = NULL;
    changed = 0;
    mutex_unlock (mutex);
    mutex_free (mutex);
    mutex = 0;
}

static void
_conf_load_buffer (char *buffer) {
    uint8_t *str = buffer;
    int line = 0;

    while (*str) {
        line++;
        uint8_t *estr = str;
        while (*estr >= 0x20) {
            estr++;
        }
        *estr = 0;

        if (str[0] == '#' || str[0] <= 0x20) {
            str = estr + 1;
            continue;
        }
        uint8_t *p = (uint8_t *)str;
        while (*p && *p > 0x20) {
            p++;
        }
        if (!*p) {
            trace_err ("error in config file line %d\n", line);
            str = estr + 1;
            continue;
        }
        *p = 0;
        p++;
        // skip whitespace
        while (*p && *p <= 0x20) {
            p++;
        }
        uint8_t *value = p;
        // remove trailing trash
        while (*p && *p >= 0x20) {
            p++;
        }
        *p = 0;
        // new items are appended, to preserve order
        conf_set_str ((const char *)str, (const char *)value);
        str = estr + 1;
    }
}

static int
_conf_load_file (const char *fname) {
    FILE *fp = fopen (fname, "rb");
    if (!fp) {
        // we're not logging the error when config could not be loaded -- it's the first run
        fp = fopen (fname, "w+b");
        if (!fp) {
            // we do log the error when we could not create the file
            trace_err ("Configuration file could not be created: %s\n", fname);
            return -1;
        }
        fclose (fp);
        return 0;
    }
    fseek (fp, 0, SEEK_END);
    size_t l = ftell (fp);
    rewind (fp);

    uint8_t *buffer = malloc (l + 1);
    if (l != fread (buffer, 1, l, fp)) {
        free (buffer);
        trace_err ("failed to read entire config file to memory\n");
        fclose (fp);
        return -1;
    }
    buffer[l] = 0;
    fclose (fp);
    fp = NULL;

    conf_lock ();

    _conf_load_buffer (buffer);

    conf_unlock ();

    free (buffer);

    return 0;
}

int
conf_load (void) {
    char config[PATH_MAX];
    snprintf (config, sizeof (config), "%s/config", dbconfdir);
    char secrets[PATH_MAX];
    snprintf (secrets, sizeof (secrets), "%s/secrets", dbstatedir);

    int config_res = _conf_load_file (config);
    int secrets_res = _conf_load_file (secrets);

    changed = 0;

    if (0 != config_res || 0 != secrets_res) {
        return -1;
    }

    return 0;
}

static int
_conf_save_with_secrets (int secrets) {
    char tempfile[PATH_MAX];
    char str[PATH_MAX];
    FILE *fp = NULL;
    buffered_file_writer_t *writer = NULL;

    if (!secrets) {
        snprintf (tempfile, sizeof (tempfile), "%s/config.tmp", dbconfdir);
        snprintf (str, sizeof (str), "%s/config", dbconfdir);
    }
    else {
        snprintf (tempfile, sizeof (tempfile), "%s/secrets.tmp", dbstatedir);
        snprintf (str, sizeof (str), "%s/secrets", dbstatedir);
    }

    conf_lock ();

    if (!changed) {
        conf_unlock ();
        return 0;
    }

    fp = fopen (tempfile, "w+b");
    if (!fp) {
        trace_err ("failed to open config file %s for writing\n", tempfile);
        conf_unlock ();
        return -1;
    }

    writer = buffered_file_writer_new (fp, 64 * 1024);

    for (DB_conf_item_t *it = conf_items; it; it = it->next) {
        int is_secret = (strstr (it->key, ".secret.") != NULL);

        if (secrets != is_secret) {
            continue;
        }

        if (buffered_file_writer_write (writer, it->key, strlen (it->key)) < 0) {
            goto error;
        }
        if (buffered_file_writer_write (writer, " ", 1) < 0) {
            goto error;
        }
        if (buffered_file_writer_write (writer, it->value, strlen (it->value)) < 0) {
            goto error;
        }
        if (buffered_file_writer_write (writer, "\n", 1) < 0) {
            goto error;
        }
    }
    if (buffered_file_writer_flush (writer) < 0) {
        goto error;
    }
    buffered_file_writer_free (writer);
    writer = NULL;
    if (EOF == fclose (fp)) {
        fp = NULL;
        goto error;
    }
    int err = rename (tempfile, str);
    if (err != 0) {
        trace_err ("config rename %s -> %s failed: %s\n", tempfile, str, strerror (errno));
    }
    else {
        chmod (str, 0600);
    }
    conf_unlock ();
    return 0;
error:
    if (fp != NULL) {
        fclose (fp);
    }
    conf_unlock ();
    trace_err ("failed to write to file %s (%s)\n", tempfile, strerror (errno));
    if (writer != NULL) {
        buffered_file_writer_free (writer);
    }

    return -1;
}

int
conf_save (void) {
    if (disable_saving) {
        return 0;
    }

    if (!mutex) {
        return 0;
    }

    int res = _conf_save_with_secrets (0);
    int res_secrets = _conf_save_with_secrets (1);

    changed = 0;

    if (0 != res || 0 != res_secrets) {
        return -1;
    }

    return 0;
}

void
conf_item_free (DB_conf_item_t *it) {
    conf_lock ();
    if (it) {
        if (it->key) {
            free (it->key);
        }
        if (it->value) {
            free (it->value);
        }
        free (it);
    }
    conf_unlock ();
}

const char *
conf_get_str_fast (const char *key, const char *def) {
    for (DB_conf_item_t *it = conf_items; it; it = it->next) {
        if (!strcasecmp (key, it->key)) {
            return it->value;
        }
    }
    return def;
}

void
conf_get_str (const char *key, const char *def, char *buffer, int buffer_size) {
    conf_lock ();
    const char *out = conf_get_str_fast (key, def);
    if (out) {
        size_t n = strlen (out) + 1;
        n = min (n, buffer_size);
        memcpy (buffer, out, n);
        buffer[buffer_size - 1] = 0;
    }
    else {
        *buffer = 0;
    }
    conf_unlock ();
}

float
conf_get_float (const char *key, float def) {
    conf_lock ();
    const char *v = conf_get_str_fast (key, NULL);
    float res = v ? (float)atof (v) : def;
    conf_unlock ();
    return res;
}

int
conf_get_int (const char *key, int def) {
    conf_lock ();
    const char *v = conf_get_str_fast (key, NULL);
    int res = v ? atoi (v) : def;
    conf_unlock ();
    return res;
}

int64_t
conf_get_int64 (const char *key, int64_t def) {
    conf_lock ();
    const char *v = conf_get_str_fast (key, NULL);
    int64_t res = v ? atoll (v) : def;
    conf_unlock ();
    return res;
}

DB_conf_item_t *
conf_find (const char *group, DB_conf_item_t *prev) {
    size_t l = strlen (group);
    for (DB_conf_item_t *it = prev ? prev->next : conf_items; it; it = it->next) {
        if (!strncasecmp (group, it->key, l)) {
            return it;
        }
    }
    return NULL;
}

void
conf_set_str (const char *key, const char *val) {
    conf_lock ();
    DB_conf_item_t *prev = NULL;
    for (DB_conf_item_t *it = conf_items; it; it = it->next) {
        int cmp = strcasecmp (key, it->key);
        if (!cmp) {
            if (val == NULL) {
                if (prev != NULL) {
                    prev->next = it->next;
                }
                else {
                    conf_items = it->next;
                }
                conf_item_free (it);
                conf_unlock ();
                return;
            }

            if (!strcmp (it->value, val)) {
                conf_unlock ();
                return;
            }
            free (it->value);
            it->value = strdup (val);
            conf_unlock ();
            changed = 1;
            return;
        }
        else if (cmp < 0) {
            break;
        }
        prev = it;
    }
    if (!val) {
        conf_unlock ();
        return;
    }
    DB_conf_item_t *it = malloc (sizeof (DB_conf_item_t));
    memset (it, 0, sizeof (DB_conf_item_t));
    it->key = strdup (key);
    it->value = strdup (val);
    changed = 1;
    if (prev) {
        DB_conf_item_t *next = prev->next;
        prev->next = it;
        it->next = next;
    }
    else {
        it->next = conf_items;
        conf_items = it;
    }
    conf_unlock ();
}

void
conf_set_int (const char *key, int val) {
    char s[10];
    snprintf (s, sizeof (s), "%d", val);
    conf_set_str (key, s);
}

void
conf_set_int64 (const char *key, int64_t val) {
    char s[20];
    snprintf (s, sizeof (s), "%" PRId64, val);
    conf_set_str (key, s);
}

void
conf_set_float (const char *key, float val) {
    char s[10];
    snprintf (s, sizeof (s), "%0.7f", val);
    conf_set_str (key, s);
}
void
conf_remove_items (const char *key) {
    size_t l = strlen (key);
    conf_lock ();
    DB_conf_item_t *prev = NULL;
    DB_conf_item_t *it;
    for (it = conf_items; it; prev = it, it = it->next) {
        if (!strncasecmp (key, it->key, l)) {
            changed = 1;
            break;
        }
    }
    DB_conf_item_t *next = NULL;
    while (it) {
        next = it->next;
        conf_item_free (it);
        it = next;
        if (!it || strncasecmp (key, it->key, l)) {
            break;
        }
    }
    if (prev) {
        prev->next = next;
    }
    else {
        conf_items = next;
    }
    conf_unlock ();
}

void
conf_enable_saving (int enable) {
    disable_saving = !enable;
}
