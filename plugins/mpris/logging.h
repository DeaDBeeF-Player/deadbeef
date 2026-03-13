/*
    MPRIS plugin for DeaDBeeF Player
    Copyright (C) Peter Lamby and other contributors
    See the file COPYING for more details
*/

#ifndef LOGGING_H_
#define LOGGING_H_

#define logDebug(...) { deadbeef->log_detailed (&plugin.plugin, DDB_LOG_LAYER_INFO, __VA_ARGS__); }
#define logError(...) { deadbeef->log_detailed (&plugin.plugin, 0, __VA_ARGS__); }
#define debug(...) logDebug(__VA_ARGS__)
#define error(...) logError(__VA_ARGS__)

#endif
