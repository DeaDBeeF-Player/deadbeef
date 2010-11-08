/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */
/* dbus-internals.h  random utility stuff (internal to D-Bus implementation)
 *
 * Copyright (C) 2002, 2003  Red Hat, Inc.
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
#ifdef DBUS_INSIDE_DBUS_H
#error "You can't include dbus-internals.h in the public header dbus.h"
#endif

#ifndef DBUS_INTERNALS_H
#define DBUS_INTERNALS_H

#include <dbus/dbus-memory.h>
#include <dbus/dbus-types.h>
#include <dbus/dbus-errors.h>
#include <dbus/dbus-sysdeps.h>
#include <dbus/dbus-threads-internal.h>

DBUS_BEGIN_DECLS

#ifndef DBUS_SESSION_BUS_DEFAULT_ADDRESS
#define DBUS_SESSION_BUS_DEFAULT_ADDRESS	"autolaunch:"
#endif

void _dbus_warn               (const char *format,
                               ...) _DBUS_GNUC_PRINTF (1, 2);

void _dbus_warn_check_failed  (const char *format,
                               ...) _DBUS_GNUC_PRINTF (1, 2);


#if defined (__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)
#define _DBUS_FUNCTION_NAME __func__
#elif defined(__GNUC__) || defined(_MSC_VER)
#define _DBUS_FUNCTION_NAME __FUNCTION__
#else
#define _DBUS_FUNCTION_NAME "unknown function"
#endif

/*
 * (code from GLib)
 * 
 * The _DBUS_LIKELY and _DBUS_UNLIKELY macros let the programmer give hints to 
 * the compiler about the expected result of an expression. Some compilers
 * can use this information for optimizations.
 *
 * The _DBUS_BOOLEAN_EXPR macro is intended to trigger a gcc warning when
 * putting assignments in the macro arg
 */
#if defined(__GNUC__) && (__GNUC__ > 2) && defined(__OPTIMIZE__)
#define _DBUS_BOOLEAN_EXPR(expr)                \
 __extension__ ({                               \
   int _dbus_boolean_var_;                      \
   if (expr)                                    \
      _dbus_boolean_var_ = 1;                   \
   else                                         \
      _dbus_boolean_var_ = 0;                   \
   _dbus_boolean_var_;                          \
})
#define _DBUS_LIKELY(expr) (__builtin_expect (_DBUS_BOOLEAN_EXPR(expr), 1))
#define _DBUS_UNLIKELY(expr) (__builtin_expect (_DBUS_BOOLEAN_EXPR(expr), 0))
#else
#define _DBUS_LIKELY(expr) (expr)
#define _DBUS_UNLIKELY(expr) (expr)
#endif

#ifdef DBUS_ENABLE_VERBOSE_MODE

/*
 at least gnu cc and msvc compiler are known to 
 have support for variable macro argument lists
 add other compilers is required
*/
#if defined(__GNUC__) || defined(_MSC_VER) 
#define DBUS_CPP_SUPPORTS_VARIABLE_MACRO_ARGUMENTS
#endif

#ifdef DBUS_CPP_SUPPORTS_VARIABLE_MACRO_ARGUMENTS
void _dbus_verbose_real       (const char *file, const int line, const char *function, 
                               const char *format,...) _DBUS_GNUC_PRINTF (4, 5);
#  define _dbus_verbose(fmt,...) _dbus_verbose_real( __FILE__,__LINE__,__FUNCTION__,fmt, ## __VA_ARGS__)
#else
void _dbus_verbose_real       (const char *format,
                               ...) _DBUS_GNUC_PRINTF (1, 2);
#  define _dbus_verbose _dbus_verbose_real
#endif
void _dbus_verbose_reset_real (void);
dbus_bool_t _dbus_is_verbose_real (void);

#  define _dbus_verbose_reset _dbus_verbose_reset_real
#  define _dbus_is_verbose _dbus_is_verbose_real
#else
#  ifdef HAVE_ISO_VARARGS
#    define _dbus_verbose(...)
#  elif defined (HAVE_GNUC_VARARGS)
#    define _dbus_verbose(format...)
#  else
static void _dbus_verbose(const char * x,...) {;}
#  endif
#  define _dbus_verbose_reset()
#  define _dbus_is_verbose() FALSE 
#endif /* !DBUS_ENABLE_VERBOSE_MODE */

