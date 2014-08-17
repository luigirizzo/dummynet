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
 * $Id: ipfw2_mod.c 12501 2014-01-10 01:09:14Z luigi $
 *
 * The main interface to build ipfw+dummynet as a linux module.
 * (and possibly as a windows module as well, though that part
 * is not complete yet).
 *
 * The control interface uses the sockopt mechanism
 * on a socket(AF_INET, SOCK_RAW, IPPROTO_RAW).
 *
 * The data interface uses the netfilter interface, at the moment
 * hooked to the PRE_ROUTING and POST_ROUTING hooks.
 * Unfortunately the netfilter interface is a moving target,
 * so we need a set of macros to adapt to the various cases.
 *
 * In the netfilter hook we just mark packet as 'QUEUE' and then
 * let the queue handler to do the whole work (filtering and
 * possibly emulation).
 * As we receive packets, we wrap them with an mbuf descriptor
 * so the existing ipfw+dummynet code runs unmodified.
 */

#include <sys/cdefs.h>
#include <sys/mbuf.h>			/* sizeof struct mbuf */
#include <sys/param.h>			/* NGROUPS */

#ifndef D
#define ND(fmt, ...) do {} while (0)
#define D1(fmt, ...) do {} while (0)
#define D(fmt, ...) printf("%-10s " fmt "\n",      \
        __FUNCTION__, ## __VA_ARGS__)
#endif

#ifdef __linux__
#include <linux/module.h>
#include <linux/kernel.h>

#ifndef CONFIG_NETFILTER
#error should configure netfilter (broken on 2.6.26 and below ?)
#endif

#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>	/* NF_IP_PRI_FILTER */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,25)
#include <net/netfilter/nf_queue.h>	/* nf_queue */
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,14)
#define __read_mostly
#endif

#endif /* !__linux__ */

#include <netinet/in.h>			/* in_addr */
#include <netinet/ip_fw.h>		/* ip_fw_ctl_t, ip_fw_chk_t */
#include <netinet/ipfw/ip_fw_private.h>		/* ip_fw_ctl_t, ip_fw_chk_t */
#include <netinet/ip_dummynet.h>	/* ip_dn_ctl_t, ip_dn_io_t */
#include <net/pfil.h>			/* PFIL_IN, PFIL_OUT */

#ifdef __linux__

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,13)
/* XXX was < 2.6.0:  inet_hashtables.h is introduced in 2.6.14 */
// #warning --- inet_hashtables not present on 2.4
#include <linux/tcp.h>
#include <net/route.h>
#include <net/sock.h>
static inline int inet_iif(const struct sk_buff *skb)
{
        return ((struct rtable *)skb->dst)->rt_iif;
}

#else
#include <net/inet_hashtables.h>	/* inet_lookup */
#endif
#endif /* __linux__ */

#include <net/route.h>			/* inet_iif */

/*
 * Here we allocate some global variables used in the firewall.
 */
//ip_dn_ctl_t    *ip_dn_ctl_ptr;
int (*ip_dn_ctl_ptr)(struct sockopt *);

ip_fw_ctl_t    *ip_fw_ctl_ptr;

int	(*ip_dn_io_ptr)(struct mbuf **m, int dir, struct ip_fw_args *fwa);
ip_fw_chk_t    *ip_fw_chk_ptr;

void		(*bridge_dn_p)(struct mbuf *, struct ifnet *);

/* Divert hooks. */
void (*ip_divert_ptr)(struct mbuf *m, int incoming);

/* ng_ipfw hooks. */
ng_ipfw_input_t *ng_ipfw_input_p = NULL;

/*---
 * Glue code to implement the registration of children with the parent.
 * Each child should call my_mod_register() when linking, so that
 * module_init() and module_exit() can call init_children() and
 * fini_children() to provide the necessary initialization.
 * We use the same mechanism for MODULE_ and SYSINIT_.
 * The former only get a pointer to the moduledata,
 * the latter have two function pointers (init/uninit)
 */
