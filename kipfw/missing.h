/*
 * Copyright (C) 2009 Luigi Rizzo, Marta Carbone, Universita` di Pisa
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
 * $Id: missing.h 12256 2013-04-26 21:12:44Z luigi $
 *
 * Header for kernel variables and functions that are not available in
 * userland.
 */

#ifndef _MISSING_H_
#define _MISSING_H_

#include <sys/cdefs.h>
#ifdef linux
#include <linux/sysctl.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#endif /* linux */

/* portability features, to be set before the rest: */
#define HAVE_NET_IPLEN		/* iplen/ipoff in net format */
#define WITHOUT_BPF		/* do not use bpf logging */

#ifdef _WIN32

#ifndef DEFINE_SPINLOCK
#define DEFINE_SPINLOCK(x)	FAST_MUTEX x
#endif
/* spinlock --> Guarded Mutex KGUARDED_MUTEX */
/* http://www.reactos.org/wiki/index.php/Guarded_Mutex */
#define spin_lock_init(_l)
#define spin_lock_bh(_l)
#define spin_unlock_bh(_l)

#include <sys/socket.h>		/* bsd-compat.c */
#include <netinet/in.h>		/* bsd-compat.c */
#include <netinet/ip.h>		/* local version */
#define INADDR_TO_IFP(a, b) b = NULL

#else	/* __linux__ */

#define MALLOC_DECLARE(x)	/* nothing */
#include <linux/time.h>		/* do_gettimeofday */
#include <netinet/ip.h>		/* local version */
struct inpcb;

/*
 * Kernel locking support.
 * FreeBSD uses mtx in dummynet.c and struct rwlock ip_fw2.c
 *
 * In linux we use spinlock_bh to implement both.
 * For 'struct rwlock' we need an #ifdef to change it to spinlock_t
 */

#ifndef DEFINE_SPINLOCK	/* this is for linux 2.4 */
#define DEFINE_SPINLOCK(x)   spinlock_t x = SPIN_LOCK_UNLOCKED
#endif


#define rw_assert(a, b)
#define rw_destroy(_l)
#define rw_init(_l, msg)	spin_lock_init(_l)
#define rw_rlock(_l)		spin_lock_bh(_l)
#define rw_runlock(_l)		spin_unlock_bh(_l)
#define rw_wlock(_l)		spin_lock_bh(_l)
#define rw_wunlock(_l)		spin_unlock_bh(_l)
#define rw_init_flags(_l, s, v)

#define mtx_assert(a, b)
#define	mtx_destroy(m)
#define mtx_init(m, a,b,c) 	spin_lock_init(m)
#define mtx_lock(_l)		spin_lock_bh(_l)
#define mtx_unlock(_l)		spin_unlock_bh(_l)

#endif	/* __linux__ */
/* end of locking support */

/*
 * Reference to an ipfw rule that can be carried outside critical sections.
 * A rule is identified by rulenum:rule_id which is ordered.
 * In version chain_id the rule can be found in slot 'slot', so
 * we don't need a lookup if chain_id == chain->id.
 *
 * On exit from the firewall this structure refers to the rule after
 * the matching one (slot points to the new rule; rulenum:rule_id-1
 * is the matching rule), and additional info (e.g. info often contains
 * the insn argument or tablearg in the low 16 bits, in host format).
 * On entry, the structure is valid if slot>0, and refers to the starting
 * rules. 'info' contains the reason for reinject, e.g. divert port,
 * divert direction, and so on.
 */
struct ipfw_rule_ref {
	uint32_t	slot;		/* slot for matching rule	*/
	uint32_t	rulenum;	/* matching rule number		*/
	uint32_t	rule_id;	/* matching rule id		*/
	uint32_t	chain_id;	/* ruleset id			*/
	uint32_t	info;		/* see below			*/
};

enum {
	IPFW_INFO_MASK	= 0x0000ffff,
	IPFW_INFO_OUT	= 0x00000000,	/* outgoing, just for convenience */
	IPFW_INFO_IN	= 0x80000000,	/* incoming, overloads dir */
	IPFW_ONEPASS	= 0x40000000,	/* One-pass, do not reinject */
	IPFW_IS_MASK	= 0x30000000,	/* which source ? */
	IPFW_IS_DIVERT	= 0x20000000,
	IPFW_IS_DUMMYNET =0x10000000,
	IPFW_IS_PIPE	= 0x08000000,	/* pipe=1, queue = 0 */
};

