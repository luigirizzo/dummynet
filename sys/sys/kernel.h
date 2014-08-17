/*
 * from freebsd's kernel.h
 */
#ifndef _SYS_KERNEL_H_
#define _SYS_KERNEL_H_

#define SYSINIT(a, b, c, d, e)  \
        void *sysinit_ ## d = d
#define VNET_SYSINIT(a, b, c, d, e)  \
        void *sysinit_ ## d = d
#define SYSUNINIT(a, b, c, d, e)  \
        void *sysuninit_ ## d = d
#define VNET_SYSUNINIT(a, b, c, d, e)  \
        void *sysuninit_ ## d = d

/*
 * Some enumerated orders; "ANY" sorts last.
 */
enum sysinit_elem_order {
        SI_ORDER_FIRST          = 0x0000000,    /* first*/
        SI_ORDER_SECOND         = 0x0000001,    /* second*/
        SI_ORDER_THIRD          = 0x0000002,    /* third*/
        SI_ORDER_MIDDLE         = 0x1000000,    /* somewhere in the middle */
        SI_ORDER_ANY            = 0xfffffff     /* last*/
};
#endif