#include <sys/module.h>
struct mod_args {
        const char *name;
        int order;
        struct moduledata *mod;
	void (*init)(void), (*uninit)(void);
};

static unsigned int mod_idx;
static struct mod_args mods[10];	/* hard limit to 10 modules */

int
my_mod_register(const char *name, int order,
	struct moduledata *mod, void *init, void *uninit);
/*
 * my_mod_register should be called automatically as the init
 * functions in the submodules. Unfortunately this compiler/linker
 * trick is not supported yet so we call it manually.
 */
int
my_mod_register(const char *name, int order,
	struct moduledata *mod, void *init, void *uninit)
{
	struct mod_args m;

	m.name = name;
	m.order = order;
	m.mod = mod;
	m.init = init;
	m.uninit = uninit;

	printf("%s %s called\n", __FUNCTION__, name);
	if (mod_idx < sizeof(mods) / sizeof(mods[0]))
		mods[mod_idx++] = m;
	return 0;
}

static void
init_children(void)
{
	unsigned int i;

        /* Call the functions registered at init time. */
	printf("%s mod_idx value %d\n", __FUNCTION__, mod_idx);
        for (i = 0; i < mod_idx; i++) {
		struct mod_args *m = &mods[i];
                printf("+++ start module %d %s %s at %p order 0x%x\n",
                        i, m->name, m->mod ? m->mod->name : "SYSINIT",
                        m->mod, m->order);
		if (m->mod && m->mod->evhand)
			m->mod->evhand(NULL, MOD_LOAD, m->mod->priv);
		else if (m->init)
			m->init();
        }
}

static void
fini_children(void)
{
	int i;

        /* Call the functions registered at init time. */
        for (i = mod_idx - 1; i >= 0; i--) {
		struct mod_args *m = &mods[i];
                printf("+++ end module %d %s %s at %p order 0x%x\n",
                        i, m->name, m->mod ? m->mod->name : "SYSINIT",
                        m->mod, m->order);
		if (m->mod && m->mod->evhand)
			m->mod->evhand(NULL, MOD_UNLOAD, m->mod->priv);
		else if (m->uninit)
			m->uninit();
        }
}
/*--- end of module binding helper functions ---*/

/*---
 * Control hooks:
 * ipfw_ctl_h() is a wrapper for linux to FreeBSD sockopt call convention.
 * then call the ipfw handler in order to manage requests.
 * In turn this is called by the linux set/get handlers.
 */
static int
ipfw_ctl_h(struct sockopt *s, int cmd, int dir, int len, void __user *user)
{
	struct thread t;
	int ret = EINVAL;

	memset(s, 0, sizeof(*s));
	s->sopt_name = cmd;
	s->sopt_dir = dir;
	s->sopt_valsize = len;
	s->sopt_val = user;

	/* sopt_td is not used but it is referenced */
	memset(&t, 0, sizeof(t));
	s->sopt_td = &t;
	
	//printf("%s called with cmd %d len %d sopt %p user %p\n", __FUNCTION__, cmd, len, s, user);

	if (ip_fw_ctl_ptr && cmd != IP_DUMMYNET3 && (cmd == IP_FW3 ||
	    cmd < IP_DUMMYNET_CONFIGURE))
		ret = ip_fw_ctl_ptr(s);
	else if (ip_dn_ctl_ptr && (cmd == IP_DUMMYNET3 ||
	    cmd >= IP_DUMMYNET_CONFIGURE))
		ret = ip_dn_ctl_ptr(s);
	
	return -ret;	/* errors are < 0 on linux */
}

#ifdef linux
/*
 * Convert an mbuf into an skbuff
 * At the moment this only works for ip packets fully contained
 * in a single mbuf. We assume that on entry ip_len and ip_off are
 * in host format, and the ip checksum is not computed.
 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0) /* check boundary */
int dst_output(struct skbuff *s)
{
	return 0;
}