/* in netinet/in.h */
#define        in_nullhost(x)  ((x).s_addr == INADDR_ANY)

/* bzero not present on linux, but this should go in glue.h */
#define bzero(s, n) memset(s, 0, n)
#define bcmp(p1, p2, n) memcmp(p1, p2, n)

/* ethernet stuff */
#define	ETHERTYPE_IP		0x0800	/* IP protocol */
//#define	ETHER_ADDR_LEN		6	/* length of an Ethernet address */
struct ether_header {
        u_char  ether_dhost[ETHER_ADDR_LEN];
        u_char  ether_shost[ETHER_ADDR_LEN];
        u_short ether_type;
};

#define ETHER_TYPE_LEN          2       /* length of the Ethernet type field */
#define ETHER_HDR_LEN           (ETHER_ADDR_LEN*2+ETHER_TYPE_LEN)

/*
 * Historically, BSD keeps ip_len and ip_off in host format
 * when doing layer 3 processing, and this often requires
 * to translate the format back and forth.
 * To make the process explicit, we define a couple of macros
 * that also take into account the fact that at some point
 * we may want to keep those fields always in net format.
 */

#if (BYTE_ORDER == BIG_ENDIAN) || defined(HAVE_NET_IPLEN)
#define SET_NET_IPLEN(p)        do {} while (0)
#define SET_HOST_IPLEN(p)       do {} while (0)
#else /* never on linux */
#define SET_NET_IPLEN(p)        do {            \
        struct ip *h_ip = (p);                  \
        h_ip->ip_len = htons(h_ip->ip_len);     \
        h_ip->ip_off = htons(h_ip->ip_off);     \
        } while (0)

#define SET_HOST_IPLEN(p)       do {            \
        struct ip *h_ip = (p);                  \
        h_ip->ip_len = ntohs(h_ip->ip_len);     \
        h_ip->ip_off = ntohs(h_ip->ip_off);     \
        } while (0)
#endif /* !HAVE_NET_IPLEN */

/* ip_dummynet.c */
#define __FreeBSD_version 500035

#ifdef __linux__
struct moduledata;
int my_mod_register(const char *name,
	int order, struct moduledata *mod, void *init, void *uninit);

/* define some macro for ip_dummynet */

struct malloc_type {
};

#define MALLOC_DEFINE(type, shortdesc, longdesc) 	\
	struct malloc_type type[1]; void *md_dummy_ ## type = type

#define CTASSERT(x)

/* log... does not use the first argument */
#define	LOG_ERR		0x100
#define	LOG_INFO	0x200
#define log(_level, fmt, arg...)  do {			\
	int _qwerty=_level;(void)_qwerty; printk(KERN_ERR fmt, ##arg); } while (0)

/*
 * gettimeofday would be in sys/time.h but it is not
 * visible if _KERNEL is defined
 */
int gettimeofday(struct timeval *, struct timezone *);

#else  /* _WIN32 */
#define MALLOC_DEFINE(a,b,c)
#endif /* _WIN32 */

extern int	hz;
extern long	tick;		/* exists in 2.4 but not in 2.6 */
extern int	bootverbose;
extern struct timeval boottime;

/* The time_uptime a FreeBSD variable increased each second */
#ifdef __linux__
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,4,37) /* revise boundaries */
#define time_uptime get_seconds()
#else /* OpenWRT */
#define time_uptime CURRENT_TIME
#endif
#else /* WIN32 */
#define time_uptime time_uptime_w32()
#endif

extern int	max_linkhdr;
extern int	ip_defttl;
extern u_long	in_ifaddrhmask;                         /* mask for hash table */
extern struct in_ifaddrhashhead *in_ifaddrhashtbl;    /* inet addr hash table  */

/*-------------------------------------------------*/

/* define, includes and functions missing in linux */
/* include and define */
#include <arpa/inet.h>		/* inet_ntoa */

struct mbuf;

/* used by ip_dummynet.c */
void reinject_drop(struct mbuf* m);

#include <linux/errno.h>	/* error define */
#include <linux/if.h>		/* IFNAMESIZ */

void rn_init(int);
/*
 * some network structure can be defined in the bsd way
 * by using the _FAVOR_BSD definition. This is not true
 * for icmp structure.
 * XXX struct icmp contains bsd names in 
 * /usr/include/netinet/ip_icmp.h
 */
#ifdef __linux__
#define icmp_code code
#define icmp_type type

/* linux in6_addr has no member __u6_addr
 * replace the whole structure ?
 */