const char* _dbus_strerror (int error_number);

#ifdef DBUS_DISABLE_ASSERT
#define _dbus_assert(condition)
#else
void _dbus_real_assert (dbus_bool_t  condition,
                        const char  *condition_text,
                        const char  *file,
                        int          line,
                        const char  *func);
#define _dbus_assert(condition)                                         \
  _dbus_real_assert ((condition) != 0, #condition, __FILE__, __LINE__, _DBUS_FUNCTION_NAME)
#endif /* !DBUS_DISABLE_ASSERT */

#ifdef DBUS_DISABLE_ASSERT
#define _dbus_assert_not_reached(explanation)
#else
void _dbus_real_assert_not_reached (const char *explanation,
                                    const char *file,
                                    int         line) _DBUS_GNUC_NORETURN;
#define _dbus_assert_not_reached(explanation)                                   \
  _dbus_real_assert_not_reached (explanation, __FILE__, __LINE__)
#endif /* !DBUS_DISABLE_ASSERT */

#ifdef DBUS_DISABLE_CHECKS
#define _dbus_return_if_fail(condition)
#define _dbus_return_val_if_fail(condition, val)
#else

extern const char *_dbus_return_if_fail_warning_format;

#define _dbus_return_if_fail(condition) do {                                       \
   _dbus_assert ((*(const char*)_DBUS_FUNCTION_NAME) != '_');                      \
  if (!(condition)) {                                                              \
    _dbus_warn_check_failed (_dbus_return_if_fail_warning_format,                  \
                             _DBUS_FUNCTION_NAME, #condition, __FILE__, __LINE__); \
    return;                                                                        \
  } } while (0)

#define _dbus_return_val_if_fail(condition, val) do {                                   \
   _dbus_assert ((*(const char*)_DBUS_FUNCTION_NAME) != '_');                           \
  if (!(condition)) {                                                                   \
    _dbus_warn_check_failed (_dbus_return_if_fail_warning_format,                       \
                             _DBUS_FUNCTION_NAME, #condition, __FILE__, __LINE__);      \
    return (val);                                                                       \
  } } while (0)

#endif /* !DBUS_DISABLE_ASSERT */

#define _DBUS_N_ELEMENTS(array) ((int) (sizeof ((array)) / sizeof ((array)[0])))

#define _DBUS_POINTER_TO_INT(pointer) ((intptr_t)(pointer))
#define _DBUS_INT_TO_POINTER(integer) ((void*)((intptr_t)(integer)))

#define _DBUS_ZERO(object) (memset (&(object), '\0', sizeof ((object))))

#define _DBUS_STRUCT_OFFSET(struct_type, member)	\
    ((intptr_t) ((unsigned char*) &((struct_type*) 0)->member))

#ifdef DBUS_DISABLE_CHECKS
/* this is an assert and not an error, but in the typical --disable-checks case (you're trying
 * to really minimize code size), disabling these assertions makes sense.
 */
#define _DBUS_ASSERT_ERROR_IS_SET(error)
#define _DBUS_ASSERT_ERROR_IS_CLEAR(error)
#else
#define _DBUS_ASSERT_ERROR_IS_SET(error)   _dbus_assert ((error) == NULL || dbus_error_is_set ((error)))
#define _DBUS_ASSERT_ERROR_IS_CLEAR(error) _dbus_assert ((error) == NULL || !dbus_error_is_set ((error)))
#endif

#define _dbus_return_if_error_is_set(error) _dbus_return_if_fail ((error) == NULL || !dbus_error_is_set ((error)))
#define _dbus_return_val_if_error_is_set(error, val) _dbus_return_val_if_fail ((error) == NULL || !dbus_error_is_set ((error)), (val))

/* This alignment thing is from ORBit2 */
/* Align a value upward to a boundary, expressed as a number of bytes.
 * E.g. align to an 8-byte boundary with argument of 8.
 */

/*
 *   (this + boundary - 1)
 *          &
 *    ~(boundary - 1)
 */

#define _DBUS_ALIGN_VALUE(this, boundary) \
  (( ((uintptr_t)(this)) + (((uintptr_t)(boundary)) -1)) & (~(((uintptr_t)(boundary))-1)))

#define _DBUS_ALIGN_ADDRESS(this, boundary) \
  ((void*)_DBUS_ALIGN_VALUE(this, boundary))


char*       _dbus_strdup                (const char  *str);
void*       _dbus_memdup                (const void  *mem,
                                         size_t       n_bytes);
dbus_bool_t _dbus_string_array_contains (const char **array,
                                         const char  *str);
char**      _dbus_dup_string_array      (const char **array);

#define _DBUS_INT16_MIN	 ((dbus_int16_t) 0x8000)
#define _DBUS_INT16_MAX	 ((dbus_int16_t) 0x7fff)
#define _DBUS_UINT16_MAX ((dbus_uint16_t)0xffff)
#define _DBUS_INT32_MIN	 ((dbus_int32_t) 0x80000000)
#define _DBUS_INT32_MAX	 ((dbus_int32_t) 0x7fffffff)
#define _DBUS_UINT32_MAX ((dbus_uint32_t)0xffffffff)
/* using 32-bit here is sort of bogus */
#define _DBUS_INT_MIN	 _DBUS_INT32_MIN
#define _DBUS_INT_MAX	 _DBUS_INT32_MAX
#define _DBUS_UINT_MAX	 _DBUS_UINT32_MAX
#ifdef DBUS_HAVE_INT64
#define _DBUS_INT64_MAX	 DBUS_INT64_CONSTANT  (0x7fffffffffffffff)
#define _DBUS_UINT64_MAX DBUS_UINT64_CONSTANT (0xffffffffffffffff)
#endif
#define _DBUS_ONE_KILOBYTE 1024
#define _DBUS_ONE_MEGABYTE 1024 * _DBUS_ONE_KILOBYTE
#define _DBUS_ONE_HOUR_IN_MILLISECONDS (1000 * 60 * 60)
#define _DBUS_USEC_PER_SECOND          (1000000)

#undef	MAX
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))

