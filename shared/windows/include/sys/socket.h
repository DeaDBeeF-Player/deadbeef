/* sys/socket.h

This file is part of Cygwin.

This software is a copyrighted work licensed under the terms of the
Cygwin license.  Please consult the file "CYGWIN_LICENSE" for
details. */

#ifndef _SYS_SOCKET_H
#define _SYS_SOCKET_H

#include "features.h"
#include <cygwin/socket.h>
#include <sys/time.h>

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef __INSIDE_CYGWIN_NET__
  int accept (int, struct sockaddr *__peer, socklen_t *);
  int accept4 (int, struct sockaddr *__peer, socklen_t *, int flags);
  int bind (int, const struct sockaddr *__my_addr, socklen_t __addrlen);
  int connect (int, const struct sockaddr *, socklen_t);
  int getpeername (int, struct sockaddr *__peer, socklen_t *);
  int getsockname (int, struct sockaddr *__addr, socklen_t *);
  int listen (int, int __n);
  ssize_t recv (int, void *__buff, size_t __len, int __flags);
  ssize_t recvfrom (int, void *__buff, size_t __len, int __flags,
		    struct sockaddr *__from, socklen_t *__fromlen);
  ssize_t recvmsg(int s, struct msghdr *msg, int flags);
  ssize_t send (int, const void *__buff, size_t __len, int __flags);
  ssize_t sendmsg(int s, const struct msghdr *msg, int flags);
  ssize_t sendto (int, const void *, size_t __len, int __flags,
		  const struct sockaddr *__to, socklen_t __tolen);
  int setsockopt (int __s, int __level, int __optname, const void *optval,
		  socklen_t __optlen);
  int getsockopt (int __s, int __level, int __optname, void *__optval,
		  socklen_t *__optlen);
  int shutdown (int, int);
  int socket (int __family, int __type, int __protocol);
  int sockatmark (int __fd);
  int socketpair (int __domain, int __type, int __protocol, int *__socket_vec);

  struct servent *getservbyname (const char *__name, const char *__proto);
#endif

#ifdef __cplusplus
};
#endif

#endif /* _SYS_SOCKET_H */
