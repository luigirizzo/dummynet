/*
 * simple override for _long_to_time()
 */
#ifndef _TIMECONV_H_
#define _TIMECONV_H_
static __inline time_t
_long_to_time(long tlong)
{
    if (sizeof(long) == sizeof(__int32_t))
        return((time_t)(__int32_t)(tlong));
    return((time_t)tlong);
}

#endif /* _TIMECONV_H_ */
