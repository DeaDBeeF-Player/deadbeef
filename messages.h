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
};

#endif // __MESSAGES_H
