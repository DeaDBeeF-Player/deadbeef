#ifndef _UNIXATOMIC_H_
#define _UNIXATOMIC_H_

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

int atomic_close(int fd);
int atomic_dup2(int oldfd, int newfd);
size_t atomic_fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
ssize_t atomic_read(int fd, const void *buf, size_t count);
void *atomic_read_file(size_t *fs, const char *filename);
ssize_t atomic_write(int fd, const void *buf, size_t count);

#endif
