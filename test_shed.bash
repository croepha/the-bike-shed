#!/bin/bash



#set -eEuo pipefail
set -u

#bash test_shed.bash > /build/shed-test-local-out
#diff -u10 --text /build/test-shed-local-test-out.expected /build/test-shed-local-test-out

# set -x

nginx_config=/build/shed-test-nginx-config
nginx_pidfile=/build/shed-test-nginx-pidfile
config_location=/build/shed-test-local-config
config_dl_location=/build/shed-test-local-dl-config
exterior_serial_file=/build/exterior_mock.pts2.file
exterior_serial_dev=/build/exterior_mock.pts2
email_rcpt_log=/build/email_mock_shed-test-local@tmp-test.test
shed=$1
shed_out_file=/build/shed-test-local-out
test_out_file=/build/shed-test-local-test-out
test_out_file_expected=/build/shed-test-local-test-out.expected


nginx -s stop -c "$nginx_config"  || true

function set_dl_config() {
    echo "$1" > $config_dl_location
    echo "Setting dl config:"
    cat $config_dl_location
}


function dump_state() (
    set +x
    echo "==  Shed output:"
    cat $shed_out_file | tr -d '\000' |
        sed -E 's/Day:[0-9]+/Day:FILTERED/' |
        sed -E 's/Child:[0-9]+/Child:XXXX/' |
        sed -E 's/expires: [0-9]+ [0-9]+/expires: FILTERED FILTERED/' |
        sed -E 's/expire_day:[0-9]+ /expire_day:FILTERED /'
    truncate --size=0 $shed_out_file
    echo "==  Config on disk:"
    cat $config_location |  sed -E 's/UserNormal: [0-9]+/UserNormal: FILTERED/' | sort
    echo "==  Serial output:"
    cat $exterior_serial_file | tr -d '\000' | sed -E 's/Day:[0-9]+/Day:FILTERED/' |
       sed -E 's/Wait [0-9][0-9]:[0-9][0-9]/Wait FILTERED/' |
       awk '{$1=$1};1'
    truncate --size=0 $exterior_serial_file
    echo "Emails:"
    cat $email_rcpt_log |
        sed -E 's/Day: [0-9]+/Day:FILTERED/' |
        sed -E $'s/b\'Date: .*/b\'Date: FILTERED/' |
        sed -E $'s/mail options: [\'SIZE=[0-9]+\']/mail options: [\'SIZE=FILTERED\']/'
    rm -f $email_rcpt_log
    touch $email_rcpt_log
    # set -x
)

function wait_line() {
    path=$1
    pattern=$2
    tail -s0 -fc1T $path | timeout 1 grep -am1 "$pattern" > /dev/null || echo "wait_line \"$path\" \"$pattern\" MISS"
}

function start_shed() {
    echo "Starting: $shed $config_location"
    SHED_TRACE=1 $shed $config_location &> $shed_out_file & shed_pid=$!
}

cat << EOF > $nginx_config
pid $nginx_pidfile;
events { worker_connections 768; }
http {
    server {
        listen 9161;
        autoindex on;
        root /;
    }
}
EOF
nginx -c $nginx_config



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


echo "--- Before run"
dump_state

echo "--- First Sartup"
start_shed
wait_line $shed_out_file 'Download finished'
wait_line $shed_out_file 'Maintenance Finished'
wait_line $exterior_serial_file 'SLEEP'
dump_state

echo "--- Making an unknown option"
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

echo "--- 00 Requesting hash email"
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

echo "--- 00 badge in, expect unknown user"
cat << EOF > $exterior_serial_dev
SCAN_START
PIN 123456
RFID 000102030405060708090a0b0c0d0e0f1011121314151617
SCAN_FINISHED
EOF
wait_line $exterior_serial_file 'TEXT_SHOW2 $'
dump_state


echo "--- Adding 00 hash to dl config, restarting shed"

set_dl_config '
UserAdder: d2db0e01045de5d6c9bcb95ba549bcdf024bf2db2f7974538cb5983fa4d86db2
'

kill $shed_pid
wait $shed_pid
shed_pid=-1
start_shed
wait_line $shed_out_file 'Download finished'
wait_line $shed_out_file 'Maintenance Finished'
wait_line $exterior_serial_file 'TEXT_SHOW2 $'
dump_state


