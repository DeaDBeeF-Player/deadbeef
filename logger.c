/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2016 Alexey Yakovenko and other contributors

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
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include "logger.h"
#include "deadbeef.h"
#include "threading.h"

typedef struct logger_s {
    void (*log) (DB_plugin_t *plugin, uint32_t layers, const char *text, void *ctx);
    void *ctx;
    struct logger_s *next;
} logger_t;

static uint32_t _log_layers = DDB_LOG_LAYER_DEFAULT | DDB_LOG_LAYER_INFO;
static logger_t *_loggers;
static uint64_t _mutex;

static void
_log_internal (DB_plugin_t *plugin, uint32_t layers, const char *text) {
    fwrite (text, strlen(text), 1, stderr);
    mutex_lock (_mutex);
    for (logger_t *l = _loggers; l; l = l->next) {
        l->log (plugin, layers, text, l->ctx);
    }
    mutex_unlock(_mutex);
}

static int
_is_log_visible (DB_plugin_t *plugin, uint32_t layers) {
    if (plugin && !(plugin->flags&DDB_PLUGIN_FLAG_LOGGING)) {
        return 0;
    }
    if (layers && !(layers & _log_layers)) {
        return 0;
    }

    return 1;
}

int
ddb_logger_init (void) {
    _mutex = mutex_create ();
    if (!_mutex) {
        return -1;
    }
    return 0;
}

void
ddb_logger_free (void) {
    if (_mutex) {
        mutex_lock (_mutex);

        while (_loggers) {
            ddb_log_viewer_unregister(_loggers->log, _loggers->ctx);
        }

        mutex_free (_mutex);
        _mutex = 0;
    }
}

void
ddb_log_detailed (DB_plugin_t *plugin, uint32_t layers, const char *fmt, ...) {
    if (!_is_log_visible(plugin, layers)) {
        return;
    }

    char text[2048];
    va_list ap;
    va_start(ap, fmt);
    (void) vsnprintf(text, sizeof (text), fmt, ap);
    va_end(ap);

    _log_internal (plugin, layers, text);
}

void
ddb_vlog_detailed (DB_plugin_t *plugin, uint32_t layers, const char *fmt, va_list ap) {
    if (!_is_log_visible(plugin, layers)) {
        return;
    }

    char text[2048];
    (void) vsnprintf(text, sizeof (text), fmt, ap);

    _log_internal (plugin, layers, text);
}

void
ddb_log (const char *fmt, ...) {
    char text[2048];
    va_list ap;
    va_start(ap, fmt);
    (void) vsnprintf(text, sizeof (text), fmt, ap);
    va_end(ap);

    _log_internal (NULL, 0, text);
}

void
ddb_vlog (const char *fmt, va_list ap) {
    char text[2048];
    (void) vsnprintf(text, sizeof (text), fmt, ap);

    _log_internal (NULL, 0, text);
}

void
ddb_log_viewer_register (void (*callback)(DB_plugin_t *plugin, uint32_t layers, const char *text, void *ctx), void *ctx) {
    mutex_lock(_mutex);

    for (logger_t *l = _loggers; l; l = l->next) {
        if (l->log == callback && l->ctx == ctx) {
            mutex_unlock(_mutex);
            return;
        }
    }

    logger_t *logger = calloc (sizeof (logger_t), 1);
    logger->log = callback;
    logger->ctx = ctx;
    logger->next = _loggers;

    _loggers = logger;
    mutex_unlock(_mutex);
}

void
ddb_log_viewer_unregister (void (*callback)(DB_plugin_t *plugin, uint32_t layers, const char *text, void *ctx), void *ctx) {
    mutex_lock(_mutex);
    logger_t *prev = NULL;
    for (logger_t *l = _loggers; l; l = l->next) {
        if (l->log == callback && l->ctx == ctx) {
            if (prev) {
                prev->next = l->next;
            }
            else {
                _loggers = l->next;
            }
            free (l);
            break;
        }
        prev = l;
    }
    mutex_unlock(_mutex);
}
