#/bin/bash

COMMAND=ipfw


echo .########## Set $COMMAND mode .##########
$COMMAND add allow ip from any to any
$COMMAND -q flush

echo .########## empty rules .##########
$COMMAND list
$COMMAND add allow ip from any to any
$COMMAND add allow ip from any to { 1.2.3.4 or 2.3.4.5 }
$COMMAND add allow { dst-ip 1.2.3.4 or dst-ip 2.3.4.5 }

echo .########## listing 3 rules .##########
$COMMAND list

$COMMAND delete 200
echo .########## listing 2 rules .##########
$COMMAND list

$COMMAND table 10 add 1.2.3.4
$COMMAND table 10 add 1.2.3.5
$COMMAND table 10 add 1.2.3.6
$COMMAND table 10 add 1.2.3.7/13
$COMMAND table 10 add 1.2.3.7/20
$COMMAND table 10 add 1.2.3.7/28

echo .########## listing table 10 with 6 elements .##########
$COMMAND table 10 list
$COMMAND table 10 delete 1.2.3.6

echo .########## listing table 10 with 5 elements .##########
$COMMAND table 10 list
$COMMAND table 10 flush

echo .########## table 10 empty .##########
$COMMAND table 10 list

echo .########## move rule 100 to set 1 300 to 3 .##########
$COMMAND set move rule 100 to 1
$COMMAND set move rule 300 to 3
$COMMAND -S show

echo .########## move rule 200 to 2 but 200 do not exist .######
$COMMAND set move rule 200 to 2

echo .########## add some rules .##########
$COMMAND add 200 queue 2 proto ip
$COMMAND add 300 queue 5 proto ip
$COMMAND add 400 queue 40 proto ip
$COMMAND add 400 queue 50 proto ip

echo .########## move rule 200 to 2 .######
$COMMAND set move rule 200 to 2

echo .########## move rule 400 to 5 .######
$COMMAND set move rule 400 to 5

echo .########## set 5 show 2 rules .######
$COMMAND set 5 show

echo .########## flush set 5 .######
$COMMAND -q set 5 flush

echo .########## set 5 show 0 rule .######
$COMMAND set 5 show

echo .########## disable set 1 .######
$COMMAND set disable 1

echo .########## show all rules except set 1 .######
$COMMAND -S show

echo .########## enable set 1 .######
$COMMAND set enable 1

echo .########## show all rules .######
$COMMAND -S show



