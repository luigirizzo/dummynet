#ifndef _SYS_SYSTM_H_
#define _SYS_SYSTM_H_

#define CALLOUT_ACTIVE          0x0002 /* callout is currently active */
#define CALLOUT_MPSAFE          0x0008 /* callout handler is mp safe */

#ifndef _WIN32	/* this is the linux version */
/* callout support, in <sys/callout.h> on FreeBSD */
/*
 * callout support on linux module is done using timers
 */
#include <linux/timer.h>
#ifdef LINUX_24
#include <linux/sched.h>        /* jiffies definition is here in 2.4 */
#endif
#define callout timer_list
static __inline int
callout_reset_on(struct callout *co, int ticks, void (*fn)(void *), void *arg, int cpu)
{
        co->expires = jiffies + ticks;
        co->function = (void (*)(unsigned long))fn;
        co->data = (unsigned long)arg;
	/*
	 * Linux 2.6.31 and above has add_timer_on(co, cpu),
	 * otherwise add_timer() always schedules a callout on the same
	 * CPU used the first time, so we don't need more.
	 */
        add_timer(co);
        return 0;
}

#define callout_init(co, safe)  init_timer(co)
#define callout_drain(co)       del_timer(co)
#define callout_stop(co)        del_timer(co)

#else /* _WIN32 */
#include <ndis.h>

/* This is the windows part for callout support */
struct callout {
	KTIMER thetimer;
	KDPC timerdpc;
	int dpcinitialized;
	LARGE_INTEGER duetime;
};

void dummynet (void*);
VOID dummynet_dpc(
    __in struct _KDPC  *Dpc,
    __in_opt PVOID  DeferredContext,
    __in_opt PVOID  SystemArgument1,
    __in_opt PVOID  SystemArgument2
    );

VOID ipfw_dpc(
    __in struct _KDPC  *Dpc,
    __in_opt PVOID  DeferredContext,
    __in_opt PVOID  SystemArgument1,
    __in_opt PVOID  SystemArgument2
    );

/* callout_reset must handle two problems:
 * - dummynet() scheduler must be run always on the same processor
 * because do_gettimeofday() is based on cpu performance counter, and
 * _occasionally_ can leap backward in time if we query another cpu.
 * typically this won't happen that much, and the cpu will almost always
 * be the same even without the affinity restriction, but better to be sure.
 * - ipfw_tick() does not have the granularity requirements of dummynet()
 * but we need to pass a pointer as argument.
 *
 * for these reasons, if we are called for dummynet() timer,
 * KeInitializeDpc is called only once as it should be, and the thread
 * is forced on cpu0 (which is always present), while if we're called
 * for ipfw_tick(), we re-initialize the DPC each time, using
 * parameter DeferredContext to pass the needed pointer. since this
 * timer is called only once a sec, this won't hurt that much.
 */
static __inline int
callout_reset_on(struct callout *co, int ticks, void (*fn)(void *), void *arg, int cpu) 
{
	if(fn == &dummynet)
	{
		if(co->dpcinitialized == 0)
		{
			KeInitializeDpc(&co->timerdpc, dummynet_dpc, NULL);
			KeSetTargetProcessorDpc(&co->timerdpc, cpu);
			co->dpcinitialized = 1;
		}
	}
	else
	{
		KeInitializeDpc(&co->timerdpc, ipfw_dpc, arg);
	}
	co->duetime.QuadPart = (-ticks)*10000;
	KeSetTimer(&co->thetimer, co->duetime, &co->timerdpc);
	return 0;
}

static __inline void
callout_init(struct callout* co, int safe)
{
	printf("%s: initializing timer at %p\n",__FUNCTION__,co);
	KeInitializeTimer(&co->thetimer);
}

static __inline int
callout_drain(struct callout* co)
{
	BOOLEAN canceled = KeCancelTimer(&co->thetimer);
	while (canceled != TRUE)
	{
		canceled = KeCancelTimer(&co->thetimer);
	}
	printf("%s: stopping timer at %p\n",__FUNCTION__,co);
	return 0;
}

static __inline int
callout_stop(struct callout* co)
{
	return callout_drain(co);
}

#endif /* _WIN32 */

#endif /* _SYS_SYSTM_H_ */
