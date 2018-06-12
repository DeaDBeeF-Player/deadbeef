//
//#ifdef 
//#undef rename
//extern int rename(const char *, const char *);
//#define rename(X,Y) rename_windows(X,Y)
#include <stdio.h>
#include <wchar.h>
#include <windows.h>
int rename_windows(const char * oldfile_c, const char * newfile_c){
    char oldfile[strlen(oldfile_c)+1];
    strcpy(oldfile, oldfile_c);
    // convert any '/' characters to '\\'
    {
        char * slash = oldfile;
        while (slash = strchr(slash, '/'))
            *slash = '\\';
    }
    char newfile[strlen(newfile_c)+1];
    strcpy(newfile, newfile_c);
    // convert any '/' characters to '\\'
    {
        char * slash = newfile;
        while (slash = strchr(slash, '/'))
            *slash = '\\';
    }

    wchar_t oldfile_w[strlen(oldfile_c) * 2 + 2];
    int iconv_ret = junk_iconv2 (oldfile_c, strlen(oldfile_c) + 1, (char *) oldfile_w, strlen(oldfile_c) * 2 + 2, "UTF-8", "WCHAR_T");
    wchar_t newfile_w[strlen(newfile_c) * 2 + 2];
    int iconv2_ret = junk_iconv2 (newfile, strlen(newfile) + 1, (char *) newfile_w, strlen(newfile_c) * 2 + 2, "UTF-8", "WCHAR_T");

    struct _stat st;
    if (_wstat (newfile_w, &st)) {
        // not exists
        {
            wchar_t dir[wcslen(newfile_w)+1];
            wcscpy (dir, newfile_w);
            wchar_t * last = wcsrchr (dir, '\\');
            if (!last) {
                // fail?
            }
            *last = 0;
            _wmkdir (dir);
        }
        HANDLE hFile = CreateFileW (newfile_w, (GENERIC_READ | GENERIC_WRITE), 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile != INVALID_HANDLE_VALUE) {
            CloseHandle(hFile);
        }
    }
    int ret = ReplaceFileW (newfile_w, oldfile_w, NULL, 0, /*reserved for future use ...*/0,/*xd*/0);
    if (!ret){
        return -1;
    }
    return 0;
}
    // trace("GetFileAttributes returns %d\n",GetFileAttributes(newfile));
    //if( GetFileAttributesA(newfile) != INVALID_FILE_ATTRIBUTES ) {
    /*    if(remove(newfile)){
            fprintf(stdout,"Removing file  %s failed!\n",newfile);
            return -1;
        }
    //}
    if((rename)(oldfile,newfile)){
        fprintf(stdout,"Renaming file %s failed!\n",oldfile);
        return -1;
    }
    return 0;*/
/*}
    //trace("oldfile: %s , newfile: %s\n",oldfile,newfile);
    if(CopyFile(oldfile,newfile,TRUE)){
        trace_err("CopyFile failed: cp %s %s\n",oldfile,newfile);
    }
    if(remove(oldfile)){
        trace_err("Remove file failed: rm %s\n",oldfile);
    	return -1;
    }
    return 0;
}*/
/*   // trace("GetFileAttributes returns %d\n",GetFileAttributes(newfile));
    if( GetFileAttributes(newfile) != INVALID_FILE_ATTRIBUTES ) {
        if(remove(newfile))
            trace("Removing file failed!\n");
    }
    if(CopyFile(oldfile,newfile))
        trace("Renaming file failed!\n");
    return 0;
}*/