struct sk_buff *
mbuf2skbuff(struct mbuf* m)
{
	return NULL;
}
#else
struct sk_buff *
mbuf2skbuff(struct mbuf* m)
{
	struct sk_buff *skb;
	size_t len = m->m_pkthdr.len;

	/* used to lookup the routing table */
	struct rtable *r;
	struct flowi fl;
	int ret = 0;	/* success for ip_route_output_key() */

	struct ip *ip = mtod(m, struct ip *);

	/* XXX ip_output has ip_len and ip_off in network format,
	 * linux expects host format */
	ip->ip_len = ntohs(ip->ip_len);
	ip->ip_off = ntohs(ip->ip_off);

	ip->ip_sum = 0;
	ip->ip_sum = in_cksum(m, ip->ip_hl<<2);

	/* fill flowi struct, we need just the dst addr, see XXX */
	bzero(&fl, sizeof(fl));
	flow_daddr.daddr = ip->ip_dst.s_addr;

	/*
	 * ip_route_output_key() should increment
	 * r->u.dst.__use and call a dst_hold(dst)
	 * XXX verify how we release the resources.
	 */
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,38) /* check boundary */
	r = ip_route_output_key(&init_net, &fl.u.ip4);
#elif LINUX_VERSION_CODE > KERNEL_VERSION(2,6,26) /* check boundary */
	ret = ip_route_output_key(&init_net, &r, &fl);
#else
	ret = ip_route_output_key(&r, &fl);
#endif
	if (ret != 0 || r == NULL ) {
		printf("NO ROUTE FOUND\n");
		return NULL;
	}

	/* allocate the skbuff and the data */
	skb = alloc_skb(len + sizeof(struct ethhdr), GFP_ATOMIC);
	if (skb == NULL) {
		printf("%s: can not allocate SKB buffers.\n", __FUNCTION__);
		return NULL;
	}

	skb->protocol = htons(ETH_P_IP); // XXX 8 or 16 bit ?
	/* sk_dst_set XXX take the lock (?) */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36)
	skb_dst_set(skb, &r->u.dst);
#else
	skb_dst_set(skb, &r->dst);
#endif
	skb->dev = skb_dst(skb)->dev;

	/* reserve space for ethernet header */
	skb_reserve(skb, sizeof(struct ethhdr));

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,22)
	skb_reset_network_header(skb); // skb->network_header = skb->data - skb->head
#else
	skb->nh.raw = skb->data;
#endif
	/* set skbuff tail pointers and copy content */
	skb_put(skb, len);
	memcpy(skb->data, m->m_data, len);

	return skb;
}
#endif /* linux 2.6+ */
#endif /* linux */


/*
 * This function is called to reinject packets to the
 * kernel stack within the linux netfilter system
 * or to send a new created mbuf.
 * In the first case we have a valid sk_buff pointer
 * encapsulated within the fake mbuf, so we can call
 * the reinject function trough netisr_dispatch.
 * In the last case we need to build a sk_buff from scratch,
 * before sending out the packet.
 */
int
ip_output(struct mbuf *m, struct mbuf *opt, struct route *ro, int flags,
    struct ip_moptions *imo, struct inpcb *inp)
{
	(void)opt; (void)ro; (void)flags; (void)imo; (void)inp;	/* UNUSED */
	if ( m->m_skb != NULL ) { /* reinjected packet, just call dispatch */
		ND("sending... ");
		netisr_dispatch(0, m);
	} else {
		/* self-generated packet, wrap as appropriate and send */
#ifdef __linux__
		struct sk_buff *skb = mbuf2skbuff(m);

		if (skb != NULL)
			dst_output(skb);
#else /* Windows */
		D("unimplemented.");
#endif
		FREE_PKT(m);
	}
	return 0;
}

/*
 * setsockopt hook has no return value other than the error code.
 */
int
do_ipfw_set_ctl(struct sock *sk, int cmd, void __user *user, unsigned int len)
{
	struct sockopt s;	/* pass arguments */
	(void)sk;		/* UNUSED */
	return ipfw_ctl_h(&s, cmd, SOPT_SET, len, user);
}

/*
 * getsockopt can can return a block of data in response.
 */
