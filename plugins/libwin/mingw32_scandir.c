
#include <dirent.h>
#include <stdlib.h>
#include <string.h>

int scandir (const char      *__dir,
             struct dirent ***__namelist,
             int            (*__selector) (const struct dirent *),
             int            (*__cmp) (const struct dirent **, const struct dirent **))
{
    int count = -1; /* this will store the scandir result - will be -1 for errors */
    DIR *curr_dir;
    struct dirent **templist;

    if (__dir != NULL && __namelist != NULL)
    {
        curr_dir = opendir(__dir);
        if (curr_dir != NULL)
        {
            int to_be_added;
            struct dirent *direntry;
            int idx, jdx;
            struct dirent *temp;

            count = 0;

            /* start! */
            direntry = readdir(curr_dir);
            while (direntry != NULL)
            {
                /* an entry was found! */
                if (__selector != NULL)
                {
                    /* '__selector' is a function which returns 1 or 0 */
                    to_be_added = __selector(direntry);
                }
                else
                {
                    to_be_added = 1;
                }

                if (to_be_added)
                {
                    /* this entry has to be added */

                    /* realloc the pointer array */
                    templist = realloc(*__namelist, sizeof(struct dirent *)*(count+1));
                    if (templist == NULL)
                    {
                        /* no room for a new pointer! */
                        /* in this case '__namelist' still holds the pointer to the old untouched array
                           so it is still a consistent result to give back */
                        break;
                    }
                    /* reallocation was succesfull but 'templist' may be different from '*__namelist'
                       in case a relocation (and a copy) was required */
                    *__namelist = templist;

                    /* claim room for the new item */
                    (*__namelist)[count] = (struct dirent *)malloc(sizeof(struct dirent));
                    if ((*__namelist)[count] == NULL)
                    {
                        /* memory allocation failed! */
                        break;
                    }
                    memcpy((*__namelist)[count], direntry, sizeof(struct dirent));
                    /* the entry got added to the array - now let's concern about sorting */
                    if (__cmp != NULL)
                    {
                        /* a sorting routine is provided */
                        if (count != 0)
                        {
                            /* scan the array for the right position */
                            idx = 0;
                            while (__cmp((const struct dirent **)&(*__namelist)[count],
                                         (const struct dirent **)&(*__namelist)[idx]) > 0)
                                idx++;

                            if (idx < count) /* idx == count when the entry is already in its own right position */
                            {
                                /* this happens when the entry is NOT in the right position */
                                /* save the pointer of the new entry structure */
                                temp = (*__namelist)[count];
                                /* shift the tail of the array */
                                for (jdx=count; jdx>idx; jdx--)
                                {
                                    (*__namelist)[jdx] = (*__namelist)[jdx-1];
                                }
                                /* put the new entry in the 'idx' position */
                                (*__namelist)[idx] = temp;
                            }
                        }
                    }
                    count++;
                }
                direntry = readdir(curr_dir);
            }
            closedir(curr_dir);
        }
    }
    return count;
}
