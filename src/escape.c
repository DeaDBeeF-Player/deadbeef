/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 1998 - 2010, Daniel Stenberg, <daniel@haxx.se>, et al.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution. The terms
 * are also available at http://curl.haxx.se/docs/copyright.html.
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************************************/

/* Escape and unescape URL encoding in strings. The functions return a new
 * allocated string or NULL if an error occurred.  */

#include <ctype.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Portable character check (remember EBCDIC). Do not use isalnum() because
   its behavior is altered by the current locale.
   See http://tools.ietf.org/html/rfc3986#section-2.3
*/
static int
Curl_isunreserved (unsigned char in) {
    switch (in) {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    case 'a':
    case 'b':
    case 'c':
    case 'd':
    case 'e':
    case 'f':
    case 'g':
    case 'h':
    case 'i':
    case 'j':
    case 'k':
    case 'l':
    case 'm':
    case 'n':
    case 'o':
    case 'p':
    case 'q':
    case 'r':
    case 's':
    case 't':
    case 'u':
    case 'v':
    case 'w':
    case 'x':
    case 'y':
    case 'z':
    case 'A':
    case 'B':
    case 'C':
    case 'D':
    case 'E':
    case 'F':
    case 'G':
    case 'H':
    case 'I':
    case 'J':
    case 'K':
    case 'L':
    case 'M':
    case 'N':
    case 'O':
    case 'P':
    case 'Q':
    case 'R':
    case 'S':
    case 'T':
    case 'U':
    case 'V':
    case 'W':
    case 'X':
    case 'Y':
    case 'Z':
    case '-':
    case '.':
    case '_':
    case '~':
        return 1;
    default:
        break;
    }
    return 0;
}

char *
uri_escape (const char *string, int inlength) {
    size_t alloc = (inlength ? (size_t)inlength : strlen (string)) + 1;
    char *ns;
    char *testing_ptr = NULL;
    unsigned char in; /* we need to treat the characters unsigned */
    size_t newlen = alloc;
    int strindex = 0;
    size_t length;

    ns = malloc (alloc);
    if (!ns)
        return NULL;

    length = alloc - 1;
    while (length--) {
        in = *string;

        if (Curl_isunreserved (in)) {
            /* just copy this */
            ns[strindex++] = in;
        }
        else {
            /* encode it */
            newlen += 2; /* the size grows with two, since this'll become a %XX */
            if (newlen > alloc) {
                alloc *= 2;
                testing_ptr = realloc (ns, alloc);
                if (!testing_ptr) {
                    free (ns);
                    return NULL;
                }
                else {
                    ns = testing_ptr;
                }
            }

            snprintf (&ns[strindex], 4, "%%%02X", in);

            strindex += 3;
        }
        string++;
    }
    ns[strindex] = 0; /* terminate it */
    return ns;
}

#define ISXDIGIT(x) (isxdigit ((int)((unsigned char)x)))
#define CURL_MASK_UCHAR 0xFF

/*
** unsigned long to unsigned char
*/

unsigned char
curlx_ultouc (unsigned long ulnum) {
#ifdef __INTEL_COMPILER
#    pragma warning(push)
#    pragma warning(disable : 810) /* conversion may lose significant bits */
#endif

    return (unsigned char)(ulnum & (unsigned long)CURL_MASK_UCHAR);

#ifdef __INTEL_COMPILER
#    pragma warning(pop)
#endif
}

char *
uri_unescape (const char *string, int length) {
    int alloc = (length ? length : (int)strlen (string)) + 1;
    char *ns = malloc (alloc);
    unsigned char in;
    int strindex = 0;
    unsigned long hex;

    if (!ns)
        return NULL;

    while (--alloc > 0) {
        in = *string;
        if (('%' == in) && ISXDIGIT (string[1]) && ISXDIGIT (string[2])) {
            /* this is two hexadecimal digits following a '%' */
            char hexstr[3];
            char *ptr;
            hexstr[0] = string[1];
            hexstr[1] = string[2];
            hexstr[2] = 0;

            hex = strtoul (hexstr, &ptr, 16);

            in = curlx_ultouc (hex); /* this long is never bigger than 255 anyway */

#ifdef CURL_DOES_CONVERSIONS
            /* escape sequences are always in ASCII so convert them on non-ASCII hosts */
            if (!handle || (Curl_convert_from_network (handle, &in, 1) != CURLE_OK)) {
                /* Curl_convert_from_network calls failf if unsuccessful */
                free (ns);
                return NULL;
            }
#endif /* CURL_DOES_CONVERSIONS */

            string += 2;
            alloc -= 2;
        }

        ns[strindex++] = in;
        string++;
    }
    ns[strindex] = 0; /* terminate it */

    return ns;
}
