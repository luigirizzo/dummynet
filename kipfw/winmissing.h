/*
 * Copyright (c) 2010 Francesco Magno, Universita` di Pisa
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * $Id: winmissing.h 11647 2012-08-06 23:20:21Z luigi $
 * definitions and other things needed to build freebsd kernel
 * modules in Windows (with the MSVC compiler)
 */

#ifndef _WINMISSING_H_
#define _WINMISSING_H_

#include <ntifs.h>
#include <ntddk.h>
#include <basetsd.h>
#include <windef.h>
#include <stdio.h>
#include <ndis.h>

typedef UCHAR	u_char;
typedef UCHAR	u_int8_t;
typedef UCHAR	uint8_t;
typedef USHORT	u_short;
typedef USHORT	u_int16_t;
typedef USHORT	uint16_t;
typedef USHORT	n_short;
typedef UINT	u_int;
typedef INT32	int32_t;
typedef UINT32	u_int32_t;
typedef UINT32	uint32_t;
typedef ULONG	u_long;
typedef ULONG	n_long;
typedef UINT64	uint64_t;
typedef UINT64	u_int64_t;
typedef INT64	int64_t;

typedef UINT32	in_addr_t;
typedef UCHAR	sa_family_t;
typedef	USHORT	in_port_t;
typedef UINT32	__gid_t;
typedef UINT32	gid_t;
typedef UINT32	__uid_t;
typedef UINT32	uid_t;
typedef ULONG	n_time;
typedef char*	caddr_t;

/* linux_lookup uses __be32 and __be16 in the prototype */
typedef uint32_t __be32; /* XXX __u32 __bitwise __be32 */
typedef uint16_t __be16; /* XXX */

//*** DEBUG STUFF ***
/*
 * To see the debugging messages you need DbgView
http://technet.microsoft.com/en-us/sysinternals/bb896647.aspx
 */
#define printf		DbgPrint
#define log(lev, ...)	DbgPrint(__VA_ARGS__)
const char* texify_cmd(int i);
const char* texify_proto(unsigned int p);
//*** end DEBUG STUFF ***

#define snprintf _snprintf
#define timespec timeval
struct timeval {
	long tv_sec;
	long tv_usec;
};

struct in_addr {
	in_addr_t s_addr;
};

struct sockaddr_in {
	uint8_t	sin_len;
	sa_family_t	sin_family;
	in_port_t	sin_port;
	struct	in_addr sin_addr;
	char	sin_zero[8];
};

/* XXX watch out, windows names are actually longer */
#define IFNAMSIZ	16
#define IF_NAMESIZE	16

#define ETHER_ADDR_LEN 6

/* we do not include the windows headers for in6_addr so
 * we need to provide our own definition for the kernel.
 */
struct in6_addr {
        union {
                uint8_t         __u6_addr8[16];
                uint16_t        __u6_addr16[8]; 
                uint32_t        __u6_addr32[4];
        } __u6_addr;                    /* 128-bit IP6 address */
};

#define	htons(x) RtlUshortByteSwap(x)
#define	ntohs(x) RtlUshortByteSwap(x)
#define	htonl(x) RtlUlongByteSwap(x)
#define	ntohl(x) RtlUlongByteSwap(x)

#define ENOSPC          28      /* No space left on device */
#define	EOPNOTSUPP	45	/* Operation not supported */
#define	EACCES		13	/* Permission denied */
#define	ENOENT		2	/* No such file or directory */
#define EINVAL          22      /* Invalid argument */
#define	EPROTONOSUPPORT	43	/* Protocol not supported */
#define	ENOMEM		12	/* Cannot allocate memory */
#define	EEXIST		17	/* File exists */
#define ESRCH		3
#define	ENOBUFS		55	/* No buffer space available */
#define	EBUSY		16	/* Module busy */


#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#define __packed 
#define __aligned(x);
#define __user
#define __init
#define __exit
#define __func__ __FUNCTION__
#define inline __inline

struct sockaddr_in6 {
	int dummy;
};

//SPINLOCKS
#define DEFINE_SPINLOCK(x)		NDIS_SPIN_LOCK x
#define mtx_init(m,a,b,c)		NdisAllocateSpinLock(m)
#define mtx_lock(_l)			NdisAcquireSpinLock(_l)
#define mtx_unlock(_l)			NdisReleaseSpinLock(_l)
#define	mtx_destroy(m)			NdisFreeSpinLock(m)
#define mtx_assert(a, b)

#define rw_rlock(_l)			NdisAcquireSpinLock(_l)
#define rw_runlock(_l)			NdisReleaseSpinLock(_l)
#define rw_assert(a, b)
#define rw_wlock(_l)			NdisAcquireSpinLock(_l)
#define rw_wunlock(_l)			NdisReleaseSpinLock(_l)
#define rw_destroy(_l)			NdisFreeSpinLock(_l)
#define rw_init(_l, msg)		NdisAllocateSpinLock(_l)
#define rw_init_flags(_l, s, v)		NdisAllocateSpinLock(_l)

#define rwlock_t NDIS_SPIN_LOCK
#define spinlock_t NDIS_SPIN_LOCK

#define s6_addr   __u6_addr.__u6_addr8


struct icmphdr {
	u_char	icmp_type;		/* type of message, see below */
	u_char	icmp_code;		/* type sub code */
	u_short	icmp_cksum;		/* ones complement cksum of struct */
};

#define	ICMP_ECHO		8		/* echo service */

#define IPOPT_OPTVAL            0               /* option ID */
#define IPOPT_OLEN              1               /* option length */
#define IPOPT_EOL               0               /* end of option list */
#define IPOPT_NOP               1               /* no operation */
#define IPOPT_LSRR              131             /* loose source route */
#define IPOPT_SSRR              137             /* strict source route */
#define IPOPT_RR                7               /* record packet route */
#define IPOPT_TS                68              /* timestamp */

#define	IPPROTO_ICMP	1		/* control message protocol */
#define	IPPROTO_TCP		6		/* tcp */
#define	IPPROTO_UDP		17		/* user datagram protocol */
#define	IPPROTO_ICMPV6		58		/* ICMP6 */
#define	IPPROTO_SCTP		132		/* SCTP */
#define	IPPROTO_HOPOPTS		0		/* IP6 hop-by-hop options */
#define	IPPROTO_ROUTING		43		/* IP6 routing header */
#define	IPPROTO_FRAGMENT	44		/* IP6 fragmentation header */
#define	IPPROTO_DSTOPTS		60		/* IP6 destination option */
#define	IPPROTO_AH		51		/* IP6 Auth Header */
#define	IPPROTO_ESP		50		/* IP6 Encap Sec. Payload */
#define	IPPROTO_NONE		59		/* IP6 no next header */
#define	IPPROTO_PIM		103		/* Protocol Independent Mcast */

#define IPPROTO_IPV6		41
#define	IPPROTO_IPV4		4		/* IPv4 encapsulation */


#define	INADDR_ANY		(uint32_t)0x00000000

#define	AF_INET		2		/* internetwork: UDP, TCP, etc. */
#define	AF_LINK		18		/* Link layer interface */

#define	IN_CLASSD(i)		(((uint32_t)(i) & 0xf0000000) == 0xe0000000)
#define	IN_MULTICAST(i)		IN_CLASSD(i)

#define DROP 0
#define PASS 1
#define DUMMYNET 2
#define INCOMING 0
#define OUTGOING 1

size_t strlcpy(char *dst, const char *src, size_t siz);
void do_gettimeofday(struct timeval *tv);
int ffs(int bits);
int time_uptime_w32();

#endif /* _WINMISSING_H_ */
