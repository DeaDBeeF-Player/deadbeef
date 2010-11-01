/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */
/* dbus-threads-internal.h  D-Bus thread primitives
 *
 * Copyright (C) 2002, 2005 Red Hat Inc.
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
#ifndef DBUS_THREADS_INTERNAL_H
#define DBUS_THREADS_INTERNAL_H

#include <dbus/dbus-macros.h>
#include <dbus/dbus-types.h>
#include <dbus/dbus-threads.h>

DBUS_BEGIN_DECLS

DBusMutex*   _dbus_mutex_new                 (void);
void         _dbus_mutex_free                (DBusMutex         *mutex);
void         _dbus_mutex_lock                (DBusMutex         *mutex);
void         _dbus_mutex_unlock              (DBusMutex         *mutex);
void         _dbus_mutex_new_at_location     (DBusMutex        **location_p);
void         _dbus_mutex_free_at_location    (DBusMutex        **location_p);

DBusCondVar* _dbus_condvar_new               (void);
void         _dbus_condvar_free              (DBusCondVar       *cond);
void         _dbus_condvar_wait              (DBusCondVar       *cond,
                                              DBusMutex         *mutex);
dbus_bool_t  _dbus_condvar_wait_timeout      (DBusCondVar       *cond,
                                              DBusMutex         *mutex,
                                              int                timeout_milliseconds);
void         _dbus_condvar_wake_one          (DBusCondVar       *cond);
void         _dbus_condvar_wake_all          (DBusCondVar       *cond);
void         _dbus_condvar_new_at_location   (DBusCondVar      **location_p);
void         _dbus_condvar_free_at_location  (DBusCondVar      **location_p);

DBUS_END_DECLS

#endif /* DBUS_THREADS_INTERNAL_H */