#define __u6_addr       in6_u
#define __u6_addr32     u6_addr32
#endif /* __linux__ */

/* defined in linux/sctp.h with no bsd definition */
struct sctphdr {
        uint16_t src_port;      /* source port */
        uint16_t dest_port;     /* destination port */
        uint32_t v_tag;         /* verification tag of packet */
        uint32_t checksum;      /* Adler32 C-Sum */
        /* chunks follow... */
};

/* missing definition */
#define TH_FIN  0x01
#define TH_SYN  0x02
#define TH_RST  0x04
#define TH_ACK  0x10

#define RTF_CLONING	0x100		/* generate new routes on use */

#define IPPROTO_OSPFIGP         89              /* OSPFIGP */
#define IPPROTO_CARP            112             /* CARP */
#ifndef _WIN32
#define IPPROTO_IPV4            IPPROTO_IPIP    /* for compatibility */
#endif

#define	CARP_VERSION		2
#define	CARP_ADVERTISEMENT	0x01

#define PRIV_NETINET_IPFW       491     /* Administer IPFW firewall. */

#define IP_FORWARDING           0x1             /* most of ip header exists */

#define NETISR_IP       2               /* same as AF_INET */

#define PRIV_NETINET_DUMMYNET   494     /* Administer DUMMYNET. */

extern int securelevel;

struct carp_header {
#if BYTE_ORDER == LITTLE_ENDIAN
        u_int8_t        carp_type:4,
                        carp_version:4;
#endif
#if BYTE_ORDER == BIG_ENDIAN
        u_int8_t        carp_version:4,
                        carp_type:4;
#endif
};

struct pim {
	int dummy;      /* windows compiler does not like empty definition */
};

#ifndef _WIN32
struct route {
	struct  rtentry *ro_rt;
	struct  sockaddr ro_dst;
};
#endif

struct ifaltq {
	void *ifq_head;
};

/*
 * ifnet->if_snd is used in ip_dummynet.c to take the transmission
 * clock.
 */
#if defined( __linux__)
#define	if_xname	name
#define	if_snd		XXX
/* search local the ip addresses, used for the "me" keyword */
#include <linux/inetdevice.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,25)
#define INADDR_TO_IFP(ip, b)	\
	b = ip_dev_find(ip.s_addr)
#else
#define INADDR_TO_IFP(ip, b)	\
	b = ip_dev_find((struct net *)&init_net, ip.s_addr)
#endif

#elif defined( _WIN32 )
/* used in ip_dummynet.c */
struct ifnet {
	char    if_xname[IFNAMSIZ];     /* external name (name + unit) */
//        struct ifaltq if_snd;          /* output queue (includes altq) */
};

struct net_device {
	char    if_xname[IFNAMSIZ];     /* external name (name + unit) */
};
#endif

/* involves mbufs */
int in_cksum(struct mbuf *m, int len);
#define divert_cookie(mtag) 0
#define divert_info(mtag) 0
#define pf_find_mtag(a) NULL
#define pf_get_mtag(a) NULL
#ifndef _WIN32
#define AF_LINK AF_ASH	/* ? our sys/socket.h */
#endif

/* we don't pullup, either success or free and fail */
#define m_pullup(m, x)					\
	((m)->m_len >= x ? (m) : (FREE_PKT(m), NULL))

struct pf_mtag {
	void            *hdr;           /* saved hdr pos in mbuf, for ECN */
	sa_family_t      af;            /* for ECN */
        u_int32_t        qid;           /* queue id */
};

#if 0 // ndef radix
/* radix stuff in radix.h and radix.c */
struct radix_node {
	caddr_t rn_key;         /* object of search */
	caddr_t rn_mask;        /* netmask, if present */
};
#endif /* !radix */

/* missing kernel functions */
char *inet_ntoa(struct in_addr ina);
int random(void);

/*
 * Return the risult of a/b
 *
 * this is used in linux kernel space,
 * since the 64bit division needs to
 * be done using a macro
 */
int64_t
div64(int64_t a, int64_t b);

char *
inet_ntoa_r(struct in_addr ina, char *buf);

/* from bsd sys/queue.h */
#define TAILQ_FOREACH_SAFE(var, head, field, tvar)                      \
        for ((var) = TAILQ_FIRST((head));                               \
            (var) && ((tvar) = TAILQ_NEXT((var), field), 1);            \
            (var) = (tvar))