int
do_ipfw_get_ctl(struct sock *sk, int cmd, void __user *user, int *len)
{
	struct sockopt s;	/* pass arguments */
	int ret = ipfw_ctl_h(&s, cmd, SOPT_GET, *len, user);

	(void)sk;		/* UNUSED */
	*len = s.sopt_valsize;	/* return length back to the caller */
	return ret;
}

#ifdef __linux__

/*
 * declare our [get|set]sockopt hooks
 */
static struct nf_sockopt_ops ipfw_sockopts = {
	.pf		= PF_INET,
	.set_optmin	= _IPFW_SOCKOPT_BASE,
	.set_optmax	= _IPFW_SOCKOPT_END,
	.set		= do_ipfw_set_ctl,
	.get_optmin	= _IPFW_SOCKOPT_BASE,
	.get_optmax	= _IPFW_SOCKOPT_END,
	.get		= do_ipfw_get_ctl,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24)
	.owner		= THIS_MODULE,
#endif
};

/*----
 * We need a number of macros to adapt to the various APIs in
 * different linux versions. Among them:
 *
 * - the hook names change between macros (NF_IP*) and enum NF_INET_*
 *
 * - the second argument to the netfilter hook is
 *	struct sk_buff **	in kernels <= 2.6.22
 *	struct sk_buff *	in kernels > 2.6.22
 *
 * - NF_STOP is not defined before 2.6 so we remap it to NF_ACCEPT
 *
 * - the packet descriptor passed to the queue handler is
 *	struct nf_info		in kernels <= 2.6.24
 *	struct nf_queue_entry	in kernels <= 2.6.24
 *
 * - the arguments to the queue handler also change;
 */

/*
 * declare hook to grab packets from the netfilter interface.
 * The NF_* names change in different versions of linux, in some
 * cases they are #defines, in others they are enum, so we
 * need to adapt.
 */
#ifndef NF_IP_PRE_ROUTING
#define NF_IP_PRE_ROUTING	NF_INET_PRE_ROUTING
#endif
#ifndef NF_IP_POST_ROUTING
#define NF_IP_POST_ROUTING	NF_INET_POST_ROUTING
#endif

/*
 * ipfw hooks into the POST_ROUTING and the PRE_ROUTING chains.
 * PlanetLab sets skb_tag to the slice id in the LOCAL_INPUT and
 * POST_ROUTING chains, so if we want to use that information we
 * need to hook the LOCAL_INPUT chain instead of the PRE_ROUTING.
 * However at the moment the skb_tag info is not reliable so
 * we stay with the standard hooks.
 */
#if 0 // defined(IPFW_PLANETLAB)
#define IPFW_HOOK_IN NF_IP_LOCAL_IN
#else
#define IPFW_HOOK_IN NF_IP_PRE_ROUTING
#endif

/*
 * The main netfilter hook.
 * To make life simple, we queue everything and then do all the
 * decision in the queue handler.
 *
 * XXX note that in 2.4 and up to 2.6.22 the skbuf is passed as sk_buff**
 * so we have an #ifdef to set the proper argument type.
 */
static unsigned int
call_ipfw(
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,13,0)
	unsigned int hooknum,
#else
	const struct nf_hook_ops *hooknum,
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,23) // in 2.6.22 we have **
	struct sk_buff  **skb,
#else
	struct sk_buff  *skb,
#endif
	const struct net_device *in, const struct net_device *out,
	int (*okfn)(struct sk_buff *))
{
	(void)hooknum; (void)skb; (void)in; (void)out; (void)okfn; /* UNUSED */
	return NF_QUEUE;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,12)	/* XXX was 2.6.0 */
#define	NF_STOP		NF_ACCEPT
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,25)

/*
 * nf_queue_entry is a recent addition, in previous versions
 * of the code the struct is called nf_info.
 */
#define nf_queue_entry	nf_info	/* for simplicity */

