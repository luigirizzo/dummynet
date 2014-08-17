/*
 * additional define not present in linux
 * should go in glue.h
 */
#ifndef _NETINET_IP_ICMP_H_
#define _NETINET_IP_ICMP_H_

#define ICMP_MAXTYPE            40      /* defined as 18 in compat.h */
#define ICMP_ROUTERSOLICIT      10              /* router solicitation */
#define ICMP_TSTAMP             13              /* timestamp request */
#define ICMP_IREQ               15              /* information request */
#define ICMP_MASKREQ            17              /* address mask request */
#define         ICMP_UNREACH_HOST       1               /* bad host */

#define ICMP_UNREACH            3               /* dest unreachable, codes: */

#endif /* _NETINET_IP_ICMP_H_ */
