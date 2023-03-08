/*
    Shellexec plugin for DeaDBeeF
    Copyright (C) 2010-2012 Deadbeef team
    Original developer Viktor Semykin <thesame.ml@gmail.com>
    Maintainance, minor improvements Oleksiy Yakovenko <waker@users.sf.net>
    GUI support and bugfixing Azeem Arshad <kr00r4n@gmail.com>

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
#ifndef __SHELLEXEC_H
#define __SHELLEXEC_H

#include <deadbeef/deadbeef.h>

// Version history:
// 1.2: changed configuration to JSON, use new title formatting

#ifdef __cplusplus
extern "C" {
#endif

//Probably it's reasonable to move these flags to parent struct
enum {
    SHX_ACTION_LOCAL_ONLY       = 1 << 0,
    SHX_ACTION_REMOTE_ONLY      = 1 << 1
};

typedef struct Shx_action_s
{
    DB_plugin_action_t parent;

    const char *shcommand;
    uint32_t shx_flags;
} Shx_action_t;

typedef struct Shx_plugin_s
{
    DB_misc_t misc;
    
    void (*save_actions) (void);
    Shx_action_t* (*action_add) (void);
    void (*action_remove) (Shx_action_t *a);
    void (*action_free) (Shx_action_t *a);
} Shx_plugin_t;

#ifdef __cplusplus
}
#endif

#endif
