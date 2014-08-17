#ifndef _SYS_TASKQUEUE_H_
#define _SYS_TASKQUEUE_H_

/*
 * Remap taskqueue to direct calls
 */

#ifdef _WIN32
struct task {
	void (*func)(void*, int);
};
#define taskqueue_enqueue(tq, ta)	(ta)->func(NULL,1)
#define TASK_INIT(a,b,c,d) do { 				\
	(a)->func = (c); } while (0)
#else
struct task {
	void (*func)(void);
};
#define taskqueue_enqueue(tq, ta)	(ta)->func()
#define TASK_INIT(a,b,c,d) do { 				\
	(a)->func = (void (*)(void))c; } while (0)

#endif
#define taskqueue_create_fast(_a, _b, _c, _d)	NULL
#define taskqueue_start_threads(_a, _b, _c, _d)

#define	taskqueue_drain(_a, _b)	/* XXX to be completed */
#define	taskqueue_free(_a)	/* XXX to be completed */

#define PRI_MIN                 (0)             /* Highest priority. */
#define PRI_MIN_ITHD            (PRI_MIN)
#define PI_NET                  (PRI_MIN_ITHD + 16)

#endif /* !_SYS_TASKQUEUE_H_ */
