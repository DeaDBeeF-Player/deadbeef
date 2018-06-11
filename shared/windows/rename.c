//
//#ifdef 
//#undef rename
//extern int rename(const char *, const char *);
//#define rename(X,Y) rename_windows(X,Y)
#include <stdio.h>

#include <windows.h>
int rename_windows(const char * oldfile, const char * newfile){

    // trace("GetFileAttributes returns %d\n",GetFileAttributes(newfile));
    if( GetFileAttributes(newfile) != INVALID_FILE_ATTRIBUTES ) {
        if(remove(newfile)){
            fprintf(stdout,"Removing file  %s failed!\n",newfile);
            return -1;
        }
    }
    if((rename)(oldfile,newfile)){
        fprintf(stdout,"Renaming file %s failed!\n",oldfile);
        return -1;
    }
    return 0;
}/*
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