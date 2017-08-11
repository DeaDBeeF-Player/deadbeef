/* cygwin/socket.h

This file is part of Cygwin.

This software is a copyrighted work licensed under the terms of the
Cygwin license.  Please consult the file "CYGWIN_LICENSE" for
details. */

#ifndef _CYGWIN_SOCKET_H
#define _CYGWIN_SOCKET_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <sys/types.h>

/* Keep #define socklen_t for backward compatibility. */
#ifndef socklen_t
typedef __socklen_t socklen_t;
#define socklen_t socklen_t
#endif

typedef __sa_family_t sa_family_t;

#ifndef __INSIDE_CYGWIN_NET__
struct sockaddr {
  sa_family_t		sa_family;	/* address family, AF_xxx	*/
  char			sa_data[14];	/* 14 bytes of protocol address	*/
};

/* Definition of sockaddr_storage according to SUSv3. */
#define _SS_MAXSIZE 128			/* Maximum size. */
#define _SS_ALIGNSIZE (sizeof (int64_t))/* Desired alignment. */
#define _SS_PAD1SIZE (_SS_ALIGNSIZE - sizeof (sa_family_t))
#define _SS_PAD2SIZE (_SS_MAXSIZE - (sizeof (sa_family_t) \
		      + _SS_PAD1SIZE + _SS_ALIGNSIZE))

struct sockaddr_storage {
  sa_family_t		ss_family;
  char			_ss_pad1[_SS_PAD1SIZE];
  int64_t		__ss_align;
  char			_ss_pad2[_SS_PAD2SIZE];
};
#endif

#include <asm/socket.h>			/* arch-dependent defines	*/
#include <cygwin/sockios.h>		/* the SIOCxxx I/O controls	*/
#include <sys/uio.h>			/* iovec support		*/

struct ucred {
  pid_t			pid;
  uid_t			uid;
  gid_t			gid;
};

struct linger {
  unsigned short	l_onoff;	/* Linger active	*/
  unsigned short	l_linger;	/* How long to linger for	*/
};

struct msghdr
{
  void *		msg_name;	/* Socket name			*/
  socklen_t		msg_namelen;	/* Length of name		*/
  struct iovec *	msg_iov;	/* Data blocks			*/
  int			msg_iovlen;	/* Number of blocks		*/
  void *		msg_control;	/* Ancillary data		*/
  socklen_t		msg_controllen;	/* Ancillary data buffer length	*/
  int			msg_flags;	/* Received flags on recvmsg	*/
};

struct cmsghdr
{
  /* Amazing but true: The type of cmsg_len should be socklen_t but, just
     as on Linux, the definition of the kernel is incompatible with this,
     so the Windows socket headers define cmsg_len as SIZE_T. */
  size_t		cmsg_len;	/* Length of cmsghdr + data	*/
  int			cmsg_level;	/* Protocol			*/
  int			cmsg_type;	/* Protocol type		*/
};

#define CMSG_ALIGN(len) \
	(((len) + __alignof__ (struct cmsghdr) - 1) \
	 & ~(__alignof__ (struct cmsghdr) - 1))
#define CMSG_LEN(len) \
	(CMSG_ALIGN (sizeof (struct cmsghdr)) + (len))
#define CMSG_SPACE(len) \
	(CMSG_ALIGN (sizeof (struct cmsghdr)) + CMSG_ALIGN(len))
#define CMSG_FIRSTHDR(mhdr)	\
	({ \
	  struct msghdr *_m = (struct msghdr *) mhdr; \
	  (unsigned) (_m)->msg_controllen >= sizeof (struct cmsghdr) \
	  ? (struct cmsghdr *) (_m)->msg_control \
	  : (struct cmsghdr *) NULL; \
	})
#define CMSG_NXTHDR(mhdr,cmsg)	\
	({ \
	  struct msghdr *_m = (struct msghdr *) mhdr; \
	  struct cmsghdr *_c = (struct cmsghdr *) cmsg; \
	  ((char *) _c + CMSG_SPACE (_c->cmsg_len) \
	   > (char *) _m->msg_control + _m->msg_controllen) \
	  ? (struct cmsghdr *) NULL \
	  : (struct cmsghdr *) ((char *) _c + CMSG_ALIGN (_c->cmsg_len)); \
	})
#define CMSG_DATA(cmsg)		\
	((unsigned char *) ((struct cmsghdr *)(cmsg) + 1))

/* "Socket"-level control message types: */
#define	SCM_RIGHTS	0x01		/* access rights (array of int) */

