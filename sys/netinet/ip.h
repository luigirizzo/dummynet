#ifndef _NETINET_IP_H_
#define _NETINET_IP_H_

#define LITTLE_ENDIAN   1234
#define BIG_ENDIAN      4321
#if defined(__BIG_ENDIAN)
#define BYTE_ORDER      BIG_ENDIAN
//#warning we are in bigendian
#elif defined(__LITTLE_ENDIAN)
//#warning we are in littleendian
#define BYTE_ORDER      LITTLE_ENDIAN
#else
#error no platform
#endif

/* XXX endiannes doesn't belong here */
// #define LITTLE_ENDIAN   1234
// #define BIG_ENDIAN      4321
// #define BYTE_ORDER      LITTLE_ENDIAN

/*
 * Structure of an internet header, naked of options.
 */
struct ip {
#if BYTE_ORDER == LITTLE_ENDIAN
        u_char  ip_hl:4,                /* header length */
                ip_v:4;                 /* version */
#endif
#if BYTE_ORDER == BIG_ENDIAN
        u_char  ip_v:4,                 /* version */
                ip_hl:4;                /* header length */
#endif
        u_char  ip_tos;                 /* type of service */
        u_short ip_len;                 /* total length */
        u_short ip_id;                  /* identification */
        u_short ip_off;                 /* fragment offset field */
#define IP_RF 0x8000                    /* reserved fragment flag */
#define IP_DF 0x4000                    /* dont fragment flag */
#define IP_MF 0x2000                    /* more fragments flag */
#define IP_OFFMASK 0x1fff               /* mask for fragmenting bits */
        u_char  ip_ttl;                 /* time to live */
        u_char  ip_p;                   /* protocol */
        u_short ip_sum;                 /* checksum */
        struct  in_addr ip_src,ip_dst;  /* source and dest address */
} __packed __aligned(4);

#define	IPTOS_LOWDELAY		0x10

#endif /* _NETINET_IP_H_ */
