/*  _______         ____    __         ___    ___
 * \    _  \       \    /  \  /       \   \  /   /       '   '  '
 *  |  | \  \       |  |    ||         |   \/   |         .      .
 *  |  |  |  |      |  |    ||         ||\  /|  |
 *  |  |  |  |      |  |    ||         || \/ |  |         '  '  '
 *  |  |  |  |      |  |    ||         ||    |  |         .      .
 *  |  |_/  /        \  \__//          ||    |  |
 * /_______/ynamic    \____/niversal  /__\  /____\usic   /|  .  . ibliotheque
 *                                                      /  \
 *                                                     / .  \
 * in_duh.h - Winamp plug-in header file.             / / \  \
 *                                                   | <  /   \_
 * By Bob.                                           |  \/ /\   /
 *                                                    \_  /  > /
 *                                                      | \ / /
 *                                                      |  ' /
 *                                                       \__/
 */

#include <windows.h>
//#include <mmreg.h>
//#include <msacm.h>
#include <math.h>

#include "in2.h"

#include "../include/dumb.h"


/******************
 * Plug in config */

#define VERSION "0.1"


#define STREAM_SIZE 576
#define STREAM_FREQ 44100


extern In_Module mod;
