#ifndef _NETINET_IP6_H_
#define _NETINET_IP6_H_
#define IN6_ARE_ADDR_EQUAL(a, b)                        \
(memcmp(&(a)->s6_addr[0], &(b)->s6_addr[0], sizeof(struct in6_addr)) == 0)

struct ip6_hdr {
        union {
                struct ip6_hdrctl {
                        u_int32_t ip6_un1_flow; /* 20 bits of flow-ID */  
                        u_int16_t ip6_un1_plen; /* payload length */
                        u_int8_t  ip6_un1_nxt;  /* next header */
                        u_int8_t  ip6_un1_hlim; /* hop limit */
                } ip6_un1;
                u_int8_t ip6_un2_vfc;   /* 4 bits version, top 4 bits class */
        } ip6_ctlun;
        struct in6_addr ip6_src;        /* source address */
        struct in6_addr ip6_dst;        /* destination address */
};
#define ip6_nxt         ip6_ctlun.ip6_un1.ip6_un1_nxt
#define ip6_flow        ip6_ctlun.ip6_un1.ip6_un1_flow


struct icmp6_hdr {
        u_int8_t        icmp6_type;     /* type field */
        u_int8_t        icmp6_code;     /* code field */
        u_int16_t       icmp6_cksum;    /* checksum field */
        union {
                u_int32_t       icmp6_un_data32[1]; /* type-specific field */
                u_int16_t       icmp6_un_data16[2]; /* type-specific field */
                u_int8_t        icmp6_un_data8[4];  /* type-specific field */
        } icmp6_dataun;
};

struct ip6_hbh {
        u_int8_t ip6h_nxt;      /* next header */
        u_int8_t ip6h_len;      /* length in units of 8 octets */
        /* followed by options */
}; 
struct ip6_rthdr {
        u_int8_t  ip6r_nxt;     /* next header */
        u_int8_t  ip6r_len;     /* length in units of 8 octets */
        u_int8_t  ip6r_type;    /* routing type */
        u_int8_t  ip6r_segleft; /* segments left */
        /* followed by routing type specific data */
};
struct ip6_frag {
        u_int8_t  ip6f_nxt;             /* next header */
        u_int8_t  ip6f_reserved;        /* reserved field */
        u_int16_t ip6f_offlg;           /* offset, reserved, and flag */
        u_int32_t ip6f_ident;           /* identification */
};
#define IP6F_OFF_MASK           0xfff8  /* mask out offset from _offlg */
#define IP6F_MORE_FRAG          0x0001  /* more-fragments flag */
struct  ip6_ext {
        u_int8_t ip6e_nxt;
        u_int8_t ip6e_len;
};
#endif /* _NETINET_IP6_H_ */
