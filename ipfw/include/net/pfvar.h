#ifndef _PF_VAR_H_
#define _PF_VAR_H_

/*
 * replacement for FreeBSD's pfqueue.h
 */
#include <sys/queue.h>

#define DIOCSTARTALTQ   _IO  ('D', 42)
#define DIOCSTOPALTQ    _IO  ('D', 43)

struct pf_altq {
	TAILQ_ENTRY(pf_altq)     entries;
	/* ... */
        u_int32_t                qid;           /* return value */

#define PF_QNAME_SIZE            64
        char                     qname[PF_QNAME_SIZE];  /* queue name */

};

struct pfioc_altq {
        u_int32_t        action;
        u_int32_t        ticket;
        u_int32_t        nr;
        struct pf_altq   altq;
};

#define DIOCGETALTQS    _IOWR('D', 47, struct pfioc_altq)
#define DIOCGETALTQ    _IOWR('D', 48, struct pfioc_altq)

#endif /* !_PF_VAR_H */
