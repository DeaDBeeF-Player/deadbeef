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
 * datduh.c - Integration with Allegro's              / / \  \
 *            datafiles.                             | <  /   \_
 *                                                   |  \/ /\   /
 * By entheh.                                         \_  /  > /
 *                                                      | \ / /
 *                                                      |  ' /
 *                                                       \__/
 */

#include <allegro.h>

#include "aldumb.h"
#include "internal/aldumb.h"



static void *dat_read_duh(PACKFILE *f, long size)
{
	DUMBFILE *df;
	DUH *duh;

	(void)size;

	df = dumbfile_open_packfile(f);

	if (!df)
		return NULL;

	duh = read_duh(df);

	dumbfile_close(df);

	return duh;
}



/* dumb_register_dat_duh(): tells Allegro about the DUH datafile object. If
 * you intend to load a datafile containing a DUH object, you must call this
 * function first. It is recommended you pass DAT_DUH, but you may have a
 * reason to use a different type (apart from pride, that doesn't count).
 */
void dumb_register_dat_duh(long type)
{
	register_datafile_object(
		type,
		&dat_read_duh,
		&_dat_unload_duh
	);
}
