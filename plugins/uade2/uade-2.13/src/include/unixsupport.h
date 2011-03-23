#ifndef _UADE_UNIXSUPPORT_H_
#define _UADE_UNIXSUPPORT_H_

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>

#include "uadeipc.h"


#define die(fmt, args...) do { fprintf(stderr, "uade: " fmt, ## args); exit(1); } while(0)

#define dieerror(fmt, args...) do { fprintf(stderr, "uade: " fmt ": %s\n", ## args, strerror(errno)); exit(1); } while(0)


char *uade_dirname(char *dst, char *src, size_t maxlen);
FILE *uade_open_amiga_file(char *aname, const char *playerdir);
void uade_portable_initializations(void);
void uade_arch_spawn(struct uade_ipc *ipc, pid_t *uadepid, const char *uadename);

/* These read and write functions MUST read and write the full size_t amount
   if they are able to. */
ssize_t uade_ipc_read(void *f, const void *buf, size_t count);
ssize_t uade_ipc_write(void *f, const void *buf, size_t count);
void *uade_ipc_set_input(const char *input);
void *uade_ipc_set_output(const char *output);

char *windows_to_cygwin_path(const char *path);

#endif