echo "--- 00 badge in, expect granted"
cat << EOF > $exterior_serial_dev
SCAN_START
PIN 123456
RFID 000102030405060708090a0b0c0d0e0f1011121314151617
SCAN_FINISHED
EOF
wait_line $shed_out_file 'GPIO_FAKE value:0'
wait_line $exterior_serial_file 'TEXT_SHOW2 $'
dump_state


echo "--- 11 badge in, expect unknown user"
cat << EOF > $exterior_serial_dev
SCAN_START
PIN 123456
RFID 110102030405060708090a0b0c0d0e0f1011121314151617
SCAN_FINISHED
EOF
wait_line $exterior_serial_file 'TEXT_SHOW2 $'
dump_state


echo "--- 11 badge in, request email"
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

echo "--- add 11 hash to dl"
set_dl_config '
UserAdder: d2db0e01045de5d6c9bcb95ba549bcdf024bf2db2f7974538cb5983fa4d86db2
UserAdder: 82611bb175a3ee6e0d851c5116638a73de2c91c4f146348f9097902a6e798cb0
'

echo "--- 00 badge, opt 301, expect download"
cat << EOF > $exterior_serial_dev
SCAN_START
OPTION 301
PIN 123456
RFID 000102030405060708090a0b0c0d0e0f1011121314151617
SCAN_FINISHED
EOF
wait_line $shed_out_file 'Download finished'
wait_line $exterior_serial_file 'TEXT_SHOW2 $'
sleep 0.1
dump_state


echo "--- 11 badge, expect access granted"
cat << EOF > $exterior_serial_dev
SCAN_START
PIN 123456
RFID 110102030405060708090a0b0c0d0e0f1011121314151617
SCAN_FINISHED
EOF
wait_line $shed_out_file 'GPIO_FAKE value:0'
wait_line $exterior_serial_file 'TEXT_SHOW2 $'
dump_state


echo "--- 11 badge, opt 200, expect add next"
cat << EOF > $exterior_serial_dev
SCAN_START
OPTION 200
PIN 123456
RFID 110102030405060708090a0b0c0d0e0f1011121314151617
SCAN_FINISHED
EOF
wait_line $exterior_serial_file 'TEXT_SHOW2 $'
dump_state


echo "--- 22 badge, opt 200, expect add next"
cat << EOF > $exterior_serial_dev
SCAN_START
PIN 123456
RFID 220102030405060708090a0b0c0d0e0f1011121314151617
SCAN_FINISHED
EOF
wait_line $exterior_serial_file 'TEXT_SHOW2 $'
sleep 0.5
dump_state

echo "--- force kill, restart"

nginx -s stop -c "$nginx_config"
kill -9 $shed_pid
wait $shed_pid 2> /dev/null
start_shed
wait_line $shed_out_file 'Maintenance Finished'
wait_line $exterior_serial_file 'TEXT_SHOW2 $'
dump_state


echo "--- 22 badge, expect access granted"
cat << EOF > $exterior_serial_dev
SCAN_START
PIN 123456
RFID 220102030405060708090a0b0c0d0e0f1011121314151617
SCAN_FINISHED
EOF
wait_line $shed_out_file 'GPIO_FAKE value:0'
wait_line $exterior_serial_file 'TEXT_SHOW2 $'
dump_state


kill $shed_pid
wait $shed_pid
shed_pid=-1
kill $serial_copy_pid
wait $serial_copy_pid
serial_copy_pid=-1

echo "--- final state"
dump_state

return

cat << EOF > $exterior_serial_dev
SCAN_START
OPTION 200
PIN 123456
RFID 000102030405060708090a0b0c0d0e0f1011121314151617
SCAN_FINISHED
EOF
dump_state


cat << EOF > $exterior_serial_dev
SCAN_START
PIN 123456
RFID 010102030405060708090a0b0c0d0e0f1011121314151617
SCAN_FINISHED
EOF
dump_state

} # main
# exit -1
main
# main > $test_out_file
# echo "Do:" cp $test_out_file $test_out_file_expected
# echo code --diff $test_out_file_expected $test_out_file
# # diff -u10 --text $test_out_file_expected $test_out_file

# main | ts -i '%.T'
