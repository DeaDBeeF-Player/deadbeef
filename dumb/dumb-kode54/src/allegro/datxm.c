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
 * datxm.c - Integration of XM files with             / / \  \
 *           Allegro's datafiles.                    | <  /   \_
 *                                                   |  \/ /\   /
 * By entheh.                                         \_  /  > /
 *                                                      | \ / /
 *                                                      |  ' /
 *                                                       \__/
 */

#include <allegro.h>

#include "aldumb.h"
#include "internal/aldumb.h"



static void *dat_read_xm(PACKFILE *f, long size)
{
	DUMBFILE *df;
	DUH *duh;

	(void)size;

	df = dumbfile_open_packfile(f);

	if (!df)
		return NULL;

	duh = dumb_read_xm(df);

	dumbfile_close(df);

	return duh;
}



/* dumb_register_dat_xm(): tells Allegro about the XM datafile object. If you
 * intend to load a datafile containing an XM object, you must call this
 * function first. It is recommended you pass DUMB_DAT_XM, but you may have a
 * reason to use a different type (perhaps you already have a datafile with
 * XM files in and they use a different type).
 */
void dumb_register_dat_xm(long type)
{
	register_datafile_object(
		type,
		&dat_read_xm,
		&_dat_unload_duh
	);
}