#undef	MIN
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))

#undef	ABS
#define ABS(a)	   (((a) < 0) ? -(a) : (a))

#define _DBUS_ISASCII(c) ((c) != '\0' && (((c) & ~0x7f) == 0))

typedef void (* DBusForeachFunction) (void *element,
                                      void *data);

dbus_bool_t _dbus_set_fd_nonblocking (int             fd,
                                      DBusError      *error);

void _dbus_verbose_bytes           (const unsigned char *data,
                                    int                  len,
                                    int                  offset);
void _dbus_verbose_bytes_of_string (const DBusString    *str,
                                    int                  start,
                                    int                  len);

const char* _dbus_header_field_to_string (int header_field);

extern const char *_dbus_no_memory_message;
#define _DBUS_SET_OOM(error) dbus_set_error_const ((error), DBUS_ERROR_NO_MEMORY, _dbus_no_memory_message)

#ifdef DBUS_BUILD_TESTS
/* Memory debugging */
void        _dbus_set_fail_alloc_counter        (int  until_next_fail);
int         _dbus_get_fail_alloc_counter        (void);
void        _dbus_set_fail_alloc_failures       (int  failures_per_failure);
int         _dbus_get_fail_alloc_failures       (void);
dbus_bool_t _dbus_decrement_fail_alloc_counter  (void);
dbus_bool_t _dbus_disable_mem_pools             (void);
int         _dbus_get_malloc_blocks_outstanding (void);

typedef dbus_bool_t (* DBusTestMemoryFunction)  (void *data);
dbus_bool_t _dbus_test_oom_handling (const char             *description,
                                     DBusTestMemoryFunction  func,
                                     void                   *data);
#else
#define _dbus_set_fail_alloc_counter(n)
#define _dbus_get_fail_alloc_counter _DBUS_INT_MAX

