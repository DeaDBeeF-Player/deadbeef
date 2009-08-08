#ifndef __MESSAGES_H
#define __MESSAGES_H

enum {
    M_SONGFINISHED,
    M_NEXTSONG,
    M_PREVSONG,
    M_PLAYSONG,
    M_PLAYSONGNUM,
    M_STOPSONG,
    M_PAUSESONG,
    M_PLAYRANDOM,
    M_SONGSEEK,
    M_SONGCHANGED, // p1=from, p2=to
    M_ADDDIR, // ctx = pointer to string, which must be freed by f_free
    M_ADDFILES, // ctx = GSList pointer, must be freed with g_slist_free
    M_FMDRAGDROP, // ctx = char* ptr, must be freed with standard free, p1 is length of data, p2 is drop_y
};

#endif // __MESSAGES_H
