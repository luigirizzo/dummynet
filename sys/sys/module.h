/*
 * trivial module support
 */
#ifndef _SYS_MODULE_H_
#define _SYS_MODULE_H_
typedef struct module *module_t;
typedef int (*modeventhand_t)(module_t, int /* modeventtype_t */, void *);
 
typedef enum modeventtype {
        MOD_LOAD,
        MOD_UNLOAD,
        MOD_SHUTDOWN,
        MOD_QUIESCE
} modeventtype_t;
 
typedef struct moduledata {
        const char      *name;          /* module name */
        modeventhand_t  evhand;         /* event handler */
        void            *priv;          /* extra data */
} moduledata_t;

/*
 * Hook the module descriptor, md, into our list of things to do.
 * We should in principle respect the order of loading.
 *
 * XXX use the gcc .init functions
 */
#define DECLARE_MODULE(a, md, c,d)				\
    moduledata_t *moddesc_##a = &md;

/*
 * XXX MODULE_VERSION is define in linux too
 */
#define MODULE_DEPEND(a,b,c,d,e)
#if defined( __linux__ ) || defined( _WIN32 )
#undef MODULE_VERSION
#define MODULE_VERSION(a,b)
#endif

#endif	/* _SYS_MODULE_H_ */

