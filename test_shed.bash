#!/bin/bash


# This is a test that tries to test all the major functions of
#  the main shed process without needing to run on real hardware.
#  The serial interface is emulated with a set of PTYs.  After we
#  do each operation we dump all of the persistent state.

set -u

# RAW_OUTPUT=1 bash -x test_shed.bash 2>&1 |  ts -i '%.s' | ts -s '%.s' > build/t 

D=/build/"$TEST_INSTANCE"

config_location=/build/shed-test-local-config
config_dl_location=/build/shed-test-local-dl-config
exterior_serial_file=/build/exterior_mock.pts2.file
exterior_serial_dev=/build/exterior_mock.pts2
email_rcpt_log=/build/email_mock_shed-test-local@tmp-test.test
shed=$1
shed_out_file=/build/shed-test-local-out

#pkill -f "$exterior_serial_dev" || true

groupadd "$TEST_INSTANCE" 2>/dev/null || true
iptables -D OUTPUT -p tcp --dport 9161 -m owner --gid-owner="$TEST_INSTANCE" -j REJECT --reject-with tcp-reset

cat << EOF > /build/mk_day_sec.py
from datetime import timedelta, datetime
import pytz
import sys
PT = pytz.timezone('America/Los_Angeles')
target = datetime.now(PT) + timedelta(hours=int(sys.argv[1]))
day_start = target.replace(hour=0, minute=0, second=0, microsecond=0)
print(int((target-day_start).total_seconds()))
EOF


function mk_day_sec() {
    python3 /build/mk_day_sec.py $1
}

open_at_sec="$( mk_day_sec 0 )"
close_at_sec="$( mk_day_sec 1 )"

[ -v RAW_OUTPUT ] || RAW_OUTPUT=0

function set_dl_config() {
    echo "$1" > $config_dl_location
    echo "-- Setting dl config:"
    cat $config_dl_location | { [ $RAW_OUTPUT = 0 ] && {
        sed -E 's/OpenAtSec: '"$open_at_sec"'/OpenAtSec: FILTERED/' | 
        sed -E 's/CloseAtSec: '"$close_at_sec"'/CloseAtSec: FILTERED/'
    } || cat; }

}

function dump_state() (
    set +x

    echo "-- Shed output:"
    cat $shed_out_file | tr -d '\000' | { [ $RAW_OUTPUT = 0 ] && {
        sed -E 's/Day:[0-9]+/Day:FILTERED/' |
        sed -E 's/Child:[0-9]+/Child:XXXX/' |
        sed -E 's/expires: [0-9]+ [0-9]+/expires: FILTERED FILTERED/' |
        sed -E 's/expire_day:[0-9]+ /expire_day:FILTERED /'
    } || cat; }
    truncate --size=0 $shed_out_file

    echo "-- Config on disk:"
    cat $config_location |  { [ $RAW_OUTPUT = 0 ] && {
        sed -E 's/UserNormal: [0-9]+/UserNormal: FILTERED/' |
        sed -E 's/OpenAtSec: '"$open_at_sec"'/OpenAtSec: FILTERED/' |
        sed -E 's/CloseAtSec: '"$close_at_sec"'/CloseAtSec: FILTERED/' |
        sort
    } || cat; }

    echo "-- Serial output:"
    cat $exterior_serial_file | tr -d '\000' | { [ $RAW_OUTPUT = 0 ] && {
        sed -E 's/Day:[0-9]+/Day:FILTERED/' |
        sed -E 's/Wait [0-9][0-9]:[0-9][0-9]/Wait FILTERED/' |
        sed -E 's/Wait [0-9][0-9]:[0-9][0-9]/Wait FILTERED/' |
        sed -E 's/TEXT_SHOW2[ ]+[0-9][0-9]:[0-9][0-9]/TEXT_SHOW2 FILTERED/' |
        awk '{$1=$1};1'
    } || cat; }       
    truncate --size=0 $exterior_serial_file

    echo "-- Emails:"
    cat $email_rcpt_log | { [ $RAW_OUTPUT = 0 ] && {
        sed -E 's/Day: [0-9]+/Day:FILTERED/' |
        sed -E $'s/b\'Date: .*/b\'Date: FILTERED/' |
        sed -E $'s/mail options: [\'SIZE=[0-9]+\']/mail options: [\'SIZE=FILTERED\']/'
    } || cat; }
    truncate --size=0 $email_rcpt_log

    set -x
)

