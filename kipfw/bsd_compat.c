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
 * $Id: bsd_compat.c 11530 2012-08-01 10:29:32Z luigi $
 *
 * kernel variables and functions that are not available in linux.
 */

#include <sys/cdefs.h>
#include <asm/div64.h>	/* do_div on 2.4 */
#include <linux/random.h>	/* get_random_bytes on 2.4 */
#include <netinet/ip_fw.h>
#include <netinet/ip_dummynet.h>
#include <sys/malloc.h>

/*
 * gettimeofday would be in sys/time.h but it is not
 * visible if _KERNEL is defined
 */
int gettimeofday(struct timeval *, struct timezone *);

int ticks;		/* kernel ticks counter */
int hz = 1000;		/* default clock time */
long tick = 1000;	/* XXX is this 100000/hz ? */
int bootverbose = 0;
struct timeval boottime;

int     ip_defttl = 64;	/* XXX set default value */
int	max_linkhdr = 16;
int fw_one_pass = 1;
u_long  in_ifaddrhmask;                         /* mask for hash table */
struct  in_ifaddrhashhead *in_ifaddrhashtbl;    /* inet addr hash table  */

u_int rt_numfibs = RT_NUMFIBS;

/*
 * pfil hook support.
 * We make pfil_head_get return a non-null pointer, which is then ignored
 * in our 'add-hook' routines.
 */
struct pfil_head;
typedef int (pfil_hook_t)
	(void *, struct mbuf **, struct ifnet *, int, struct inpcb *);

struct pfil_head *
pfil_head_get(int proto, u_long flags)
{
	static int dummy;
	return (struct pfil_head *)&dummy;
}
 
int
pfil_add_hook(pfil_hook_t *func, void *arg, int dir, struct pfil_head *h)
{
	return 0;
}

int
pfil_remove_hook(pfil_hook_t *func, void *arg, int dir, struct pfil_head *h)
{
	return 0;
}

/* define empty body for kernel function */
int
priv_check(struct thread *td, int priv)
{
	return 0;
}

int
securelevel_ge(struct ucred *cr, int level)
{
	return 0;
}

int
sysctl_handle_int(SYSCTL_HANDLER_ARGS)
{
	return 0;
}

int
sysctl_handle_long(SYSCTL_HANDLER_ARGS)
{
	return 0;
}

void
ether_demux(struct ifnet *ifp, struct mbuf *m)
{
	return;
}

int
ether_output_frame(struct ifnet *ifp, struct mbuf *m)
{
	return 0;
}

void
in_rtalloc_ign(struct route *ro, u_long ignflags, u_int fibnum)
{
	return;
}

void
icmp_error(struct mbuf *n, int type, int code, uint32_t dest, int mtu)
{
	return;
}

u_short
in_cksum_skip(struct mbuf *m, int len, int skip)
{
	return 0;
}

u_short
in_cksum_hdr(struct ip *ip)
{
	return 0;
}

/*
 * we don't really reassemble, just return whatever we had.
 */
struct mbuf *
ip_reass(struct mbuf *clone)
{
	return clone;
}
#ifdef INP_LOCK_ASSERT
#undef INP_LOCK_ASSERT
#define INP_LOCK_ASSERT(a)
#endif

/* credentials check */
#include <netinet/ip_fw.h>
#ifdef __linux__
int
cred_check(void *_insn,  int proto, struct ifnet *oif,
    struct in_addr dst_ip, u_int16_t dst_port, struct in_addr src_ip,
    u_int16_t src_port, struct bsd_ucred *u, int *ugid_lookupp,
    struct sk_buff *skb)
{
	int match = 0;
	ipfw_insn_u32 *insn = (ipfw_insn_u32 *)_insn;

	if (*ugid_lookupp == 0) {        /* actively lookup and copy in cache */
		/* returns null if any element of the chain up to file is null.
		 * if sk != NULL then we also have a reference
		 */
		*ugid_lookupp = linux_lookup(proto,
			src_ip.s_addr, htons(src_port),
			dst_ip.s_addr, htons(dst_port),
			skb, oif ? 1 : 0, u);
	}
	if (*ugid_lookupp < 0)
		return 0;

	if (insn->o.opcode == O_UID)
		match = (u->uid == (uid_t)insn->d[0]);
	else if (insn->o.opcode == O_JAIL)
		match = (u->xid == (uid_t)insn->d[0]);
	else if (insn->o.opcode == O_GID)
		match = (u->gid == (uid_t)insn->d[0]);
	return match;
}
#endif	/* __linux__ */

int
jailed(struct ucred *cred)
{
	return 0;
}

/*
* Return 1 if an internet address is for a ``local'' host
* (one to which we have a connection).  If subnetsarelocal
* is true, this includes other subnets of the local net.
* Otherwise, it includes only the directly-connected (sub)nets.
*/
int
in_localaddr(struct in_addr in)
{
	return 1;
}