#ifdef __INSIDE_CYGWIN__
/* Definition of struct msghdr up to release 1.5.18 */
struct OLD_msghdr
{
  void *		msg_name;	/* Socket name			*/
  int			msg_namelen;	/* Length of name		*/
  struct iovec *	msg_iov;	/* Data blocks			*/
  int			msg_iovlen;	/* Number of blocks		*/
  void *		msg_accrights;	/* Per protocol magic		*/
  					/* (eg BSD descriptor passing)	*/
  int			msg_accrightslen; /* Length of rights list	*/
};
#endif

/* Socket types. */
#define SOCK_STREAM	1		/* stream (connection) socket	*/
#define SOCK_DGRAM	2		/* datagram (conn.less) socket	*/
#define SOCK_RAW	3		/* raw socket			*/
#define SOCK_RDM	4		/* reliably-delivered message	*/
#define SOCK_SEQPACKET	5		/* sequential packet socket	*/

/* GNU extension flags.  Or them to the type parameter in calls to
   socket(2) to mark socket as nonblocking and/or close-on-exec. */
#define SOCK_NONBLOCK	0x01000000
#define SOCK_CLOEXEC	0x02000000
#ifdef __INSIDE_CYGWIN__
#define _SOCK_FLAG_MASK	0xff000000	/* Bits left for more extensions */
#endif

/* Supported address families. */
/*
 * Address families.
 */
#define AF_UNSPEC       0               /* unspecified */
#define AF_UNIX         1               /* local to host (pipes, portals) */
#define AF_LOCAL        1               /* POSIX name for AF_UNIX */
#define AF_INET         2               /* internetwork: UDP, TCP, etc. */
#define AF_IMPLINK      3               /* arpanet imp addresses */
#define AF_PUP          4               /* pup protocols: e.g. BSP */
#define AF_CHAOS        5               /* mit CHAOS protocols */
#define AF_NS           6               /* XEROX NS protocols */
#define AF_ISO          7               /* ISO protocols */
#define AF_OSI          AF_ISO          /* OSI is ISO */
#define AF_ECMA         8               /* european computer manufacturers */
#define AF_DATAKIT      9               /* datakit protocols */
#define AF_CCITT        10              /* CCITT protocols, X.25 etc */
#define AF_SNA          11              /* IBM SNA */
#define AF_DECnet       12              /* DECnet */
#define AF_DLI          13              /* Direct data link interface */
#define AF_LAT          14              /* LAT */
#define AF_HYLINK       15              /* NSC Hyperchannel */
#define AF_APPLETALK    16              /* AppleTalk */
#define AF_NETBIOS      17              /* NetBios-style addresses */
#define AF_INET6        23              /* IP version 6 */

#define AF_MAX          32
/*
 * Protocol families, same as address families for now.
 */
#define PF_UNSPEC       AF_UNSPEC
#define PF_UNIX         AF_UNIX
#define PF_LOCAL        AF_LOCAL
#define PF_INET         AF_INET
#define PF_IMPLINK      AF_IMPLINK
#define PF_PUP          AF_PUP
#define PF_CHAOS        AF_CHAOS
#define PF_NS           AF_NS
#define PF_ISO          AF_ISO
#define PF_OSI          AF_OSI
#define PF_ECMA         AF_ECMA
#define PF_DATAKIT      AF_DATAKIT
#define PF_CCITT        AF_CCITT
#define PF_SNA          AF_SNA
#define PF_DECnet       AF_DECnet
#define PF_DLI          AF_DLI
#define PF_LAT          AF_LAT
#define PF_HYLINK       AF_HYLINK
#define PF_APPLETALK    AF_APPLETALK
#define PF_NETBIOS      AF_NETBIOS
#define PF_INET6        AF_INET6

#define PF_MAX          AF_MAX

/* Maximum queue length specificable by listen.  */
#define SOMAXCONN       0x7fffffff

/* Flags we can use with send/ and recv. */
#define MSG_OOB         0x1             /* process out-of-band data */
#define MSG_PEEK        0x2             /* peek at incoming message */
#define MSG_DONTROUTE   0x4             /* send without using routing tables */
#define MSG_WAITALL     0x8             /* wait for all requested bytes */
#define MSG_DONTWAIT	0x10		/* selective non-blocking operation */
#define MSG_NOSIGNAL    0x20            /* Don't raise SIGPIPE */
#define MSG_TRUNC       0x0100          /* Normal data truncated */
#define MSG_CTRUNC      0x0200          /* Control data truncated */
/* Windows-specific flag values returned by recvmsg. */
#define MSG_BCAST	0x0400		/* Broadcast datagram */
#define MSG_MCAST	0x0800		/* Multicast datagram */

