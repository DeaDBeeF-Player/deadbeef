/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2016 Oleksiy Yakovenko and other contributors

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
#include <deadbeef/deadbeef.h>
#include "threading.h"

typedef struct logger_s {
    void (*log) (DB_plugin_t *plugin, uint32_t layers, const char *text, void *ctx);
    void *ctx;
    struct logger_s *next;
} logger_t;

static uint32_t _log_layers = DDB_LOG_LAYER_DEFAULT | DDB_LOG_LAYER_INFO;
static logger_t *_loggers;
static uint64_t _mutex;

// This buffer is used during the initialization time, before there are any listeners.
// As soon as the first listener registers -- it gets a callback with this buffer, then buffering stops.
// Only error messages are buffered (DDB_LOG_LAYER_DEFAULT)
#define INIT_BUFFER_SIZE 32768
static char *init_buffer;
static char *init_buffer_ptr;

static char *init_buffer_info;
static char *init_buffer_info_ptr;

#ifdef ANDROID
#    include <android/log.h>
static void
console_write (const char *text) {
    __android_log_write (ANDROID_LOG_INFO, ANDROID_LOGGER_TAG, text);
}
#else
static void
console_write (const char *text) {
    fwrite (text, strlen (text), 1, stderr);
    fflush (stderr);
}
#endif

static void
_log_internal (DB_plugin_t *plugin, uint32_t layers, const char *text) {
    mutex_lock (_mutex);
    console_write (text);
    size_t len = strlen (text);
    for (logger_t *l = _loggers; l; l = l->next) {
        l->log (plugin, layers, text, l->ctx);
    }
    if (init_buffer && (layers == DDB_LOG_LAYER_DEFAULT)) {
        if (init_buffer_ptr - init_buffer + len + 1 < INIT_BUFFER_SIZE) {
            memcpy (init_buffer_ptr, text, len);
            init_buffer_ptr += len;
            *init_buffer_ptr = 0;
        }
    }
    if (init_buffer_info && (layers == DDB_LOG_LAYER_INFO)) {
        if (init_buffer_info_ptr - init_buffer_info + len + 1 < INIT_BUFFER_SIZE) {
            memcpy (init_buffer_info_ptr, text, len);
            init_buffer_info_ptr += len;
            *init_buffer_info_ptr = 0;
        }
    }
    mutex_unlock (_mutex);
}

static int
_is_log_visible (DB_plugin_t *plugin, uint32_t layers) {
    if (plugin && !(plugin->flags & DDB_PLUGIN_FLAG_LOGGING)) {
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
    init_buffer = calloc (1, INIT_BUFFER_SIZE);
    init_buffer_ptr = init_buffer;
    init_buffer_info = calloc (1, INIT_BUFFER_SIZE);
    init_buffer_info_ptr = init_buffer_info;
    return 0;
}

void
ddb_logger_stop_buffering (void) {
    mutex_lock (_mutex);
    if (init_buffer) {
        free (init_buffer);
        init_buffer = init_buffer_ptr = NULL;
    }
    if (init_buffer_info) {
        free (init_buffer_info);
        init_buffer_info = init_buffer_info_ptr = NULL;
    }
    mutex_unlock (_mutex);
}

void
ddb_logger_free (void) {
    if (_mutex) {
        mutex_lock (_mutex);

        ddb_logger_stop_buffering ();

        while (_loggers) {
            ddb_log_viewer_unregister (_loggers->log, _loggers->ctx);
        }

        mutex_free (_mutex);
        _mutex = 0;
    }
}

void
ddb_log_detailed (DB_plugin_t *plugin, uint32_t layers, const char *fmt, ...) {
    if (!_is_log_visible (plugin, layers)) {
        return;
    }

    char text[2048];
    va_list ap;
    va_start (ap, fmt);
    (void)vsnprintf (text, sizeof (text), fmt, ap);
    va_end (ap);

    _log_internal (plugin, layers, text);
}

void
ddb_vlog_detailed (DB_plugin_t *plugin, uint32_t layers, const char *fmt, va_list ap) {
    if (!_is_log_visible (plugin, layers)) {
        return;
    }

    char text[2048];
    (void)vsnprintf (text, sizeof (text), fmt, ap);

    _log_internal (plugin, layers, text);
}

void
ddb_log (const char *fmt, ...) {
    char text[2048];
    va_list ap;
    va_start (ap, fmt);
    (void)vsnprintf (text, sizeof (text), fmt, ap);
    va_end (ap);

    _log_internal (NULL, 0, text);
}

void
ddb_vlog (const char *fmt, va_list ap) {
    char text[2048];
    (void)vsnprintf (text, sizeof (text), fmt, ap);

    _log_internal (NULL, 0, text);
}

void
ddb_log_viewer_register (
    void (*callback) (DB_plugin_t *plugin, uint32_t layers, const char *text, void *ctx),
    void *ctx) {
    mutex_lock (_mutex);

    for (logger_t *l = _loggers; l; l = l->next) {
        if (l->log == callback && l->ctx == ctx) {
            mutex_unlock (_mutex);
            return;
        }
    }

    logger_t *logger = calloc (1, sizeof (logger_t));
    logger->log = callback;
    logger->ctx = ctx;
    logger->next = _loggers;

    _loggers = logger;

    if (init_buffer) {
        if (*init_buffer) {
            callback (NULL, DDB_LOG_LAYER_DEFAULT, init_buffer, ctx);
        }
    }

    if (init_buffer_info) {
        if (*init_buffer_info) {
            callback (NULL, DDB_LOG_LAYER_INFO, init_buffer_info, ctx);
        }
    }

    ddb_logger_stop_buffering ();

    mutex_unlock (_mutex);
}

void
ddb_log_viewer_unregister (
    void (*callback) (DB_plugin_t *plugin, uint32_t layers, const char *text, void *ctx),
    void *ctx) {
    mutex_lock (_mutex);
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
    mutex_unlock (_mutex);
}
