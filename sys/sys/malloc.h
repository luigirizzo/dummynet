#ifndef _SYS_MALLOC_H_
#define _SYS_MALLOC_H_

/*
 * No matter what, try to get clear memory and be non-blocking.
 * XXX check if 2.4 has a native way to zero memory,
 * XXX obey to the flags (M_NOWAIT <-> GPF_ATOMIC, M_WAIT <-> GPF_KERNEL)
 */
#ifndef _WIN32 /* this is the linux version */

/*
 * XXX On zeroshell (2.6.25.17) we get a load error
 *	__you_cannot_kmalloc_that_much
 * which is triggered when kmalloc() is called with a large
 * compile-time constant argument (include/linux/slab_def.h)
 *
 * I think it may be a compiler (or source) bug because there is no
 * evidence that such a large request is made.
 * Making the _size argument to kmalloc volatile prevents the compiler
 * from making the mistake, though it is clearly not ideal.
 */

#if !defined (LINUX_24) && LINUX_VERSION_CODE > KERNEL_VERSION(2,6,22)
#define malloc(_size, type, flags)			\
	({ volatile int _v = _size; kmalloc(_v, GFP_ATOMIC | __GFP_ZERO); })
#else /* LINUX <= 2.6.22 and LINUX_24 */
/* linux 2.6.22 does not zero allocated memory */
#define malloc(_size, type, flags)			\
	({ int _s = _size;				\
	void *_ret = kmalloc(_s, GFP_ATOMIC);		\
	if (_ret) memset(_ret, 0, _s);			\
        (_ret);						\
        })
#endif /* LINUX <= 2.6.22 */

#define calloc(_n, _s) malloc((_n * _s), NULL, GFP_ATOMIC | __GFP_ZERO)
#define free(_var, type) kfree(_var)

#else /* _WIN32, the windows version */

/*
 * ntddk.h uses win_malloc() and MmFreeContiguousMemory().
 * wipfw uses
 * ExAllocatePoolWithTag(, pool, len, tag)
 * ExFreePoolWithTag(ptr, tag)
 */
#define malloc(_size, _type, _flags) my_alloc(_size)
#define calloc(_size, _type, _flags) my_alloc(_size)

void *my_alloc(int _size);
/* the 'tag' version does not work without -Gz in the linker */
#define free(_var, type) ExFreePool(_var)
//#define free(_var, type) ExFreePoolWithTag(_var, 'wfpi')

#endif /* _WIN32 */

#define M_NOWAIT        0x0001          /* do not block */
#define M_ZERO          0x0100          /* bzero the allocation */
#endif /* _SYS_MALLOC_H_ */
