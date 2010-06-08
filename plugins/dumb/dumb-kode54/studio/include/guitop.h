#ifndef INCLUDED_GUITOP_H
#define INCLUDED_GUITOP_H


#include "dumbdesk.h"

extern int the_time;

extern volatile int true_time;


void run_desktop(GUI_DESKTOP_PARAM *param);

void initialise_guitop(void);


#endif /* INCLUDED_GUITOP_H */