#define SLIST_FOREACH_SAFE(var, head, field, tvar)                      \
        for ((var) = SLIST_FIRST((head));                               \
            (var) && ((tvar) = SLIST_NEXT((var), field), 1);            \
            (var) = (tvar))

/* depending of linux version */
#ifndef ETHERTYPE_IPV6
#define ETHERTYPE_IPV6          0x86dd          /* IP protocol version 6 */
#endif

/*-------------------------------------------------*/
#define RT_NUMFIBS 1
extern u_int rt_numfibs;

/* involves kernel locking function */
#ifdef RTFREE
#undef RTFREE
#define RTFREE(a) fprintf(stderr, "RTFREE: commented out locks\n");
#endif

void getmicrouptime(struct timeval *tv);

/* from sys/netinet/ip_output.c */
struct ip_moptions;
struct route;
struct ip;

struct mbuf *ip_reass(struct mbuf *);
u_short in_cksum_hdr(struct ip *);
int ip_output(struct mbuf *m, struct mbuf *opt, struct route *ro, int flags,
    struct ip_moptions *imo, struct inpcb *inp);

/* from net/netisr.c */
void netisr_dispatch(int num, struct mbuf *m);

/* definition moved in missing.c */
int sooptcopyout(struct sockopt *sopt, const void *buf, size_t len);

int sooptcopyin(struct sockopt *sopt, void *buf, size_t len, size_t minlen);

/* defined in session.c */
int priv_check(struct thread *td, int priv);

/* struct ucred is in linux/socket.h and has pid, uid, gid.
 * We need a 'bsd_ucred' to store also the extra info
 */

struct bsd_ucred {
	uid_t		uid;
	gid_t		gid;
	uint32_t	xid;
	uint32_t	nid;
};

int
cred_check(void *insn, int proto, struct ifnet *oif,
    struct in_addr dst_ip, u_int16_t dst_port, struct in_addr src_ip,
    u_int16_t src_port, struct bsd_ucred *u, int *ugid_lookupp,
    struct sk_buff *skb);

int securelevel_ge(struct ucred *cr, int level);

struct sysctl_oid;
struct sysctl_req;

#ifdef _WIN32
#define module_param_named(_name, _var, _ty, _perm)
#else /* !_WIN32 */

/* Linux 2.4 is mostly for openwrt */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
#include <linux/bitops.h>	 /* generic_ffs() used in ip_fw2.c */
typedef uint32_t __be32;
typedef uint16_t __be16;
struct sock;
struct net;
struct inet_hashinfo;
struct sock *inet_lookup(
	struct inet_hashinfo *hashinfo,
        const __be32 saddr, const __be16 sport,
        const __be32 daddr, const __be16 dport,
        const int dif);
struct sock *tcp_v4_lookup(u32 saddr, u16 sport, u32 daddr, u16 dport, int dif);
#endif /* Linux < 2.6 */

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,17) &&	\
	LINUX_VERSION_CODE > KERNEL_VERSION(2,6,16)	/* XXX NOT sure, in 2.6.9 give an error */
#define module_param_named(_name, _var, _ty, _perm)	\
	//module_param(_name, _ty, 0644)
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24)
typedef unsigned long uintptr_t;

#ifdef __i386__
static inline unsigned long __fls(unsigned long word)
{
        asm("bsr %1,%0"
            : "=r" (word)
            : "rm" (word));
        return word;
}
#endif

#endif /* LINUX < 2.6.25 */

#endif /* !_WIN32 so maybe __linux__ */

#if defined (__linux__) && !defined (EMULATE_SYSCTL)
#define SYSCTL_DECL(_1)
#define SYSCTL_OID(_1, _2, _3, _4, _5, _6, _7, _8)
#define SYSCTL_NODE(_1, _2, _3, _4, _5, _6)
#define _SYSCTL_BASE(_name, _var, _ty, _perm)		\
	module_param_named(_name, *(_var), _ty, 	\
		( (_perm) == CTLFLAG_RD) ? 0444: 0644 )
#define SYSCTL_PROC(_base, _oid, _name, _mode, _var, _val, _desc, _a, _b)

#define SYSCTL_INT(_base, _oid, _name, _mode, _var, _val, _desc)	\
	_SYSCTL_BASE(_name, _var, int, _mode)

#define SYSCTL_LONG(_base, _oid, _name, _mode, _var, _val, _desc)	\
	_SYSCTL_BASE(_name, _var, long, _mode)

#define SYSCTL_ULONG(_base, _oid, _name, _mode, _var, _val, _desc)	\
	_SYSCTL_BASE(_name, _var, ulong, _mode)

