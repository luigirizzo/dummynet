#ifndef __LINUX_KERNEL_VERSION_H
#define __LINUX_KERNEL_VERSION_H

#include <linux/version.h>
#include <linux/vermagic.h>
#include <linux/stringify.h>

// Macros to deal with OS/kernel versions (RHEL or Ubuntu)

#if defined(RHEL_RELEASE_CODE)
#define IS_RHEL 1
#else
#define IS_RHEL 0
#define RHEL_RELEASE_CODE 0
#define RHEL_RELEASE_VERSION(a,b) (((a) << 8) + (b))
#endif

#define RHEL_VERSION_GT(a,b) RHEL_RELEASE_CODE >  RHEL_RELEASE_VERSION((a),(b))
#define RHEL_VERSION_GE(a,b) RHEL_RELEASE_CODE >=  RHEL_RELEASE_VERSION((a),(b))
#define RHEL_VERSION_LT(a,b) RHEL_RELEASE_CODE <  RHEL_RELEASE_VERSION((a),(b))
#define RHEL_VERSION_LE(a,b) RHEL_RELEASE_CODE <=  RHEL_RELEASE_VERSION((a),(b))
#define RHEL_VERSION_EQ(a,b) RHEL_RELEASE_CODE ==  RHEL_RELEASE_VERSION((a),(b))

#if defined(CONFIG_VERSION_SIGNATURE) || defined(UTS_UBUNTU_RELEASE_ABI)
#define IS_UBUNTU 1
#else
#define IS_UBUNTU 0
#define UTS_UBUNTU_RELEASE_ABI 0
#endif

// a,b,c are kernel version and 'd' is Ubuntu ABI
#define UBUNTU_VERSION_GT(a,b,c,d) LINUX_VERSION_CODE > KERNEL_VERSION((a),(b),(c)) || \
        (LINUX_VERSION_CODE == KERNEL_VERSION((a),(b),(c)) && UTS_UBUNTU_RELEASE_ABI > (d))
#define UBUNTU_VERSION_GE(a,b,c,d) LINUX_VERSION_CODE > KERNEL_VERSION((a),(b),(c)) || \
        (LINUX_VERSION_CODE == KERNEL_VERSION((a),(b),(c)) && UTS_UBUNTU_RELEASE_ABI >= (d))
#define UBUNTU_VERSION_LT(a,b,c,d) LINUX_VERSION_CODE < KERNEL_VERSION((a),(b),(c)) || \
        (LINUX_VERSION_CODE == KERNEL_VERSION((a),(b),(c)) && UTS_UBUNTU_RELEASE_ABI < (d))
#define UBUNTU_VERSION_LE(a,b,c,d) LINUX_VERSION_CODE < KERNEL_VERSION((a),(b),(c)) || \
    (LINUX_VERSION_CODE == KERNEL_VERSION((a),(b),(c)) && UTS_UBUNTU_RELEASE_ABI <= (d))
#define UBUNTU_VERSION_EQ(a,b,c,d) LINUX_VERSION_CODE == KERNEL_VERSION((a),(b),(c)) && UTS_UBUNTU_RELEASE_ABI == (d)

#if IS_RHEL
#define OS_BUILD_VERSION "RH" __stringify(RHEL_MAJOR) "." __stringify(RHEL_MINOR) " (" UTS_RELEASE ")"
#elif IS_UBUNTU
#define OS_BUILD_VERSION "Ubuntu " UTS_RELEASE
#endif


#endif
