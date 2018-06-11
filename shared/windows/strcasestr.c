
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


