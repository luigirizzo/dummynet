/*
 * Copyright (c) 2010 Luigi Rizzo, Universita` di Pisa
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
 * headers to build userland ipfw under tcc.
 */
 
#ifndef _TCC_GLUE_H
#define	_TCC_GLUE_H

//#define	__restrict
#define	NULL	((void *)0)
typedef int size_t;
typedef unsigned char	u_char;
typedef unsigned char	uint8_t;
typedef unsigned char	u_int8_t;
typedef unsigned short	u_short;
typedef unsigned short	uint16_t;
typedef unsigned short	u_int16_t;
typedef int		__int32_t;
typedef int		int32_t;
typedef int		socklen_t;
typedef int		pid_t;
typedef unsigned int	time_t;
typedef unsigned int	uint;
typedef unsigned int	u_int;
typedef unsigned int	uint32_t;
typedef unsigned int	u_int32_t;
typedef unsigned int	gid_t;
typedef unsigned int	uid_t;
typedef unsigned long	u_long;
typedef unsigned long	uintptr_t;
typedef long long int	int64_t;
typedef unsigned long long	int uint64_t;
typedef unsigned long long	int u_int64_t;

typedef uint32_t	in_addr_t;
struct in_addr {
	uint32_t	s_addr;
};
struct sockaddr_in {
	uint8_t _sin_len;
        uint8_t	sin_family;
        uint16_t	sin_port;
        struct  in_addr sin_addr;
        char    sin_zero[8];
};
#define IFNAMSIZ	16
#define INET6_ADDRSTRLEN	64

struct in6_addr {
        union {
                uint8_t         __s6_addr8[16];
                uint16_t        __s6_addr16[8];
                uint32_t        __s6_addr32[4];
        } __u6; // _addr;                    /* 128-bit IP6 address */
};


#define LITTLE_ENDIAN 1234
#define BYTE_ORDER LITTLE_ENDIAN

/* to be revised */
#define	EX_OK		0
#define	EX_DATAERR	1
#define	EX_OSERR	2
#define	EX_UNAVAILABLE	3
#define	EX_USAGE	4
#define	EX_NOHOST	5

#define	EEXIST		1
#define	EINVAL		2
#define	ERANGE		3
#define	ESRCH		4

#define	IPPROTO_IP		1
#define	IPPROTO_IPV6		2
#define	IPPROTO_RAW		100

#define	IPTOS_LOWDELAY		100
#define	IPTOS_MINCOST		101
#define	IPTOS_RELIABILITY	102
#define	IPTOS_THROUGHPUT	103
#define	SOCK_RAW		12
#define	AF_INET			2
#define	AF_INET6		28

#define	INADDR_ANY		0


#define bcmp(src, dst, len)	memcmp(src, dst, len)
#define bcopy(src, dst, len)	memcpy(dst, src, len)
#define bzero(p, len)	memset(p, 0, len)
#define index(s, c)	strchr(s, c)

char *strsep(char **stringp, const char *delim);

void    warn(const char *, ...);
//void    warnx(const char *, ...);
#define warnx warn
void    err(int, const char *, ...);
#define	errx err

uint16_t	htons(uint16_t)__attribute__ ((stdcall));
uint16_t	ntohs(uint16_t)__attribute__ ((stdcall));
uint32_t	htonl(uint32_t)__attribute__ ((stdcall));
uint32_t	ntohl(uint32_t)__attribute__ ((stdcall));
int inet_aton(const char *cp, struct in_addr *pin)__attribute__ ((stdcall));;
char * inet_ntoa(struct in_addr)__attribute__ ((stdcall));;
const char * inet_ntop(int af, const void * src, char * dst,
         socklen_t size)__attribute__ ((stdcall));;
int inet_pton(int af, const char * src, void * dst)__attribute__ ((stdcall));;

struct group {
	gid_t	gr_gid;
	char	gr_name[16];
};
struct passwd {
	uid_t	pw_uid;
	char	pw_name[16];
};

#define getpwnam(s)	(NULL)
#define getpwuid(s)	(NULL)

#define getgrnam(x) (NULL)
#define getgrgid(x) (NULL)

int getopt(int argc, char * const argv[], const char *optstring);

int getsockopt(int s, int level, int optname, void * optval,
         socklen_t * optlen);

int setsockopt(int s, int level, int optname, const void *optval,
         socklen_t optlen);

struct  protoent {
        char    *p_name;           /* official protocol name */
        char    **p_aliases;  /* alias list */
        short   p_proto;                /* protocol # */
};

struct  servent {
        char    *s_name;           /* official service name */
        char    **s_aliases;  /* alias list */
        short   s_port;                 /* port # */
        char    *s_proto;          /* protocol to use */
};

struct  hostent {
        char    *h_name;           /* official name of host */
        char    **h_aliases;  /* alias list */
        short   h_addrtype;             /* host address type */
        short   h_length;               /* length of address */
        char    **h_addr_list; /* list of addresses */
#define h_addr  h_addr_list[0]          /* address, for backward compat */
};

struct hostent* gethostbyaddr(const char* addr, int len, int type)__attribute__ ((stdcall));
struct hostent* gethostbyname(const char *name)__attribute__ ((stdcall));

struct protoent* getprotobynumber(int number)__attribute__ ((stdcall));
struct protoent* getprotobyname(const char* name)__attribute__ ((stdcall));

struct servent* getservbyport(int port, const char* proto)__attribute__ ((stdcall));
struct servent* getservbyname(const char* name, const char* proto) __attribute__ ((stdcall));

extern int optind;
extern char *optarg;

#include <windef.h>

#define WSADESCRIPTION_LEN      256
#define WSASYS_STATUS_LEN       128

typedef struct WSAData {
        WORD                    wVersion;
        WORD                    wHighVersion;
        char                    szDescription[WSADESCRIPTION_LEN+1];
        char                    szSystemStatus[WSASYS_STATUS_LEN+1];
        unsigned short          iMaxSockets;
        unsigned short          iMaxUdpDg;
        char FAR *              lpVendorInfo;
} WSADATA, * LPWSADATA;

int WSAStartup(
    WORD wVersionRequested,
    LPWSADATA lpWSAData
    );

int
WSACleanup(void);

int WSAGetLastError();

/* return error on process handling */
#define	pipe(f)		(-1)
#define	kill(p, s)	(-1)
#define	waitpid(w,s,o)	(-1)
#define fork(x)		(-1)
#define execvp(f, a)	(-1)

#define _W_INT(i)       (i)
#define _WSTATUS(x)     (_W_INT(x) & 0177)
#define WIFEXITED(x)    (_WSTATUS(x) == 0)
#define WEXITSTATUS(x)  (_W_INT(x) >> 8)
#define _WSTOPPED       0177            /* _WSTATUS if process is stopped */
#define WIFSIGNALED(x)  (_WSTATUS(x) != _WSTOPPED && _WSTATUS(x) != 0)
#define WTERMSIG(x)     (_WSTATUS(x))

#endif /* _TCC_GLUE_H */
