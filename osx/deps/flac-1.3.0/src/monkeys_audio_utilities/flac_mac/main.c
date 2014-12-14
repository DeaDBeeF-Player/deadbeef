/* flac_mac - wedge utility to add FLAC support to Monkey's Audio
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

/*
 * This program can be used to allow FLAC to masquerade as one of the other
 * supported lossless codecs in Monkey's Audio.  See the documentation for
 * how to do this.
 */

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<wtypes.h>
#include<process.h>
#include<winbase.h>

#include "share/compat.h"
#include "share/safe_str.h"

static int execit(char *prog, char *args);
static int forkit(char *prog, char *args);

int main(int argc, char *argv[])
{
	int flac_return_val = 0, opt_arg = 1, from_arg = -1, to_arg = -1, flac_level = 5, i;
	char prog[MAX_PATH], cmdline[MAX_PATH*2], from[MAX_PATH], to[MAX_PATH], macdir[MAX_PATH], options[256], *p;
	enum { WAVPACK, RKAU, SHORTEN } codec;

	/* get the directory where MAC external codecs reside */
	if(0 != (p = strrchr(argv[0],'\\'))) {
		safe_strncpy(macdir, argv[0], sizeof(macdir));
		*(strrchr(macdir,'\\')+1) = '\0';
	}
	else {
		safe_strncpy(macdir, "", sizeof(macdir));
	}

	/* determine which codec we were called as and parse the options */
	if(p == 0)
		p = argv[0];
	else
		p++;
	if(0 == strnicmp(p, "short", 5)) {
		codec = SHORTEN;
	}
	else if(0 == strnicmp(p, "rkau", 4)) {
		codec = RKAU;
		if(argv[1][0] == '-' && argv[1][1] == 'l') {
			opt_arg = 2;
			switch(argv[1][2]) {
				case '1': flac_level = 1; break;
				case '2': flac_level = 5; break;
				case '3': flac_level = 8; break;
			}
		}
	}
	else if(0 == strnicmp(p, "wavpack", 7)) {
		codec = WAVPACK;
		if(argv[1][0] == '-') {
			opt_arg = 2;
			switch(argv[1][1]) {
				case 'f': flac_level = 1; break;
				case 'h': flac_level = 8; break;
				default: opt_arg = 1;
			}
		}
	}
	else {
		return -5;
	}

	/* figure out which arguments are the source and destination files */
	for(i = 1; i < argc; i++)
		if(argv[i][0] != '-') {
			from_arg = i++;
			break;
		}
	for( ; i < argc; i++)
		if(argv[i][0] != '-') {
			to_arg = i++;
			break;
		}
	if(to_arg < 0)
		return -4;

	/* build the command to call flac with */
	flac_snprintf(prog, sizeof (prog), "%sflac.exe", macdir);
	flac_snprintf(options, sizeof (options), "-%d", flac_level);
	for(i = opt_arg; i < argc; i++)
		if(argv[i][0] == '-') {
			safe_strncat(options, " ", sizeof(options));
			safe_strncat(options, argv[i], sizeof(options));
		}
	flac_snprintf(cmdline, sizeof (cmdline), "\"%s\" %s -o \"%s\" \"%s\"", prog, options, argv[to_arg], argv[from_arg]);

	flac_return_val = execit(prog, cmdline);

	/*
	 * Now that flac has finished, we need to fork a process that will rename
	 * the resulting file with the correct extension once MAC has moved it to
	 * it's final resting place.
	 */
	if(0 == flac_return_val) {
		char *cptr;
		/* get the destination directory, if any */
		if(0 != (p = strchr(argv[to_arg],'\\'))) {
			safe_strncpy(from, argv[to_arg], sizeof(from));
			*(strrchr(from,'\\')+1) = '\0';
		}
		else {
			safe_strncpy(from, "", sizeof(from));
		}

		/* for the full 'from' and 'to' paths for the renamer process */
		p = strrchr(argv[from_arg],'\\');
		safe_strncat(from, p? p+1 : argv[from_arg], sizeof(from));
		safe_strncpy(to, from, sizeof(to));

		cptr = strrchr(from,'.');
		if(cptr == NULL)
			return -3;
		cptr [0] = 0;

		switch(codec) {
			case SHORTEN:
				safe_strncat(from, ".shn", sizeof (from));
				break;
			case WAVPACK:
				safe_strncat(from, ".wv", sizeof (from));
				break;
			case RKAU:
				safe_strncpy(from, ".rka", sizeof (from));
				break;
			default:
				return -4;
		}

		cptr = strrchr(to,'.');
		if(cptr == NULL)
			return -3;
		cptr [0] = 0;

		safe_strncat(to, ".flac", sizeof(to));

		flac_snprintf(prog, sizeof (prog), "%sflac_ren.exe", macdir);
		flac_snprintf(cmdline, sizeof (cmdline), "\"%s\" \"%s\" \"%s\"", prog, from, to);

		flac_return_val = forkit(prog, cmdline);
	}

	return flac_return_val;
}

int execit(char *prog, char *args)
{
	BOOL ok;
	STARTUPINFO startup_info;
	PROCESS_INFORMATION proc_info;

	GetStartupInfo(&startup_info);

	ok = CreateProcess(
		prog,
		args,
		0, /*process security attributes*/
		0, /*thread security attributes*/
		FALSE,
		0, /*dwCreationFlags*/
		0, /*environment*/
		0, /*lpCurrentDirectory*/
		&startup_info,
		&proc_info
	);
	if(ok) {
		DWORD dw;
		dw = WaitForSingleObject(proc_info.hProcess, INFINITE);
		ok = (dw != 0xFFFFFFFF);
		CloseHandle(proc_info.hThread);
		CloseHandle(proc_info.hProcess);
	}

	return ok? 0 : -1;
}

int forkit(char *prog, char *args)
{
	BOOL ok;
	STARTUPINFO startup_info;
	PROCESS_INFORMATION proc_info;

	GetStartupInfo(&startup_info);

	ok = CreateProcess(
		prog,
		args,
		0, /*process security attributes*/
		0, /*thread security attributes*/
		FALSE,
		DETACHED_PROCESS, /*dwCreationFlags*/
		0, /*environment*/
		0, /*lpCurrentDirectory*/
		&startup_info,
		&proc_info
	);
	if(ok) {
		CloseHandle(proc_info.hThread);
		CloseHandle(proc_info.hProcess);
	}

	return ok? 0 : -2;
}
