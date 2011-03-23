#ifndef _UADE_UNIXWALKDIR_H_
#define _UADE_UNIXWALKDIR_H_

enum uade_wtype {
	UADE_WALK_REGULAR_FILE = 1,
	UADE_WALK_DIRECTORY,
	UADE_WALK_SYMLINK,
	UADE_WALK_SPECIAL
};

void *uade_walk_directories(const char *dirname,
			    void *(*fn) (const char *file,
					 enum uade_wtype wtype, void *opaque),
			    void *opaque);

#endif
