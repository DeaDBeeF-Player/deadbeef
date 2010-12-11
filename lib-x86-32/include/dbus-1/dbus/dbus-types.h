/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */
/* dbus-types.h  types such as dbus_bool_t
 *
 * Copyright (C) 2002  Red Hat Inc.
 *
 * Licensed under the Academic Free License version 2.1
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */
#if !defined (DBUS_INSIDE_DBUS_H) && !defined (DBUS_COMPILATION)
#error "Only <dbus/dbus.h> can be included directly, this file may disappear or change contents."
#endif

#ifndef DBUS_TYPES_H
#define DBUS_TYPES_H

#include <stddef.h>
#include <dbus/dbus-arch-deps.h>

typedef dbus_uint32_t  dbus_unichar_t;
/* boolean size must be fixed at 4 bytes due to wire protocol! */
typedef dbus_uint32_t  dbus_bool_t;

/* Normally docs are in .c files, but there isn't a .c file for this. */
/**
 * @defgroup DBusTypes Basic types
 * @ingroup  DBus
 * @brief dbus_bool_t, dbus_int32_t, etc.
 *
 * Typedefs for common primitive types.
 *
 * @{
 */

/**
 * @typedef dbus_bool_t
 *
 * A boolean, valid values are #TRUE and #FALSE.
 */

/**
 * @typedef dbus_uint32_t
 *
 * A 32-bit unsigned integer on all platforms.
 */

/**
 * @typedef dbus_int32_t
 *
 * A 32-bit signed integer on all platforms.
 */

/**
 * @typedef dbus_uint16_t
 *
 * A 16-bit unsigned integer on all platforms.
 */

/**
 * @typedef dbus_int16_t
 *
 * A 16-bit signed integer on all platforms.
 */


/**
 * @typedef dbus_uint64_t
 *
 * A 64-bit unsigned integer on all platforms that support it.
 * If supported, #DBUS_HAVE_INT64 will be defined.
 *
 * C99 requires a 64-bit type and most likely all interesting
 * compilers support one. GLib for example flat-out requires
 * a 64-bit type.
 *
 * You probably want to just assume #DBUS_HAVE_INT64 is always defined.
 */

/**
 * @typedef dbus_int64_t
 *
 * A 64-bit signed integer on all platforms that support it.
 * If supported, #DBUS_HAVE_INT64 will be defined.
 *
 * C99 requires a 64-bit type and most likely all interesting
 * compilers support one. GLib for example flat-out requires
 * a 64-bit type.
 * 
 * You probably want to just assume #DBUS_HAVE_INT64 is always defined.
 */

/**
 * @def DBUS_HAVE_INT64
 *
 * Defined if 64-bit integers are available. Will be defined
 * on any platform you care about, unless you care about
 * some truly ancient UNIX, or some bizarre embedded platform.
 *
 * C99 requires a 64-bit type and most likely all interesting
 * compilers support one. GLib for example flat-out requires
 * a 64-bit type.
 *
 * You should feel comfortable ignoring this macro and just using
 * int64 unconditionally.
 * 
 */

/**
 * @def DBUS_INT64_CONSTANT
 *
 * Declare a 64-bit signed integer constant. The macro
 * adds the necessary "LL" or whatever after the integer,
 * giving a literal such as "325145246765LL"
 */

/**
 * @def DBUS_UINT64_CONSTANT
 *
 * Declare a 64-bit unsigned integer constant. The macro
 * adds the necessary "ULL" or whatever after the integer,
 * giving a literal such as "325145246765ULL"
 */

/** @} */

#endif /* DBUS_TYPES_H */
