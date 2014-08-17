#!/bin/sh

IPFW=./ipfw/ipfw
PING=/bin/ping
RH=127.0.0.1		# remote host
R=10			# test rule number
P=1			# test pipe number

abort()
{ 
echo $* 
}

#insmod dummynet2/ipfw_mod.ko
#$IPFW show > /dev/null
#$IPFW pipe show 
echo "Flushing rules, do you agree ?"
$IPFW flush

# test_msg rule counter
clean() 
{ 
	$IPFW delete $R 2> /dev/null
	$IPFW pipe $P delete 2> /dev/null
}

# simple counter/allow test
echo -n "counter/allow test..."
clean
$IPFW add $R allow icmp from any to 127.0.0.1 > /dev/null
$PING -f -c100 $RH > /dev/null
counter=`$IPFW show | grep $R | head -n 1 | cut -d " " -f3`
[ ! $counter -eq 400 ] && abort "Wrong counter $counter 400"
echo "...OK"

# simple drop test
echo -n "deny test..."
clean
$IPFW add $R deny icmp from any to 127.0.0.1 > /dev/null
$PING -f -c10 -W 1 $RH > /dev/null
counter=`$IPFW show | grep $R | head -n 1 | cut -d " " -f4`
[ ! $counter -eq 10 ] && abort "Wrong counter $counter 10"
echo "...OK"

# pipe delay test
echo -n "pipe delay test..."
clean
$IPFW pipe $P config delay 2000ms >/dev/null
$IPFW add $R pipe $P icmp from any to $RH >/dev/null
$PING -f -c10 -W 1 $RH > /dev/null
counter1=`$IPFW show | grep $R | head -n 1 | cut -d " " -f4`
sleep 2
counter2=`$IPFW show | grep $R | head -n 1 | cut -d " " -f4`
[ ! $counter1 -eq 10 ] && abort "Wrong counter $counter 10"
[ ! $counter2 -eq 20 ] && abort "Wrong counter $counter 20"
echo "...OK"

# pipe bw test
echo -n "pipe bw test..."
clean
$IPFW pipe $P config bw 2Kbit/s >/dev/null
$IPFW add $R pipe $P icmp from any to $RH >/dev/null
$PING -i 0.1 -c10 -W 1 $RH > /dev/null
counter=`$IPFW show | grep $R | head -n 1 | cut -d " " -f4`
[ $counter -gt 30 ] && abort "Wrong counter $counter should be < 30"
sleep 1
counter=`$IPFW show | grep $R | head -n 1 | cut -d " " -f4`
[ $counter -gt 30 ] && abort "Wrong counter $counter should be < 30"
echo "...OK"

# Final clean
clean
