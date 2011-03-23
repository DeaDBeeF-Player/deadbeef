#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include <unixwalkdir.h>

void *uade_walk_directories(const char *dirname,
			    void *(*fn) (const char *file,
					 enum uade_wtype wtype, void *opaque),
			    void *opaque)
{
	char *dename;
	DIR *dir;
	size_t namelen;
	struct dirent *de;
	void *ret = NULL;
	struct stat st;
	enum uade_wtype wtype;

	namelen = strlen(dirname) + 256 + 2;
	if ((dename = malloc(namelen)) == NULL)
		return NULL;

	if ((dir = opendir(dirname)) == NULL)
		return NULL;

	while ((de = readdir(dir)) != NULL) {

		if (strcmp(de->d_name, ".") == 0
		    || strcmp(de->d_name, "..") == 0)
			continue;

		if (snprintf(dename, namelen, "%s/%s", dirname, de->d_name) >=
		    namelen) {
			fprintf(stderr, "interesting: too long a filename\n");
			continue;
		}

		if (lstat(dename, &st))
			continue;

		if (S_ISREG(st.st_mode))
			wtype = UADE_WALK_REGULAR_FILE;
		else if (S_ISDIR(st.st_mode))
			wtype = UADE_WALK_DIRECTORY;
		else if (S_ISLNK(st.st_mode))
			wtype = UADE_WALK_SYMLINK;
		else
			wtype = UADE_WALK_SPECIAL;

		if ((ret = fn(dename, wtype, opaque)) != NULL)
			break;

		if (wtype == UADE_WALK_DIRECTORY) {
			if ((ret =
			     uade_walk_directories(dename, fn, opaque)) != NULL)
				break;
		}
	}

	closedir(dir);
	free(dename);

	return ret;
}