/* also, 2.4 and perhaps something else have different arguments */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,14)	/* XXX unsure */
/* on 2.4 we use nf_info */
#define QH_ARGS		struct sk_buff *skb, struct nf_info *info, void *data
#else	/* 2.6.14. 2.6.24 */
#define QH_ARGS		struct sk_buff *skb, struct nf_info *info, unsigned int qnum, void *data
#endif

#define DEFINE_SKB	/* nothing, already an argument */
#define	REINJECT(_inf, _verd)	nf_reinject(skb, _inf, _verd)

#else	/* 2.6.25 and above */

#define QH_ARGS		struct nf_queue_entry *info, unsigned int queuenum
#define DEFINE_SKB	struct sk_buff *skb = info->skb;
#define	REINJECT(_inf, _verd)	nf_reinject(_inf, _verd)
#endif

/*
 * used by dummynet when dropping packets
 * XXX use dummynet_send()
 */
void
reinject_drop(struct mbuf* m)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,25)	/* unsure on the exact boundary */
	struct sk_buff *skb = (struct sk_buff *)m;
#endif
	REINJECT(m->queue_entry, NF_DROP);
}

/*
 * The real call to the firewall. nf_queue_entry points to the skbuf,
 * and eventually we need to return both through nf_reinject().
 */
static int
ipfw2_queue_handler(QH_ARGS)
{
	DEFINE_SKB	/* no semicolon here, goes in the macro */
	int ret = 0;	/* return value */
	struct mbuf *m;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
	if (skb->nh.iph == NULL) {
		printf("null dp, len %d reinject now\n", skb->len);
		REINJECT(info, NF_ACCEPT);
		return 0;
	}
#endif
	m = malloc(sizeof(*m), 0, 0);
	if (m == NULL) {
		printf("malloc fail, len %d reinject now\n", skb->len);
		REINJECT(info, NF_ACCEPT);
		return 0;
	}

	m->m_skb = skb;
	m->m_len = skb->len;		/* len from ip header to end */
	m->m_pkthdr.len = skb->len;	/* total packet len */
	m->m_pkthdr.rcvif = info->indev;
	m->queue_entry = info;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,22)	/* XXX was 2.6.0 */
	m->m_data = (char *)skb->nh.iph;
#else
	m->m_data = (char *)skb_network_header(skb);	// XXX unsigned ? */
#endif

	/* XXX add the interface */
	if (info->hook == IPFW_HOOK_IN) {
		ret = ipfw_check_hook(NULL, &m, info->indev, PFIL_IN, NULL);
	} else {
		ret = ipfw_check_hook(NULL, &m, info->outdev, PFIL_OUT, NULL);
	}

	if (m != NULL) {	/* Accept. reinject and free the mbuf */
		REINJECT(info, NF_ACCEPT);
		m_freem(m);
	} else if (ret == 0) {
		/* dummynet has kept the packet, will reinject later. */
	} else {
		/*
		 * Packet dropped by ipfw or dummynet. Nothing to do as
		 * FREE_PKT already did a reinject as NF_DROP
		 */
	}
	return 0;
}

struct route;
struct ip_moptions;
struct inpcb;

/* XXX should include prototypes for netisr_dispatch and ip_output */
/*
 * The reinjection routine after a packet comes out from dummynet.
 * We must update the skb timestamp so ping reports the right time.
 * This routine is also used (with num == -1) as FREE_PKT. XXX
 */
void
netisr_dispatch(int num, struct mbuf *m)
{
	struct nf_queue_entry *info = m->queue_entry;
	struct sk_buff *skb = m->m_skb;	/* always used */

	/*
	 * This function can be called by the FREE_PKT()
	 * used when ipfw generate their own mbuf packets
	 * or by the mbuf2skbuff() function.
	 */
	m_freem(m);

	/* XXX check
	 * info is null in the case of a real mbuf
	 * (one created by the ipfw code without a
	 * valid sk_buff pointer
	 */
	if (info == NULL)
		return;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,22)	// XXX above 2.6.x ?
	__net_timestamp(skb);	/* update timestamp */
