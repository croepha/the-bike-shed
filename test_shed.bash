#set -eEuo pipefail
set -u

config_location=/build/shed-test-local-config
config_dl_location=/build/shed-test-local-dl-config
exterior_serial_file=/build/exterior_mock.pts2.file
exterior_serial_dev=/build/exterior_mock.pts2
email_rcpt_log=/build/email_mock_shed-test-local@tmp-test.test
shed=/build/shed3.dbg.exec
shed_out_file=/build/shed-test-local-out

function set_dl_config() {
    echo "$1" > $config_dl_location
    echo "Setting dl config:"
    cat $config_dl_location
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

set -x
cat $exterior_serial_dev > $exterior_serial_file & serial_copy_pid=$!

rm -f $email_rcpt_log
touch $email_rcpt_log
touch $shed_out_file

function t() { tr -d '\000' | sed -E 's/^[0-9a-f]+[.][0-9]{3}: ((DEBUG|TRACE| INFO| WARN|ERROR|FATAL):.*)\((.*):.* .*:(.*)\)$/\1(\3 \4)/'; }


function dump_state() {
    set +x
    echo "Shed output:"
    cat $shed_out_file | t | sed -E 's/Day:[0-9]+/Day:FILTERED/'
    #   sed -E $'s/^[0-9a-f]+[.][0-9]{3}: ((DEBUG|TRACE| INFO| WARN|ERROR|FATAL):.*\t)\((.*):.* .*:(.*)\)$/\\1(\\3 \\4)/' |
    #   sed -E 's/Child:[0-9]+/Child:XXXX/' | sed '/^DEBUG:.*$/d'
    truncate --size=0 $shed_out_file
    echo "Config on disk:"
    cat $config_location
    echo "Serial output:"
    cat $exterior_serial_file | sed -E 's/Day:[0-9]+/Day:FILTERED/'
    truncate --size=0 $exterior_serial_file
    echo "Emails:"
    cat $email_rcpt_log |
        sed -E 's/Day: [0-9]+/Day:FILTERED/' |
        sed -E $'s/b\'Date: .*/b\'Date: FILTERED/' |
        sed -E $'s/mail options: [\'SIZE=[0-9]+\']/mail options: [\'SIZE=FILTERED\']/'
    rm -f $email_rcpt_log
    touch $email_rcpt_log
    # set -x
}

function wait_line() {
    path=$1
    pattern=$2
    tail -s0 -fc1T $path | timeout 0.5 grep -am1 "$pattern" > /dev/null
}

function start_shed() {
    echo "Starting: $shed $config_location"
    $shed $config_location &> $shed_out_file & shed_pid=$!
}

cat $exterior_serial_dev > $exterior_serial_file & serial_copy_pid=$!

kill $shed_pid; wait $shed_pid

cat << EOF > $config_location
DebugSerialPath: /build/exterior_mock.pts
DebugClearTimeoutMS: 0
DoorUnlockMS: 0
ConfigURL: http://127.0.0.1:9160$config_dl_location
EmailAddress: tmp-from@testtest.test
EmailServer:  smtp://127.0.0.1:8025
EmailUserPass:  user:pass
DestinationEmailAddress: shed-test-local@tmp-test.test
EOF

set_dl_config ''


echo "--- Before run"
dump_state

echo "--- First Sartup"
start_shed
wait_line $shed_out_file 'Download finished'
wait_line $shed_out_file 'Maintenance Finished'
wait_line $exterior_serial_file 'TEXT_SHOW2 $'
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

echo "--- Requesting hash email"
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

echo "--- badge in, expect unknown user"
cat << EOF > $exterior_serial_dev
SCAN_START
PIN 123456
RFID 000102030405060708090a0b0c0d0e0f1011121314151617
SCAN_FINISHED
EOF
wait_line $exterior_serial_file 'TEXT_SHOW2 $'
dump_state


echo "--- Adding hash to dl config, restarting shed"

set_dl_config '
UserAdder: d2db0e01045de5d6c9bcb95ba549bcdf024bf2db2f7974538cb5983fa4d86db2
'

kill $shed_pid
wait $shed_pid
start_shed
wait_line $shed_out_file 'Download finished'
wait_line $shed_out_file 'Maintenance Finished'
wait_line $exterior_serial_file 'TEXT_SHOW2 $'
dump_state

echo "--- badge in, expect granted"
cat << EOF > $exterior_serial_dev
SCAN_START
PIN 123456
RFID 000102030405060708090a0b0c0d0e0f1011121314151617
SCAN_FINISHED
EOF
wait_line $shed_out_file 'GPIO_FAKE value:0'
wait_line $exterior_serial_file 'TEXT_SHOW2 $'
dump_state



kill $shed_pid
wait $shed_pid
kill $serial_copy_pid
wait $serial_copy_pid
dump_state
exit



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





