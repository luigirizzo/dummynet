#!/bin/sh
# this script execute N times the command CMD
# collecting the memory usage on a file.
# The value of the Dirty memory should not increase
# between tests.

BASE_NAME=ipfw_r5808_
N=10000
CMD1="/sbin/insmod ../dummynet2/ipfw_mod.ko"
CMD2="/sbin/rmmod ipfw_mod"

# main
# remove any previous loaded module
/sbin/rmmod ipfw_mod 

# pre

for n in `seq $N`; do
	$CMD1
	$CMD2
	[ $n = 10 ] && cat /proc/meminfo > /tmp/${BASE_NAME}_${n}
	[ $n = 100 ] && cat /proc/meminfo > /tmp/${BASE_NAME}_${n}
	[ $n = 1000 ] && cat /proc/meminfo > /tmp/${BASE_NAME}_${n}
done;

# post