#endif

	/* XXX to obey one-pass, possibly call the queue handler here */
	REINJECT(info, ((num == -1)?NF_DROP:NF_STOP));	/* accept but no more firewall */
}

/*
 * socket lookup function for linux.
 * This code is used to associate uid, gid, jail/xid to packets,
 * and store the info in a cache *ugp where they can be accessed quickly.
 * The function returns 1 if the info is found, -1 otherwise.
 *
 * We do this only on selected protocols: TCP, ...
 *
 * The chain is the following
 *   sk_buff*  sock*  socket*    file*
 *	skb  ->  sk ->sk_socket->file ->f_owner    ->pid
 *	skb  ->  sk ->sk_socket->file ->f_uid (direct)
 *	skb  ->  sk ->sk_socket->file ->f_cred->fsuid (2.6.29+)
 *
 * Related headers:
 * linux/skbuff.h	struct skbuff
 * net/sock.h		struct sock
 * linux/net.h		struct socket
 * linux/fs.h		struct file
 *
 * With vserver we may have sk->sk_xid and sk->sk_nid that
 * which we store in fw_groups[1] (matches O_JAIL) and fw_groups[2]
 * (no matches yet)
 *
 * Note- for locally generated, outgoing packets we should not need
 * need a lookup because the sk_buff already points to the socket where
 * the info is.
 */
extern struct inet_hashinfo tcp_hashinfo;
int
linux_lookup(const int proto, const __be32 saddr, const __be16 sport,
		const __be32 daddr, const __be16 dport,
		struct sk_buff *skb, int dir, struct bsd_ucred *u)
{
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,13) 	/* XXX was 2.6.0 */
	return -1;
#else
	struct sock *sk;
	int ret = -1;	/* default return value */
	int st = -1;	/* state */


	if (proto != IPPROTO_TCP)	/* XXX extend for UDP */
		return -1;

	if ((dir ? (void *)skb_dst(skb) : (void *)skb->dev) == NULL) {
		panic(" -- this should not happen\n");
		return -1;
	}

	if (skb->sk) {
		sk = skb->sk;
	} else {
		/*
		 * Try a lookup. On a match, sk has a refcount that we must
		 * release on exit (we know it because skb->sk = NULL).
		 *
		 * inet_lookup above 2.6.24 has an additional 'net' parameter
		 * so we use a macro to conditionally supply it.
		 * swap dst and src depending on the direction.
		 */
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,24)
#define _OPT_NET_ARG
#else
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,26)
/* there is no dev_net() on 2.6.25 */
#define _OPT_NET_ARG (skb->dev->nd_net),
#else	/* 2.6.26 and above */
#define _OPT_NET_ARG dev_net(skb->dev),
#endif
#endif
		sk =  (dir) ? /* dir != 0 on output */
		    inet_lookup(_OPT_NET_ARG &tcp_hashinfo,
			daddr, dport, saddr, sport,	// match outgoing
			inet_iif(skb)) :
		    inet_lookup(_OPT_NET_ARG &tcp_hashinfo,
			saddr, sport, daddr, dport,	// match incoming
			skb->dev->ifindex);
#undef _OPT_NET_ARG

		if (sk == NULL) /* no match, nothing to be done */
			return -1;
	}
	ret = 1;	/* retrying won't make things better */
	st = sk->sk_state;
#ifdef CONFIG_VSERVER
	u->xid = sk->sk_xid;
	u->nid = sk->sk_nid;
#else
	u->xid = u->nid = 0;
#endif
	/*
	 * Exclude tcp states where sk points to a inet_timewait_sock which
	 * has no sk_socket field (surely TCP_TIME_WAIT, perhaps more).
	 * To be safe, use a whitelist and not a blacklist.
	 * Before dereferencing sk_socket grab a lock on sk_callback_lock.
	 *
	 * Once again we need conditional code because the UID and GID
	 * location changes between kernels.
	 */
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,28)
/* use the current's real uid/gid */
#define _CURR_UID f_uid
#define _CURR_GID f_gid
#else /* 2.6.29 and above */
/* use the current's file access real uid/gid */
#define _CURR_UID f_cred->fsuid
#define _CURR_GID f_cred->fsgid
#endif