int
sooptcopyout(struct sockopt *sopt, const void *buf, size_t len)
{
	size_t valsize = sopt->sopt_valsize;

	if (len < valsize)
		sopt->sopt_valsize = valsize = len;
	//printf("copyout buf = %p, sopt = %p, soptval = %p, len = %d \n", buf, sopt, sopt->sopt_val, len);
	bcopy(buf, sopt->sopt_val, valsize);
	return 0;
}

/*
 * copy data from userland to kernel
 */
int
sooptcopyin(struct sockopt *sopt, void *buf, size_t len, size_t minlen)
{
	size_t valsize = sopt->sopt_valsize;

	if (valsize < minlen)
		return EINVAL;
	if (valsize > len)
		sopt->sopt_valsize = valsize = len;
	//printf("copyin buf = %p, sopt = %p, soptval = %p, len = %d \n", buf, sopt, sopt->sopt_val, len);
	bcopy(sopt->sopt_val, buf, valsize);
	return 0;
}

void
getmicrouptime(struct timeval *tv)
{
	do_gettimeofday(tv);
}


#include <arpa/inet.h>

char *
inet_ntoa_r(struct in_addr ina, char *buf)
{
#ifdef _WIN32
#else
	unsigned char *ucp = (unsigned char *)&ina;

	sprintf(buf, "%d.%d.%d.%d",
	ucp[0] & 0xff,
	ucp[1] & 0xff,
	ucp[2] & 0xff,
	ucp[3] & 0xff);
#endif
	return buf;
}

char *
inet_ntoa(struct in_addr ina)
{
	static char buf[16];
	return inet_ntoa_r(ina, buf);
}

int
random(void)
{
#ifdef _WIN32
	static unsigned long seed;
	if (seed == 0) {
		LARGE_INTEGER tm;
		KeQuerySystemTime(&tm);
		seed = tm.LowPart;
	}
	return RtlRandomEx(&seed) & 0x7fffffff;
#else
	int r;
	get_random_bytes(&r, sizeof(r));
	return r & 0x7fffffff; 
#endif
}


/*
 * do_div really does a u64 / u32 bit division.
 * we save the sign and convert to uint befor calling.
 * We are safe just because we always call it with small operands.
 */
int64_t
div64(int64_t a, int64_t b)
{
#ifdef _WIN32
        int a1 = a, b1 = b;
	return a1/b1;
#else
	uint64_t ua, ub;
	int sign = ((a>0)?1:-1) * ((b>0)?1:-1);

	ua = ((a>0)?a:-a);
	ub = ((b>0)?b:-b);
        do_div(ua, ub);
	return sign*ua;
#endif
}

#ifdef __MIPSEL__
size_t
strlcpy(char *dst, const char *src, size_t siz)
{
        char *d = dst;
        const char *s = src;
        size_t n = siz;
 
        /* Copy as many bytes as will fit */
        if (n != 0 && --n != 0) {
                do {
                        if ((*d++ = *s++) == 0)
                                break;
                } while (--n != 0);
        }

        /* Not enough room in dst, add NUL and traverse rest of src */
        if (n == 0) {
                if (siz != 0)
                        *d = '\0';              /* NUL-terminate dst */
                while (*s++)
                        ;
        }

        return(s - src - 1);    /* count does not include NUL */
}
#endif // __MIPSEL__

/*
 * compact version of fnmatch.
 */
int
fnmatch(const char *pattern, const char *string, int flags)
{
	char s;

	if (!string || !pattern)
		return 1;	/* no match */
	while ( (s = *string++) ) {
		char p = *pattern++;
		if (p == '\0')		/* pattern is over, no match */
			return 1;
		if (p == '*')		/* wildcard, match */
			return 0;
		if (p == '.' || p == s)	/* char match, continue */
			continue;
		return 1;		/* no match */
	}
	/* end of string, make sure the pattern is over too */
	if (*pattern == '\0' || *pattern == '*')
		return 0;
	return 1;	/* no match */
}


/*
 * linux 2.6.33 defines these functions to access to
 * skbuff internal structures. Define the missing
 * function for the previous versions too.
 */
#ifdef linux
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,31)
inline void skb_dst_set(struct sk_buff *skb, struct dst_entry *dst)
{
        skb->dst = dst;
}

inline struct dst_entry *skb_dst(const struct sk_buff *skb)
{
        return (struct dst_entry *)skb->dst;
}
#endif /* < 2.6.31 */
#endif /* linux */


/* support for sysctl emulation.
 * XXX this is actually MI code that should be enabled also on openwrt
 */
#ifdef EMULATE_SYSCTL
static struct sysctltable GST;

