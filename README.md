# Build
This builds on all RHEL7 / CentOS 7 releases.
Make sure the linux kernel-headers package is installed.
   
```
 $ make all
```


##  Operation 

### Load the kernel module
```
# insmod ./dummynet2/ipfw_mod.ko
```

### Add some ipfw rules
```
# ipfw -q flush
# ipfw -q pipe flush
# ipfw add pipe 3 in src-port 80,443,8080
# ipfw pipe 3 config delay 80ms bw 40Mbit/s plr 0.01 queue 1000Kbytes
```


### Unload the kernel module
```
# rmmod ipfw_mod.ko
```


# Authors

This directory contains a port of ipfw and dummynet to Linux and Windows.
This version of ipfw and dummynet is called "ipfw3" as it is the
third major rewrite of the code.  The source code here comes straight
from FreeBSD (roughly the version in HEAD as of February 2010),
plus some glue code and headers written from scratch.  Unless
specified otherwise, all the code here is under a BSD license.

The build will produce

    a kernel module,    ipfw_mod.ko 
    a userland program, /sbin/ipfw 


## CREDITS:
- Luigi Rizzo (main design and development)
- Marta Carbone (Linux and Planetlab ports)
- Riccardo Panicucci (modular scheduler support)
- Francesco Magno (Windows port)
- Fabio Checconi (the QFQ scheduler)
- Funding from Universita` di Pisa (NETOS project),
- European Commission (ONELAB2 project)
- ACM SIGCOMM (Sigcomm Community Projects Award, April 2012)


