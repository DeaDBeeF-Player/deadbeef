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

typedef struct logger_s {
    void (*log) (DB_plugin_t *plugin, uint32_t layers, const char *text);
    struct logger_s *next;
} logger_t;

static uint32_t _log_layers = DDB_LOG_LAYER_DEFAULT | DDB_LOG_LAYER_INFO;
static logger_t *_loggers;

static void
_log_internal (DB_plugin_t *plugin, uint32_t layers, const char *text) {
    fwrite (text, strlen(text), 1, stderr);
    for (logger_t *l = _loggers; l; l = l->next) {
        l->log (plugin, layers, text);
    }
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
ddb_log_viewer_register (void (*callback)(DB_plugin_t *plugin, uint32_t layers, const char *text)) {
    logger_t *logger = calloc (sizeof (logger_t), 1);
    logger->log = callback;
    logger->next = _loggers;
    _loggers = logger;
}

void
ddb_log_viewer_unregister (void (*callback)(DB_plugin_t *plugin, uint32_t layers, const char *text)) {
    logger_t *prev = NULL;
    for (logger_t *l = _loggers; l; l = l->next) {
        if (l->log == callback) {
            if (prev) {
                prev->next = l->next;
            }
            else {
                _loggers = l->next;
            }
            free (l);
            return;
        }
    }
}