int
kesysctl_emu_get(struct sockopt* sopt)
{
	struct dn_id* oid = sopt->sopt_val;
	struct sysctlhead* entry;
	int sizeneeded = sizeof(struct dn_id) + GST.totalsize +
		sizeof(struct sysctlhead);
	unsigned char* pstring;
	unsigned char* pdata;
	int i;
	
	if (sopt->sopt_valsize < sizeneeded) {
		// this is a probe to retrieve the space needed for
		// a dump of the sysctl table
		oid->id = sizeneeded;
		sopt->sopt_valsize = sizeof(struct dn_id);
		return 0;
	}
	
	entry = (struct sysctlhead*)(oid+1);
	for( i=0; i<GST.count; i++) {
		entry->blocklen = GST.entry[i].head.blocklen;
		entry->namelen = GST.entry[i].head.namelen;
		entry->flags = GST.entry[i].head.flags;
		entry->datalen = GST.entry[i].head.datalen;
		pdata = (unsigned char*)(entry+1);
		pstring = pdata+GST.entry[i].head.datalen;
		bcopy(GST.entry[i].data, pdata, GST.entry[i].head.datalen);
		bcopy(GST.entry[i].name, pstring, GST.entry[i].head.namelen);
		entry = (struct sysctlhead*)
			((unsigned char*)(entry) + GST.entry[i].head.blocklen);
	}
	sopt->sopt_valsize = sizeneeded;
	return 0;
}

int
kesysctl_emu_set(void* p, int l)
{
	struct sysctlhead* entry;
	unsigned char* pdata;
	unsigned char* pstring;
	int i = 0;
	
	entry = (struct sysctlhead*)(((struct dn_id*)p)+1);
	pdata = (unsigned char*)(entry+1);
	pstring = pdata + entry->datalen;
	
	for (i=0; i<GST.count; i++) {
		if (strcmp(GST.entry[i].name, pstring) != 0)
			continue;
		printf("%s: match found! %s\n",__FUNCTION__,pstring);
		//sanity check on len, not really useful now since
		//we only accept int32
		if (entry->datalen != GST.entry[i].head.datalen) {
			printf("%s: len mismatch, user %d vs kernel %d\n",
				__FUNCTION__, entry->datalen,
				GST.entry[i].head.datalen);
			return -1;
		}
		// check access (at the moment flags handles only the R/W rights
		//later on will be type + access
		if( (GST.entry[i].head.flags & 3) == CTLFLAG_RD) {
			printf("%s: the entry %s is read only\n",
				__FUNCTION__,GST.entry[i].name);
			return -1;
		}
		bcopy(pdata, GST.entry[i].data, GST.entry[i].head.datalen);
		return 0;
	}
	printf("%s: match not found\n",__FUNCTION__);
	return 0;
}

/* convert all _ to . until the first . */
static void
underscoretopoint(char* s)
{
	for (; *s && *s != '.'; s++)
		if (*s == '_')
			*s = '.';
}

static int
formatnames()
{
	int i;
	int size=0;
	char* name;

	for (i=0; i<GST.count; i++)
		size += GST.entry[i].head.namelen;
	GST.namebuffer = malloc(size, 0, 0);
	if (GST.namebuffer == NULL)
		return -1;
	name = GST.namebuffer;
	for (i=0; i<GST.count; i++) {
		bcopy(GST.entry[i].name, name, GST.entry[i].head.namelen);
		underscoretopoint(name);
		GST.entry[i].name = name;
		name += GST.entry[i].head.namelen;
	}
	return 0;
}

static void
dumpGST()
{
	int i;

	for (i=0; i<GST.count; i++) {
		printf("SYSCTL: entry %i\n", i);
		printf("name %s\n", GST.entry[i].name);
		printf("namelen %i\n", GST.entry[i].head.namelen);
		printf("type %i access %i\n",
			GST.entry[i].head.flags >> 2,
			GST.entry[i].head.flags & 0x00000003);
		printf("data %i\n", *(int*)(GST.entry[i].data));
		printf("datalen %i\n", GST.entry[i].head.datalen);
		printf("blocklen %i\n", GST.entry[i].head.blocklen);
	}
}

void sysctl_addgroup_f1();
void sysctl_addgroup_f2();
void sysctl_addgroup_f3();
void sysctl_addgroup_f4();

void
keinit_GST()
{
	int ret;

	sysctl_addgroup_f1();
	sysctl_addgroup_f2();
	sysctl_addgroup_f3();
	sysctl_addgroup_f4();
	ret = formatnames();
	if (ret != 0)
		printf("conversion of names failed for some reason\n");
	//dumpGST();
	printf("*** Global Sysctl Table entries = %i, total size = %i ***\n",
		GST.count, GST.totalsize);
}

void
keexit_GST()
{
	if (GST.namebuffer != NULL)
		free(GST.namebuffer,0);
	bzero(&GST, sizeof(GST));
}

void
sysctl_pushback(char* name, int flags, int datalen, void* data)
{
	if (GST.count >= GST_HARD_LIMIT) {
		printf("WARNING: global sysctl table full, this entry will not be added,"
				"please recompile the module increasing the table size\n");
		return;
	}
	GST.entry[GST.count].head.namelen = strlen(name)+1; //add space for '\0'
	GST.entry[GST.count].name = name;
	GST.entry[GST.count].head.flags = flags;
	GST.entry[GST.count].data = data;
	GST.entry[GST.count].head.datalen = datalen;
	GST.entry[GST.count].head.blocklen =
		((sizeof(struct sysctlhead) + GST.entry[GST.count].head.namelen +
			GST.entry[GST.count].head.datalen)+3) & ~3;
	GST.totalsize += GST.entry[GST.count].head.blocklen;
	GST.count++;
}
#endif /* EMULATE_SYSCTL */
