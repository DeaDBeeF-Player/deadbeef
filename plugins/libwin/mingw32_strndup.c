
#include <malloc.h>
#include <string.h>

char *strndup(char *buff, size_t bufflen)
{
    char *result = NULL;

    if (strlen(buff)>bufflen)
    {
        if ((result=malloc(1+bufflen)) != NULL)
        {
            memcpy(result, buff, bufflen);
            result[bufflen] = 0;
        }
    }
    else
    {
        result = strdup(buff);
    }

    return result;
}
