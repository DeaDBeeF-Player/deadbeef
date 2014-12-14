/* flac_ren - renamer part of utility to add FLAC support to Monkey's Audio
 * Copyright (C) 2000-2009  Josh Coalson
 * Copyright (C) 2011-2013  Xiph.Org Foundation
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include <io.h>
#include <sys/stat.h>
#include <wtypes.h>
#include <winbase.h>

int main(int argc, char *argv[])
{
	struct stat s;

	/* wait till the 'from' file has reached its final destination */
	do {
		Sleep(2000);
	} while(stat(argv[1], &s) < 0);

	/* now rename it */
	return rename(argv[1], argv[2]);
}
