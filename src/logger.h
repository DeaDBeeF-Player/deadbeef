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

#ifndef logger_h
#define logger_h

#include <deadbeef/deadbeef.h>

#ifdef __cplusplus
extern "C" {
#endif

int
ddb_logger_init (void);

void
ddb_logger_free (void);

void
ddb_logger_stop_buffering (void);

void
ddb_log_detailed (DB_plugin_t *plugin, uint32_t layers, const char *fmt, ...);

void
ddb_vlog_detailed (DB_plugin_t *plugin, uint32_t layers, const char *fmt, va_list ap);

void
ddb_log (const char *fmt, ...);

void
ddb_vlog (const char *fmt, va_list ap);

void
ddb_log_viewer_register (void (*callback)(DB_plugin_t *plugin, uint32_t layers, const char *text, void *ctx), void *ctx);

void
ddb_log_viewer_unregister (void (*callback)(DB_plugin_t *plugin, uint32_t layers, const char *text, void *ctx), void *ctx);

#ifdef __cplusplus
}
#endif

#endif /* logger_h */