/* Setsockoptions(2) level. Thanks to BSD these must match IPPROTO_xxx */
#define SOL_IP		0
#define SOL_IPV6	41
#define SOL_IPX		256
#define SOL_AX25	257
#define SOL_ATALK	258
#define	SOL_NETROM	259
#define SOL_TCP		6
#define SOL_UDP		17

/* IP options */
#ifndef IPTOS_LOWDELAY
#define	IPTOS_LOWDELAY		0x10
#define	IPTOS_THROUGHPUT	0x08
#define	IPTOS_RELIABILITY	0x04
#endif

/* These need to appear somewhere around here */
#define IP_DEFAULT_MULTICAST_TTL        1
#define IP_DEFAULT_MULTICAST_LOOP       1
#define IP_MAX_MEMBERSHIPS              20

/* IP options for use with getsockopt/setsockopt */
#define IP_OPTIONS                       1
#define IP_HDRINCL                       2
#define IP_TOS                           3
#define IP_TTL                           4
#define IP_MULTICAST_IF                  9
#define IP_MULTICAST_TTL                10
#define IP_MULTICAST_LOOP               11
#define IP_ADD_MEMBERSHIP               12
#define IP_DROP_MEMBERSHIP              13
#define IP_DONTFRAGMENT                 14
#define IP_ADD_SOURCE_MEMBERSHIP        15
#define IP_DROP_SOURCE_MEMBERSHIP       16
#define IP_BLOCK_SOURCE                 17
#define IP_UNBLOCK_SOURCE               18
#define IP_PKTINFO                      19
#define IP_UNICAST_IF                   31

/* IPv6 options for use with getsockopt/setsockopt */
#define IPV6_HOPOPTS                     1
#define IPV6_UNICAST_HOPS                4
#define IPV6_MULTICAST_IF                9
#define IPV6_MULTICAST_HOPS             10
#define IPV6_MULTICAST_LOOP             11
#define IPV6_ADD_MEMBERSHIP             12
#define IPV6_DROP_MEMBERSHIP            13
#define IPV6_JOIN_GROUP                 IPV6_ADD_MEMBERSHIP
#define IPV6_LEAVE_GROUP                IPV6_DROP_MEMBERSHIP
#define IPV6_DONTFRAG                   14
#define IPV6_PKTINFO                    19
#define IPV6_HOPLIMIT                   21
#define IPV6_CHECKSUM                   26
#define IPV6_V6ONLY                     27
#define IPV6_UNICAST_IF                 31
#define IPV6_RTHDR                      32
#define IPV6_RECVRTHDR                  38
#define IPV6_TCLASS                     39
#define IPV6_RECVTCLASS                 40

/* IP agnostic options for use with getsockopt/setsockopt */
#define MCAST_JOIN_GROUP                41
#define MCAST_LEAVE_GROUP               42
#define MCAST_BLOCK_SOURCE              43
#define MCAST_UNBLOCK_SOURCE            44
#define MCAST_JOIN_SOURCE_GROUP         45
#define MCAST_LEAVE_SOURCE_GROUP        46

#ifndef __INSIDE_CYGWIN_NET__
#define MCAST_INCLUDE                    0
#define MCAST_EXCLUDE                    1
#endif

/* Old WinSock1 values, needed internally */
#ifdef __INSIDE_CYGWIN__
#define _WS1_IP_OPTIONS          1
#define _WS1_IP_MULTICAST_IF     2
#define _WS1_IP_MULTICAST_TTL    3
#define _WS1_IP_MULTICAST_LOOP   4
#define _WS1_IP_ADD_MEMBERSHIP   5
#define _WS1_IP_DROP_MEMBERSHIP  6
#define _WS1_IP_TTL              7
#define _WS1_IP_TOS              8
#define _WS1_IP_DONTFRAGMENT     9
#endif

/* IPX options */
#define IPX_TYPE	1

/* TCP options - this way around because someone left a set in the c library includes */
#ifndef TCP_NODELAY
#define TCP_NODELAY     0x0001
#define TCP_MAXSEG	2
#endif

/* SUS symbolic values for the second parm to shutdown(2) */
#define SHUT_RD   0		/* == Win32 SD_RECEIVE */
#define SHUT_WR   1		/* == Win32 SD_SEND    */
#define SHUT_RDWR 2		/* == Win32 SD_BOTH    */

/* The various priorities. */
#define SOPRI_INTERACTIVE	0
#define SOPRI_NORMAL		1
#define SOPRI_BACKGROUND	2

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* _CYGWIN_SOCKET_H */
