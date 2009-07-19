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
 * packfile.c - Packfile input module.                / / \  \
 *                                                   | <  /   \_
 * By entheh.                                        |  \/ /\   /
 *                                                    \_  /  > /
 * Note that this does not use file compression;        | \ / /
 * for that you must open the file yourself and         |  ' /
 * then use dumbfile_open_packfile().                    \__/
 */

#include <allegro.h>

#include "aldumb.h"



static void *dumb_packfile_open(const char *filename)
{
	return pack_fopen(filename, F_READ);
}



static int dumb_packfile_skip(void *f, long n)
{
	return pack_fseek(f, n);
}



static int dumb_packfile_getc(void *f)
{
	return pack_getc(f);
}



static long dumb_packfile_getnc(char *ptr, long n, void *f)
{
	return pack_fread(ptr, n, f);
}



static void dumb_packfile_close(void *f)
{
	pack_fclose(f);
}



static DUMBFILE_SYSTEM packfile_dfs = {
	&dumb_packfile_open,
	&dumb_packfile_skip,
	&dumb_packfile_getc,
	&dumb_packfile_getnc,
	&dumb_packfile_close
};



void dumb_register_packfiles(void)
{
	register_dumbfile_system(&packfile_dfs);
}



static DUMBFILE_SYSTEM packfile_dfs_leave_open = {
	NULL,
	&dumb_packfile_skip,
	&dumb_packfile_getc,
	&dumb_packfile_getnc,
	NULL
};



DUMBFILE *dumbfile_open_packfile(PACKFILE *p)
{
	return dumbfile_open_ex(p, &packfile_dfs_leave_open);
}



DUMBFILE *dumbfile_from_packfile(PACKFILE *p)
{
	return p ? dumbfile_open_ex(p, &packfile_dfs) : NULL;
}