#define SYSCTL_UINT(_base, _oid, _name, _mode, _var, _val, _desc)	\
	 _SYSCTL_BASE(_name, _var, uint, _mode)

#define TUNABLE_INT(_name, _ptr)

#define SYSCTL_VNET_PROC		SYSCTL_PROC
#define SYSCTL_VNET_INT			SYSCTL_INT
#define SYSCTL_VNET_UINT		SYSCTL_UINT

#endif

#define SYSCTL_HANDLER_ARGS 		\
	struct sysctl_oid *oidp, void *arg1, int arg2, struct sysctl_req *req
int sysctl_handle_int(SYSCTL_HANDLER_ARGS);
int sysctl_handle_long(SYSCTL_HANDLER_ARGS); 


void ether_demux(struct ifnet *ifp, struct mbuf *m);

int ether_output_frame(struct ifnet *ifp, struct mbuf *m);

void in_rtalloc_ign(struct route *ro, u_long ignflags, u_int fibnum);

void icmp_error(struct mbuf *n, int type, int code, uint32_t dest, int mtu);

void rtfree(struct rtentry *rt);

u_short in_cksum_skip(struct mbuf *m, int len, int skip);

#ifdef INP_LOCK_ASSERT
#undef INP_LOCK_ASSERT
#define INP_LOCK_ASSERT(a)
#endif

int jailed(struct ucred *cred);

/*
* Return 1 if an internet address is for a ``local'' host
* (one to which we have a connection).  If subnetsarelocal
* is true, this includes other subnets of the local net.
* Otherwise, it includes only the directly-connected (sub)nets.
*/
int in_localaddr(struct in_addr in);

/* the prototype is already in the headers */
//int ipfw_chg_hook(SYSCTL_HANDLER_ARGS); 

int fnmatch(const char *pattern, const char *string, int flags);

int
linux_lookup(const int proto, const __be32 saddr, const __be16 sport,
	const __be32 daddr, const __be16 dport,
	struct sk_buff *skb, int dir, struct bsd_ucred *u);

/* vnet wrappers, in vnet.h and ip_var.h */
//int ipfw_init(void);
//void ipfw_destroy(void);

#define	MTAG_IPFW	1148380143	/* IPFW-tagged cookie */
#define	MTAG_IPFW_RULE	1262273568	/* rule reference */

struct ip_fw_args;
extern int (*ip_dn_io_ptr)(struct mbuf **m, int dir, struct ip_fw_args *fwa);

#define curvnet                 NULL
#define	CURVNET_SET(_v)
#define	CURVNET_RESTORE()
#define VNET_ASSERT(condition)

#define VNET_NAME(n)            n
#define VNET_DECLARE(t, n)      extern t n
#define VNET_DEFINE(t, n)       t n
#define _VNET_PTR(b, n)         &VNET_NAME(n)
/*
 * Virtualized global variable accessor macros.
 */
#define VNET_VNET_PTR(vnet, n)          (&(n))
#define VNET_VNET(vnet, n)              (n)

#define VNET_PTR(n)             (&(n))
#define VNET(n)                 (n)

VNET_DECLARE(int, ip_defttl);
#define V_ip_defttl    VNET(ip_defttl);

int ipfw_check_hook(void *arg, struct mbuf **m0, struct ifnet *ifp,
	int dir, struct inpcb *inp);

/* hooks for divert */
extern void (*ip_divert_ptr)(struct mbuf *m, int incoming);

extern int (*ip_dn_ctl_ptr)(struct sockopt *);
typedef int ip_fw_ctl_t(struct sockopt *);
extern ip_fw_ctl_t *ip_fw_ctl_ptr;

/* netgraph prototypes */
typedef int ng_ipfw_input_t(struct mbuf **, int, struct ip_fw_args *, int);
extern  ng_ipfw_input_t *ng_ipfw_input_p;

/* For kernel ipfw_ether and ipfw_bridge. */
struct ip_fw_args;
typedef int ip_fw_chk_t(struct ip_fw_args *args);
extern  ip_fw_chk_t     *ip_fw_chk_ptr;

#define V_ip_fw_chk_ptr         VNET(ip_fw_chk_ptr)
#define V_ip_fw_ctl_ptr         VNET(ip_fw_ctl_ptr)
#define	V_tcbinfo		VNET(tcbinfo)
#define	V_udbinfo		VNET(udbinfo)

#endif /* !_MISSING_H_ */
