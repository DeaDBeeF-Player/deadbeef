/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2011 Alexey Yakovenko <waker@users.sourceforge.net>

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

    utf8 code is based on Basic UTF-8 manipulation routines
    by Jeff Bezanson
    placed in the public domain Fall 2005
*/
#ifndef __UTF8_H
#define __UTF8_H

#include <stdint.h>
#include <stdarg.h>

/* is c the start of a utf8 sequence? */
#define isutf(c) (((c)&0xC0)!=0x80)

/* convert UTF-8 data to wide character */
int u8_toucs(uint32_t *dest, int32_t sz, const char *src, int32_t srcsz);

/* the opposite conversion */
int u8_toutf8(char *dest, int32_t sz, uint32_t *src, int32_t srcsz);

/* single character to UTF-8 */
int u8_wc_toutf8(char *dest, wchar_t ch);

/* character number to byte offset */
int u8_offset(char *str, int32_t charnum);

/* byte offset to character number */
int u8_charnum(char *s, int32_t offset);

/* return next character, updating an index variable */
uint32_t u8_nextchar(const char *s, int32_t *i);

/* move to next character */
void u8_inc(const char *s, int32_t *i);

/* move to previous character */
void u8_dec(const char *s, int32_t *i);

/* assuming src points to the character after a backslash, read an
   escape sequence, storing the result in dest and returning the number of
   input characters processed */
int u8_read_escape_sequence(const char *src, uint32_t *dest);

/* given a wide character, convert it to an ASCII escape sequence stored in
   buf, where buf is "sz" bytes. returns the number of characters output. */
int u8_escape_wchar(char *buf, int32_t sz, uint32_t ch);

/* convert a string "src" containing escape sequences to UTF-8 */
int u8_unescape(char *buf, int32_t sz, const char *src);

/* convert UTF-8 "src" to ASCII with escape sequences.
   if escape_quotes is nonzero, quote characters will be preceded by
   backslashes as well. */
int u8_escape(char *buf, int32_t sz, const char *src, int32_t escape_quotes);

/* utility predicates used by the above */
int octal_digit(char c);
int hex_digit(char c);

/* return a pointer to the first occurrence of ch in s, or NULL if not
   found. character index of found character returned in *charn. */
char *u8_strchr(char *s, uint32_t ch, int32_t *charn);

/* same as the above, but searches a buffer of a given size instead of
   a NUL-terminated string. */
char *u8_memchr(char *s, uint32_t ch, size_t sz, int32_t *charn);

/* count the number of characters in a UTF-8 string */
int u8_strlen(char *s);

int u8_is_locale_utf8(char *locale);

/* printf where the format string and arguments may be in UTF-8.
   you can avoid this function and just use ordinary printf() if the current
   locale is UTF-8. */
int u8_vprintf(char *fmt, va_list ap);
int u8_printf(char *fmt, ...);

// validate utf8 string
// returns 1 if valid, 0 otherwise
int u8_valid (const char  *str,
        int max_len,
        const char **end);

int
u8_tolower (const signed char *c, int l, char *out);

int
u8_strcasecmp (const char *a, const char *b);

const char *
utfcasestr (const char *s1, const char *s2);

#endif