#define GOOD_STATES (	\
	(1<<TCP_LISTEN) | (1<<TCP_SYN_RECV)   | (1<<TCP_SYN_SENT)   | \
	(1<<TCP_ESTABLISHED)  | (1<<TCP_FIN_WAIT1) | (1<<TCP_FIN_WAIT2) )
	// surely exclude TCP_CLOSE, TCP_TIME_WAIT, TCP_LAST_ACK
	// uncertain TCP_CLOSE_WAIT and TCP_CLOSING

	if ((1<<st) & GOOD_STATES) {
		read_lock_bh(&sk->sk_callback_lock);
		if (sk->sk_socket && sk->sk_socket->file) {
			u->uid = sk->sk_socket->file->_CURR_UID;
			u->gid = sk->sk_socket->file->_CURR_GID;
		}
		read_unlock_bh(&sk->sk_callback_lock);
	} else {
		u->uid = u->gid = 0;
	}
	if (!skb->sk) /* return the reference that came from the lookup */
		sock_put(sk);
#undef GOOD_STATES
#undef _CURR_UID
#undef _CURR_GID
	return ret;

#endif /* LINUX > 2.4 */
}

/*
 * Now prepare to hook the various functions.
 * Linux 2.4 has a different API so we need some adaptation
 * for register and unregister hooks
 *
 * the unregister function changed arguments between 2.6.22 and 2.6.24
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,14)
struct nf_queue_handler ipfw2_queue_handler_desc = {
        .outfn = ipfw2_queue_handler,
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,8,2)
        .name = "ipfw2 dummynet queue",
#endif
};
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,8,2)
#define REG_QH_ARG(pf, fn)	pf, &(fn ## _desc)
#else
#define REG_QH_ARG(pf, fn)	&(fn ## _desc)
#endif
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,17) /* XXX was 2.6.0 */
static int
nf_register_hooks(struct nf_hook_ops *ops, int n)
{
	int i, ret = 0;
	for (i = 0; i < n; i++) {
		ret = nf_register_hook(ops + i);
		if (ret < 0)
			break;
	}
	return ret;
}

static void
nf_unregister_hooks(struct nf_hook_ops *ops, int n)
{
	int i;
	for (i = 0; i < n; i++) {
		nf_unregister_hook(ops + i);
	}
}
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,14) /* XXX was 2.6.0 */
#define REG_QH_ARG(pf, fn)	pf, fn, NULL
#endif
#define UNREG_QH_ARG(pf, fn) //fn	/* argument for nf_[un]register_queue_handler */
#define SET_MOD_OWNER

#else /* linux > 2.6.17 */

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24)
#define UNREG_QH_ARG(pf, fn) //fn
#elif LINUX_VERSION_CODE < KERNEL_VERSION(3,8,2)
#define UNREG_QH_ARG(pf, fn)	pf, &(fn ## _desc)
#else
#define UNREG_QH_ARG(pf, fn)
#endif /* 2.6.0 < LINUX > 2.6.24 */

#define SET_MOD_OWNER	.owner = THIS_MODULE,

#endif	/* !LINUX < 2.6.0 */

static struct nf_hook_ops ipfw_ops[] __read_mostly = {
        {
                .hook           = call_ipfw,
                .pf             = PF_INET,
                .hooknum        = IPFW_HOOK_IN,
                .priority       = NF_IP_PRI_FILTER,
                SET_MOD_OWNER
        },
        {
                .hook           = call_ipfw,
                .pf             = PF_INET,
                .hooknum        = NF_IP_POST_ROUTING,
                .priority       = NF_IP_PRI_FILTER,
		SET_MOD_OWNER
        },
};
#endif /* __linux__ */

/* descriptors for the children, until i find a way for the
 * linker to produce them
 */
