#ifndef __LINUX_NETFILTER_VERSION_H
#define __LINUX_NETFILTER_VERSION_H

#include "linux_kernel_version.h"

#if IS_RHEL
#if (RHEL_VERSION_LT(7,0))
    #define NF_HOOK_PARAMLIST \
        (\
                 unsigned int hooknum, \
                 struct sk_buff *skb, \
                 const struct net_device *in, \
                 const struct net_device *out, \
                 int (*okfn)(struct sk_buff *) \
             )
    #define NFINFO_RECVIF info->indev
    #define NFINFO_OUTIF info->outdev
    #define NFINFO_HOOK info->hook

#elif (RHEL_VERSION_LT(7,2))
        #define NF_HOOK_PARAMLIST \
            (\
                     const struct nf_hook_ops *ops, \
                     struct sk_buff *skb, \
                     const struct net_device *in, \
                     const struct net_device *out, \
                     int (*okfn)(struct sk_buff *) \
                 )
    #define NFINFO_RECVIF info->indev
    #define NFINFO_OUTIF info->outdev
    #define NFINFO_HOOK info->hook

#else // RHEL 7.2 & 7.3 (might need update to account for possible changes in new releases)
        #define NF_HOOK_PARAMLIST \
            (\
                     const struct nf_hook_ops *ops, \
                     struct sk_buff *skb, \
                     const struct net_device *in, \
                     const struct net_device *out, \
                     const struct nf_hook_state *state \
                 )
    #define NFINFO_RECVIF info->state.in
    #define NFINFO_OUTIF info->state.out
    #define NFINFO_HOOK info->state.hook

#endif
#elif LINUX_VERSION_CODE < KERNEL_VERSION(3,13,0)
    #define NF_HOOK_PARAMLIST \
        (\
                 unsigned int hooknum, \
                 struct sk_buff *skb, \
                 const struct net_device *in, \
                 const struct net_device *out, \
                 int (*okfn)(struct sk_buff *) \
             )
    #define NFINFO_RECVIF info->indev
    #define NFINFO_OUTIF info->outdev
    #define NFINFO_HOOK info->hook

#elif LINUX_VERSION_CODE < KERNEL_VERSION(4,1,0)
    #define NF_HOOK_PARAMLIST \
        (\
                 const struct nf_hook_ops *ops, \
                 struct sk_buff *skb, \
                 const struct net_device *in, \
                 const struct net_device *out, \
                 int (*okfn)(struct sk_buff *) \
             )
    #define NFINFO_RECVIF info->indev
    #define NFINFO_OUTIF info->outdev
    #define NFINFO_HOOK info->hook

#elif LINUX_VERSION_CODE < KERNEL_VERSION(4,4,0)
    #define NF_HOOK_PARAMLIST \
        (\
                 const struct nf_hook_ops *ops, \
                 struct sk_buff *skb, \
                 const struct nf_hook_state *state \
             )
    #define NFINFO_RECVIF info->state.in
    #define NFINFO_OUTIF info->state.out
    #define NFINFO_HOOK info->state.hook

#else
    #define NF_HOOK_PARAMLIST \
        (\
                 void* priv, \
                 struct sk_buff *skb, \
                 const struct nf_hook_state *state \
             )
    #define NFINFO_RECVIF info->state.in
    #define NFINFO_OUTIF info->state.out
    #define NFINFO_HOOK info->state.hook

#endif

#endif
