/*
	xfermem: unidirectional fast pipe

	copyright ?-2008 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Oliver Fromme
	old timestamp: Sun Apr  6 02:26:26 MET DST 1997

	See xfermem.h for documentation/description.
*/


#ifndef NOXFERMEM

#include "mpg123app.h"
#include <string.h>
#include <errno.h>
#include <sys/uio.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <fcntl.h>

#ifndef HAVE_MMAP
#include <sys/ipc.h>
#include <sys/shm.h>
#endif

#include "debug.h"

#if defined (HAVE_MMAP) && defined(MAP_ANONYMOUS) && !defined(MAP_ANON)
#define MAP_ANON MAP_ANONYMOUS
#endif

void xfermem_init (txfermem **xf, size_t bufsize, size_t msize, size_t skipbuf)
{
	size_t regsize = bufsize + msize + skipbuf + sizeof(txfermem);

#ifdef HAVE_MMAP
#  ifdef MAP_ANON
	if ((*xf = (txfermem *) mmap(0, regsize, PROT_READ | PROT_WRITE,
			MAP_ANON | MAP_SHARED, -1, 0)) == (txfermem *) -1) {
		perror ("mmap()");
		exit (1);
	}
#  else
	int devzero;
	if ((devzero = open("/dev/zero", O_RDWR, 0)) == -1) {
		perror ("open(/dev/zero)");
		exit (1);
	}
	if ((*xf = (txfermem *) mmap(0, regsize, PROT_READ | PROT_WRITE,
			MAP_SHARED, devzero, 0)) == (txfermem *) -1) {
		perror ("mmap()");
		exit (1);
	}
	close (devzero);
#  endif
#else
	struct shmid_ds shmemds;
	int shmemid;
	if ((shmemid = shmget(IPC_PRIVATE, regsize, IPC_CREAT | 0600)) == -1) {
		perror ("shmget()");
		exit (1);
	}
	if ((*xf = (txfermem *) shmat(shmemid, 0, 0)) == (txfermem *) -1) {
		perror ("shmat()");
		shmctl (shmemid, IPC_RMID, &shmemds);
		exit (1);
	}
	if (shmctl(shmemid, IPC_RMID, &shmemds) == -1) {
		perror ("shmctl()");
		xfermem_done (*xf);
		exit (1);
	}
#endif
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, (*xf)->fd) < 0) {
		perror ("socketpair()");
		xfermem_done (*xf);
		exit (1);
	}
	(*xf)->freeindex = (*xf)->readindex = 0;
	(*xf)->wakeme[0] = (*xf)->wakeme[1] = FALSE;
	(*xf)->data = ((byte *) *xf) + sizeof(txfermem) + msize;
	(*xf)->metadata = ((byte *) *xf) + sizeof(txfermem);
	(*xf)->size = bufsize;
	(*xf)->metasize = msize + skipbuf;
	(*xf)->justwait = 0;
}

void xfermem_done (txfermem *xf)
{
	if(!xf)
		return;
#ifdef HAVE_MMAP
	munmap ((caddr_t) xf, xf->size + xf->metasize + sizeof(txfermem));
#else
	if (shmdt((void *) xf) == -1) {
		perror ("shmdt()");
		exit (1);
	}
#endif
}

void xfermem_init_writer (txfermem *xf)
{
	if(xf)
		close (xf->fd[XF_READER]);
}

void xfermem_init_reader (txfermem *xf)
{
	if(xf)
		close (xf->fd[XF_WRITER]);
}

size_t xfermem_get_freespace (txfermem *xf)
{
	size_t freeindex, readindex;

	if(!xf)
		return 0;

	if ((freeindex = xf->freeindex) < 0
			|| (readindex = xf->readindex) < 0)
		return (0);
	if (readindex > freeindex)
		return ((readindex - freeindex) - 1);
	else
		return ((xf->size - (freeindex - readindex)) - 1);
}

size_t xfermem_get_usedspace (txfermem *xf)
{
	size_t freeindex, readindex;

	if(!xf)
		return 0;

	if ((freeindex = xf->freeindex) < 0
			|| (readindex = xf->readindex) < 0)
		return (0);
	if (freeindex >= readindex)
		return (freeindex - readindex);
	else
		return (xf->size - (readindex - freeindex));
}