/* These are constant expressions so that blocks
 * they protect should be optimized away
 */
#define _dbus_decrement_fail_alloc_counter() (FALSE)
#define _dbus_disable_mem_pools()            (FALSE)
#define _dbus_get_malloc_blocks_outstanding  (0)
#endif /* !DBUS_BUILD_TESTS */

typedef void (* DBusShutdownFunction) (void *data);
dbus_bool_t _dbus_register_shutdown_func (DBusShutdownFunction  function,
                                          void                 *data);

extern int _dbus_current_generation;

/* Thread initializers */
#define _DBUS_LOCK_NAME(name)           _dbus_lock_##name
#define _DBUS_DECLARE_GLOBAL_LOCK(name) extern DBusMutex  *_dbus_lock_##name
#define _DBUS_DEFINE_GLOBAL_LOCK(name)  DBusMutex         *_dbus_lock_##name  
#define _DBUS_LOCK(name)                _dbus_mutex_lock   (_dbus_lock_##name)
#define _DBUS_UNLOCK(name)              _dbus_mutex_unlock (_dbus_lock_##name)

/* 1-5 */
_DBUS_DECLARE_GLOBAL_LOCK (list);
_DBUS_DECLARE_GLOBAL_LOCK (connection_slots);
_DBUS_DECLARE_GLOBAL_LOCK (pending_call_slots);
_DBUS_DECLARE_GLOBAL_LOCK (server_slots);
_DBUS_DECLARE_GLOBAL_LOCK (message_slots);
/* 5-10 */
_DBUS_DECLARE_GLOBAL_LOCK (bus);
_DBUS_DECLARE_GLOBAL_LOCK (bus_datas);
_DBUS_DECLARE_GLOBAL_LOCK (shutdown_funcs);
_DBUS_DECLARE_GLOBAL_LOCK (system_users);
_DBUS_DECLARE_GLOBAL_LOCK (message_cache);
/* 10-14 */
_DBUS_DECLARE_GLOBAL_LOCK (shared_connections);
_DBUS_DECLARE_GLOBAL_LOCK (win_fds);
_DBUS_DECLARE_GLOBAL_LOCK (sid_atom_cache);
_DBUS_DECLARE_GLOBAL_LOCK (machine_uuid);

#if !DBUS_USE_SYNC
_DBUS_DECLARE_GLOBAL_LOCK (atomic);
#define _DBUS_N_GLOBAL_LOCKS (15)
#else
#define _DBUS_N_GLOBAL_LOCKS (14)
#endif

dbus_bool_t _dbus_threads_init_debug (void);

dbus_bool_t   _dbus_address_append_escaped (DBusString       *escaped,
                                            const DBusString *unescaped);

void          _dbus_set_bad_address        (DBusError         *error,
                                            const char        *address_problem_type,
                                            const char        *address_problem_field,
                                            const char        *address_problem_other);

#define DBUS_UUID_LENGTH_BYTES 16
#define DBUS_UUID_LENGTH_WORDS (DBUS_UUID_LENGTH_BYTES / 4)
#define DBUS_UUID_LENGTH_HEX   (DBUS_UUID_LENGTH_BYTES * 2)

/**
 * A globally unique ID ; we have one for each DBusServer, and also one for each
 * machine with libdbus installed on it.
 */
union DBusGUID
{
  dbus_uint32_t as_uint32s[DBUS_UUID_LENGTH_WORDS];     /**< guid as four uint32 values */
  char as_bytes[DBUS_UUID_LENGTH_BYTES];                /**< guid as 16 single-byte values */
};

void        _dbus_generate_uuid  (DBusGUID         *uuid);
dbus_bool_t _dbus_uuid_encode    (const DBusGUID   *uuid,
                                  DBusString       *encoded);
dbus_bool_t _dbus_read_uuid_file (const DBusString *filename,
                                  DBusGUID         *uuid,
                                  dbus_bool_t       create_if_not_found,
                                  DBusError        *error);

dbus_bool_t _dbus_get_local_machine_uuid_encoded (DBusString *uuid_str);

DBUS_END_DECLS

#endif /* DBUS_INTERNALS_H */