extern moduledata_t *moddesc_ipfw;
extern moduledata_t *moddesc_dummynet;
extern moduledata_t *moddesc_dn_fifo;
extern moduledata_t *moddesc_dn_wf2qp;
extern moduledata_t *moddesc_dn_rr;
extern moduledata_t *moddesc_dn_qfq;
extern moduledata_t *moddesc_dn_prio;
extern void *sysinit_ipfw_init;
extern void *sysuninit_ipfw_destroy;
extern void *sysinit_vnet_ipfw_init;
extern void *sysuninit_vnet_ipfw_uninit;

/*
 * Module glue - init and exit function.
 */
int __init
ipfw_module_init(void)
{
	int ret = 0;
#ifdef _WIN32
	unsigned long resolution;
#endif

	rn_init(64);
	my_mod_register("ipfw",  1, moddesc_ipfw, NULL, NULL);
	my_mod_register("sy_ipfw",  2, NULL,
		sysinit_ipfw_init, sysuninit_ipfw_destroy);
	my_mod_register("sy_Vnet_ipfw",  3, NULL,
		sysinit_vnet_ipfw_init, sysuninit_vnet_ipfw_uninit);
	my_mod_register("dummynet",  4, moddesc_dummynet, NULL, NULL);
	my_mod_register("dn_fifo",  5, moddesc_dn_fifo, NULL, NULL);
	my_mod_register("dn_wf2qp",  6, moddesc_dn_wf2qp, NULL, NULL);
	my_mod_register("dn_rr",  7, moddesc_dn_rr, NULL, NULL);
	my_mod_register("dn_qfq",  8, moddesc_dn_qfq, NULL, NULL);
	my_mod_register("dn_prio",  9, moddesc_dn_prio, NULL, NULL);
	init_children();

#ifdef _WIN32
	resolution = ExSetTimerResolution(1, TRUE);
	printf("*** ExSetTimerResolution: resolution set to %d n-sec ***\n",resolution);
#endif
#ifdef EMULATE_SYSCTL
	keinit_GST();
#endif 

#ifdef __linux__
	/* sockopt register, in order to talk with user space */
	ret = nf_register_sockopt(&ipfw_sockopts);
        if (ret < 0) {
		printf("error %d in nf_register_sockopt\n", ret);
		goto clean_modules;
	}

	/* queue handler registration, in order to get network
	 * packet under a private queue */
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,8,2)
	ret =
#endif
	    nf_register_queue_handler(REG_QH_ARG(PF_INET, ipfw2_queue_handler) );
        if (ret < 0)	/* queue busy */
		goto unregister_sockopt;

        ret = nf_register_hooks(ipfw_ops, ARRAY_SIZE(ipfw_ops));
        if (ret < 0)
		goto unregister_sockopt;

	printf("%s loaded\n", __FUNCTION__);
	return 0;


/* handle errors on load */
unregister_sockopt:
	nf_unregister_queue_handler(UNREG_QH_ARG(PF_INET, ipfw2_queue_handler) );
	nf_unregister_sockopt(&ipfw_sockopts);

clean_modules:
	fini_children();
	printf("%s error\n", __FUNCTION__);

#endif	/* __linux__ */
	return ret;
}

/* module shutdown */
void __exit
ipfw_module_exit(void)
{
#ifdef EMULATE_SYSCTL
	keexit_GST();
#endif
#ifdef _WIN32
	ExSetTimerResolution(0,FALSE);

#else  /* linux hook */
        nf_unregister_hooks(ipfw_ops, ARRAY_SIZE(ipfw_ops));
	/* maybe drain the queue before unregistering ? */
	nf_unregister_queue_handler(UNREG_QH_ARG(PF_INET, ipfw2_queue_handler) );
	nf_unregister_sockopt(&ipfw_sockopts);
#endif	/* __linux__ */

	fini_children();

	printf("%s unloaded\n", __FUNCTION__);
}

#ifdef __linux__
module_init(ipfw_module_init)
module_exit(ipfw_module_exit)
MODULE_LICENSE("Dual BSD/GPL"); /* the code here is all BSD. */
#endif
