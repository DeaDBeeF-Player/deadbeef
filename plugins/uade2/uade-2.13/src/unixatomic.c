#include <errno.h>
#include <stdint.h>
#include <assert.h>

#include "unixatomic.h"
#include "sysincludes.h"

int atomic_close(int fd)
{
  while (1) {
    if (close(fd) < 0) {
      if (errno == EINTR)
	continue;
      return -1;
    }
    break;
  }
  return 0;
}


int atomic_dup2(int oldfd, int newfd)
{
  while (1) {
    if (dup2(oldfd, newfd) < 0) {
      if (errno == EINTR)
	continue;
      return -1;
    }
    break;
  }
  return newfd;
}


size_t atomic_fread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
  uint8_t *dest = ptr;
  size_t readmembers = 0;
  size_t ret;

  while (readmembers < nmemb) {
    ret = fread(dest + size * readmembers, size, nmemb - readmembers, stream);
    if (ret == 0)
      break;
    readmembers += ret;
  }

  assert(readmembers <= nmemb);

  return readmembers;
}


ssize_t atomic_read(int fd, const void *buf, size_t count)
{
  char *b = (char *) buf;
  ssize_t bytes_read = 0;
  ssize_t ret;
  while (bytes_read < count) {
    ret = read(fd, &b[bytes_read], count - bytes_read);
    if (ret < 0) {
      if (errno == EINTR)
        continue;
      if (errno == EAGAIN) {
	fd_set s;
	FD_ZERO(&s);
	FD_SET(fd, &s);
	if (select(fd + 1, &s, NULL, NULL, NULL) == 0)
	  fprintf(stderr, "atomic_read: very strange. infinite select() returned 0. report this!\n");
	continue;
      }
      return -1;
    } else if (ret == 0) {
      return 0;
    }
    bytes_read += ret;
  }
  return bytes_read;
}


void *atomic_read_file(size_t *fs, const char *filename)
{
  FILE *f;
  size_t off;
  void *mem = NULL;
  size_t msize;
  long pos;

  if ((f = fopen(filename, "rb")) == NULL)
    goto error;

  if (fseek(f, 0, SEEK_END))
    goto error;
  pos = ftell(f);
  if (pos < 0)
    goto error;
  if (fseek(f, 0, SEEK_SET))
    goto error;

  *fs = pos;
  msize = (pos > 0) ? pos : 1;

  if ((mem = malloc(msize)) == NULL)
    goto error;

  off = atomic_fread(mem, 1, *fs, f);
  if (off < *fs) {
    fprintf(stderr, "Not able to read the whole file %s\n", filename);
    goto error;
  }

  fclose(f);
  return mem;

 error:
  if (f)
    fclose(f);
  free(mem);
  *fs = 0;
  return NULL;
}


ssize_t atomic_write(int fd, const void *buf, size_t count)
{
  char *b = (char *) buf;
  ssize_t bytes_written = 0;
  ssize_t ret;
  while (bytes_written < count) {
    ret = write(fd, &b[bytes_written], count - bytes_written);
    if (ret < 0) {
      if (errno == EINTR)
        continue;
      if (errno == EAGAIN) {
	fd_set s;
	FD_ZERO(&s);
	FD_SET(fd, &s);
	if (select(fd + 1, NULL, &s, NULL, NULL) == 0)
	  fprintf(stderr, "atomic_write: very strange. infinite select() returned 0. report this!\n");
	continue;
      }
      return -1;
    }
    bytes_written += ret;
  }
  return bytes_written;
}
