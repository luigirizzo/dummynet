#!/bin/sh
#
# 20100507 marta, quick test for dyn rules
# ./ipfw/ipfw -d show |grep \ 80

IPFW_MOD=dummynet2/ipfw_mod.ko
IPFW=ipfw/ipfw

# main
# remove any previous loaded module
/sbin/rmmod ipfw_mod 
/sbin/insmod ${IPFW_MOD}
echo "25" >  /sys/module/ipfw_mod/parameters/dyn_ack_lifetime
${IPFW} add 1 check-state
${IPFW} add 9 allow all from any to any keep-state
${IPFW} add 10 allow all from any to onelab1.iet.unipi.it keep-state

telnet 72.14.234.104 80 