int xfermem_getcmd (int fd, int block)
{
	fd_set selfds;
	byte cmd;

	for (;;) {
		struct timeval selto = {0, 0};

		FD_ZERO (&selfds);
		FD_SET (fd, &selfds);
#ifdef HPUX
		switch (select(FD_SETSIZE, (int *) &selfds, NULL, NULL, block ? NULL : &selto)) {
#else
		switch (select(FD_SETSIZE, &selfds, NULL, NULL, block ? NULL : &selto)) {
#endif
			case 0:
				if (!block)
					return (0);
				continue;
			case -1:
				if (errno == EINTR)
					continue;
				return (-2);
			case 1:
				if (FD_ISSET(fd, &selfds))
					switch (read(fd, &cmd, 1)) {
						case 0: /* EOF */
							return (-1);
						case -1:
							if (errno == EINTR)
								continue;
							return (-3);
						case 1:
							return (cmd);
						default: /* ?!? */
							return (-4);
					}
				else /* ?!? */
					return (-5);
			default: /* ?!? */
				return (-6);
		}
	}
}

int xfermem_putcmd (int fd, byte cmd)
{
	for (;;) {
		switch (write(fd, &cmd, 1)) {
			case 1:
				return (1);
			case -1:
				if (errno != EINTR)
					return (-1);
		}
	}
}

int xfermem_block (int readwrite, txfermem *xf)
{
	int myfd = xf->fd[readwrite];
	int result;

	xf->wakeme[readwrite] = TRUE;
	if (xf->wakeme[1 - readwrite])
		xfermem_putcmd (myfd, XF_CMD_WAKEUP);
	result = xfermem_getcmd(myfd, TRUE);
	xf->wakeme[readwrite] = FALSE;
	return ((result <= 0) ? -1 : result);
}

/* Parallel-safe code to signal a process and wait for it to respond. */
int xfermem_sigblock(int readwrite, txfermem *xf, int pid, int signal)
{
	int myfd = xf->fd[readwrite];
	int result;

	xf->wakeme[readwrite] = TRUE;
	kill(pid, signal);

	/* not sure about that block... here */
	if (xf->wakeme[1 - readwrite])
		xfermem_putcmd (myfd, XF_CMD_WAKEUP);

	result = xfermem_getcmd(myfd, TRUE);
	xf->wakeme[readwrite] = FALSE;

	return ((result <= 0) ? -1 : result);
}

int xfermem_write(txfermem *xf, byte *buffer, size_t bytes)
{
	if(buffer == NULL || bytes < 1) return FALSE;

	/* You weren't so braindead to have not allocated enough space at all, right? */
	while (xfermem_get_freespace(xf) < bytes)
	{
		int cmd =  xfermem_block(XF_WRITER, xf);
		if (cmd == XF_CMD_TERMINATE || cmd < 0)
		{
			error("failed to wait for free space");
			return TRUE; /* Failure. */
		}
	}
	/* Now we have enough space. copy the memory, possibly with the wrap. */
	if(xf->size - xf->freeindex >= bytes)
	{	/* one block of free memory */
		memcpy(xf->data+xf->freeindex, buffer, bytes);
	}
	else
	{ /* two blocks */
		size_t endblock = xf->size - xf->freeindex;
		memcpy(xf->data+xf->freeindex, buffer, endblock);
		memcpy(xf->data, buffer + endblock, bytes-endblock);
	}
	/* Advance the free space pointer, including the wrap. */
	xf->freeindex = (xf->freeindex + bytes) % xf->size;
	/* Wake up the buffer process if necessary. */
	debug("write waking");
	if(xf->wakeme[XF_READER])
	return xfermem_putcmd(xf->fd[XF_WRITER], XF_CMD_WAKEUP_INFO) < 0 ? TRUE : FALSE;

	return FALSE;
}

#else /* stubs for generic / win32 */

#include "mpg123app.h"
#include "xfermem.h"

void xfermem_init (txfermem **xf, size_t bufsize, size_t msize, size_t skipbuf)
{
}
void xfermem_done (txfermem *xf)
{
}
void xfermem_init_writer (txfermem *xf)
{
}
void xfermem_init_reader (txfermem *xf)
{
}
size_t xfermem_get_freespace (txfermem *xf)
{
  return 0;
}
size_t xfermem_get_usedspace (txfermem *xf)
{
  return 0;
}
int xfermem_write(txfermem *xf, byte *buffer, size_t bytes)
{
	return FALSE;
}
int xfermem_getcmd (int fd, int block)
{
  return 0;
}
int xfermem_putcmd (int fd, byte cmd)
{
  return 0;
}
int xfermem_block (int readwrite, txfermem *xf)
{
  return 0;
}
int xfermem_sigblock (int readwrite, txfermem *xf, int pid, int signal)
{
  return 0;
}
#endif

/* eof */

