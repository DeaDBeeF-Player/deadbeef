/*
    shared/windows/strcasestr.c
    Copyright (C) 2016 Elio Blanca

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
#include <string.h>
#include <ctype.h>



char *strcasestr(const char *haystack, const char *needle)
{
    char *result = NULL;
    int needle_len, length;
    int idx, jdx;

    if (haystack != NULL && needle != NULL)
    {
        needle_len = (int)strlen(needle);
        length = strlen(haystack) - needle_len;
        idx = 0;
        while ((result=strchr(&haystack[idx],tolower(needle[0])))!=NULL
               ||
               (result=strchr(&haystack[idx],toupper(needle[0])))!=NULL)
        {
            idx = (int)(result - haystack);
            if (idx > length)
            {
                /* needle cannot fit into remaining characters */
                result = NULL;
                break;
            }
            for (jdx=0; jdx<needle_len; jdx++)
            {
                if (tolower(result[jdx]) != tolower(needle[jdx]))
                {
                    break;
                }
            }
            if (jdx == needle_len)
            {
                /* we found needle! */
                break;
            }
            else
            {
                idx++;
            }
        }
    }

    return result;
}


