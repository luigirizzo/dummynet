#ifndef _CDEFS_H_
#define _CDEFS_H_

/*
 * various compiler macros and common functions
 */

#ifndef __packed
#define __packed       __attribute__ ((__packed__))
#endif

#ifndef __aligned
#define __aligned(x) __attribute__((__aligned__(x)))
#endif

/* defined as assert */
void panic(const char *fmt, ...);

#define KASSERT(exp,msg) do {                                           \
        if (__predict_false(!(exp)))                                    \
                panic msg;                                              \
} while (0)

/* don't bother to optimize */
#ifndef __predict_false
#define __predict_false(x)   (x)	/* __builtin_expect((exp), 0) */
#endif

#endif /* !_CDEFS_H_ */
