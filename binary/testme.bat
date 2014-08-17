@echo on
@set CYGWIN=nodosfilewarning

@ipfw -q flush
@ipfw -q pipe flush
@echo ######################################################################
@echo ## Setting delay to 100ms for both incoming and outgoing ip packets ##
@echo ## and sending 4 echo request to Google                             ##
@echo ######################################################################
ipfw pipe 3 config delay 100ms
ipfw add pipe 3 ip from any to any
ipfw pipe show
ping -n 4 www.google.it

@echo ##############################################
@echo ## Raising delay to 300ms and pinging again ##
@echo ##############################################
ipfw pipe 3 config delay 300ms
ipfw pipe show
ping -n 4 www.google.com

@echo ##################################
@echo ## Shaping bandwidth to 500kbps ##
@echo ##################################
ipfw pipe 3 config bw 500Kbit/s
ipfw pipe show
wget http://info.iet.unipi.it/~luigi/1m
@del 1m

@echo ###################################
@echo ## Lowering bandwidth to 250kbps ##
@echo ###################################
ipfw pipe 3 config bw 250Kbit/s
ipfw pipe show
wget http://info.iet.unipi.it/~luigi/1m
@del 1m

@echo ###################################################################
@echo ## Simulating 50 percent packet loss and sending 15 echo request ##
@echo ###################################################################
@ipfw -q flush
@ipfw -q pipe flush
ipfw add prob 0.5 deny proto icmp in
ping -n 15 -w 300 www.google.it
@ipfw -q flush

@echo ##############################
@echo ## Showing SYSCTL variables ##
@echo ##############################
ipfw sysctl -a

@echo #############################################
@echo ## Inserting rules to test command parsing ##
@echo #############################################
@echo -- dropping all packets of a specific protocol --
ipfw add deny proto icmp
@echo -- dropping packets of all protocols except a specific one --
ipfw add deny not proto tcp
@echo -- dropping all packets from IP x to IP y --
ipfw add deny src-ip 1.2.3.4 dst-ip 5.6.7.8
@echo -- dropping all ssh outgoing connections --
ipfw add deny out dst-port 22
@echo -- allowing already opened browser connections --
@echo -- but preventing new ones from being opened   --
ipfw add deny out proto tcp dst-port 80 tcpflags syn
@echo -- another way to do the same thing --
ipfw add allow out proto tcp dst-port 80 established
ipfw add deny out proto tcp dst-port 80 setup
@echo -- checking what rules have been inserted --
ipfw -c show
@ipfw -q flush

@echo #################
@echo ## Cleaning up ##
@echo #################
ipfw -q flush
ipfw -q pipe flush

pause