function wait_line() {
    path=$1
    pattern=$2
    tail -s0 -fc1T $path | timeout 1 grep -am1 "$pattern" > /dev/null || echo "wait_line \"$path\" \"$pattern\" MISS"
}

function start_shed() {
    echo "Starting: $shed $config_location"
    SHED_TRACE=1 sg "$TEST_INSTANCE" "exec $shed $config_location" &> $shed_out_file & shed_pid=$!
    #SHED_TRACE=1 $shed $config_location &> $shed_out_file & shed_pid=$!
}

# function fail_cleanup() {
#     echo "Doing cleanup"
#     trap - ERR
#     set +e
#     set +E
#     set +u
#     kill $shed_pid
#     wait $shed_pid
#     kill $serial_copy_pid
#     wait $serial_copy_pid
#     exit -1
# }
# trap fail_cleanup ERR

function main() {
rm -f $email_rcpt_log
touch $email_rcpt_log
touch $shed_out_file

cat $exterior_serial_dev > $exterior_serial_file & serial_copy_pid=$!

# kill $shed_pid; wait $shed_pid

cat << EOF > $config_location
DebugSerialPath: /build/exterior_mock.pts
DebugClearTimeoutMS: 0
DoorUnlockMS: 0
ConfigURL: http://127.0.0.1:9161$config_dl_location
EmailAddress: tmp-from@testtest.test
EmailServer:  smtp://127.0.0.1:8025
EmailUserPass:  user:pass
DestinationEmailAddress: shed-test-local@tmp-test.test
ConfigDownloadStartupDelayMS: 0
EOF

set_dl_config ''

echo
echo "====================================================="
echo "=== Before run"
echo
dump_state

echo
echo "====================================================="
echo "=== First Sartup"
echo
start_shed
wait_line $shed_out_file 'Download finished'
wait_line $shed_out_file 'Maintenance Finished'
wait_line $exterior_serial_file 'SLEEP'
dump_state


echo
echo "====================================================="
echo "=== Making an unknown option"
echo
cat << EOF > $exterior_serial_dev
SCAN_START
OPTION 001
PIN 123456
RFID 000102030405060708090a0b0c0d0e0f1011121314151617
SCAN_FINISHED
EOF
wait_line $shed_out_file 'Got an unknown option'
wait_line $exterior_serial_file 'TEXT_SHOW2 $'
dump_state


echo
echo "====================================================="
echo "=== 00 Requesting hash email"
echo
cat << EOF > $exterior_serial_dev
SCAN_START
OPTION 100
PIN 123456
RFID 000102030405060708090a0b0c0d0e0f1011121314151617
SCAN_FINISHED
EOF
wait_line $shed_out_file 'Requested send'
wait_line $exterior_serial_file 'TEXT_SHOW2 $'
wait_line $email_rcpt_log 'END MESSAGE'
dump_state


echo
echo "====================================================="
echo "=== 00 badge in, expect unknown user"
echo
cat << EOF > $exterior_serial_dev
SCAN_START
PIN 123456
RFID 000102030405060708090a0b0c0d0e0f1011121314151617
SCAN_FINISHED
EOF
wait_line $exterior_serial_file 'TEXT_SHOW2 $'
dump_state


echo
echo "====================================================="
echo "=== Adding 00 hash to dl config, restarting shed"
echo

set_dl_config '
UserAdder: d2db0e01045de5d6c9bcb95ba549bcdf024bf2db2f7974538cb5983fa4d86db2
OpenAtSec: '"$open_at_sec"'
CloseAtSec: '"$close_at_sec"'
'

pkill -P $shed_pid
wait $shed_pid
shed_pid=-1
start_shed
wait_line $shed_out_file 'Download finished'
wait_line $shed_out_file 'Maintenance Finished'
wait_line $exterior_serial_file 'TEXT_SHOW2 $'
dump_state


echo
echo "====================================================="
echo "=== 00 badge in, expect granted"
echo
cat << EOF > $exterior_serial_dev
SCAN_START
PIN 123456
RFID 000102030405060708090a0b0c0d0e0f1011121314151617
SCAN_FINISHED
EOF
wait_line $shed_out_file 'GPIO_FAKE value:0'
wait_line $exterior_serial_file 'TEXT_SHOW2 $'
dump_state

echo
echo "====================================================="
echo "=== 11 badge in, expect unknown user"
echo
cat << EOF > $exterior_serial_dev
SCAN_START
PIN 123456
RFID 110102030405060708090a0b0c0d0e0f1011121314151617
SCAN_FINISHED
EOF
wait_line $exterior_serial_file 'TEXT_SHOW2 $'
dump_state


echo
echo "====================================================="
echo "=== 11 badge in, request email"
echo
cat << EOF > $exterior_serial_dev
SCAN_START
OPTION 100
PIN 123456
RFID 110102030405060708090a0b0c0d0e0f1011121314151617
SCAN_FINISHED
EOF
wait_line $shed_out_file 'Requested send'
wait_line $exterior_serial_file 'TEXT_SHOW2 $'
wait_line $email_rcpt_log 'END MESSAGE'
dump_state

echo
echo "====================================================="
echo "=== add 11 hash to dl"
echo
set_dl_config '
UserAdder: d2db0e01045de5d6c9bcb95ba549bcdf024bf2db2f7974538cb5983fa4d86db2
UserAdder: 82611bb175a3ee6e0d851c5116638a73de2c91c4f146348f9097902a6e798cb0
'

echo
echo "====================================================="
echo "=== 00 badge, opt 301, expect download"
echo
cat << EOF > $exterior_serial_dev
SCAN_START
OPTION 301
PIN 123456
RFID 000102030405060708090a0b0c0d0e0f1011121314151617
SCAN_FINISHED
EOF
wait_line $shed_out_file 'Download finished'
wait_line $exterior_serial_file 'TEXT_SHOW2 $'
sleep 0.01
dump_state


echo
echo "====================================================="
echo "=== 11 badge, expect access granted"
echo
cat << EOF > $exterior_serial_dev
SCAN_START
PIN 123456
RFID 110102030405060708090a0b0c0d0e0f1011121314151617
SCAN_FINISHED
EOF
wait_line $shed_out_file 'GPIO_FAKE value:0'
wait_line $exterior_serial_file 'TEXT_SHOW2 $'
dump_state


echo
echo "====================================================="
echo "=== 11 badge, opt 200, expect add next"
echo
cat << EOF > $exterior_serial_dev
SCAN_START
OPTION 200
PIN 123456
RFID 110102030405060708090a0b0c0d0e0f1011121314151617
SCAN_FINISHED
EOF
wait_line $exterior_serial_file 'TEXT_SHOW2 $'
dump_state


echo
echo "====================================================="
echo "=== 22 badge, opt 200, expect add next"
echo
cat << EOF > $exterior_serial_dev
SCAN_START
PIN 123456
RFID 220102030405060708090a0b0c0d0e0f1011121314151617
SCAN_FINISHED
EOF
wait_line $exterior_serial_file 'TEXT_SHOW2 $'
sleep 0.01
dump_state


echo
echo "====================================================="
echo "=== force kill, restart"
echo

iptables -A OUTPUT -p tcp --dport 9161 -m owner --gid-owner="$TEST_INSTANCE" -j REJECT  --reject-with tcp-reset
pkill -9 -P $shed_pid
wait $shed_pid 2> /dev/null
start_shed
wait_line $shed_out_file 'Maintenance Finished'
wait_line $exterior_serial_file 'TEXT_SHOW2 $'
dump_state


echo
echo "====================================================="
echo "=== 22 badge, expect access granted"
echo
cat << EOF > $exterior_serial_dev
SCAN_START
PIN 123456
RFID 220102030405060708090a0b0c0d0e0f1011121314151617
SCAN_FINISHED
EOF
wait_line $shed_out_file 'GPIO_FAKE value:0'
wait_line $exterior_serial_file 'TEXT_SHOW2 $'
dump_state


echo
echo "====================================================="
echo "=== get time back"
echo
cat << EOF > $exterior_serial_dev
SCAN_START
OPTION 302
PIN 0000
RFID 000002030405060708090a0b0c0d0e0f1011121314151617
SCAN_FINISHED
EOF
wait_line $exterior_serial_file 'TEXT_SHOW2 $'
dump_state


pkill -P $shed_pid
wait $shed_pid
shed_pid=-1
kill $serial_copy_pid
wait $serial_copy_pid
serial_copy_pid=-1

echo
echo "====================================================="
echo "=== final state"
echo
dump_state

return


} # main
# exit -1
main

