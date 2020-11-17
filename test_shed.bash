

config_location=/build/shed-test-local-config
config_dl_location=/build/shed-test-local-dl-config
exterior_serial_file=/build/exterior_mock.pts2.file
exterior_serial_dev=/build/exterior_mock.pts2
email_rcpt_log=/build/email_mock_shed-test-local@tmp-test.test
shed=/build/shed3.dbg.exec
shed_out_file=/build/shed-test-local-out

cat << EOF > $config_location
DebugSerialPath: /build/exterior_mock.pts
DebugClearTimeoutMS: 0
ConfigURL: http://127.0.0.1:9160$config_dl_location
EmailAddress: tmp-from@testtest.test
EmailServer:  smtp://127.0.0.1:8025
EmailUserPass:  user:pass
DestinationEmailAddress: shed-test-local@tmp-test.test
EOF

cat << EOF > $config_dl_location
EOF

cat $exterior_serial_dev > $exterior_serial_file & serial_copy_pid=$!

rm -f $email_rcpt_log
touch $email_rcpt_log
touch $shed_out_file

function dump_state() {
    echo "Shed output:"
    cat $shed_out_file
    cat /dev/null > $shed_out_file
    echo "Config on disk:"
    cat $config_location
    echo "DL Config:"
    cat $config_dl_location
    echo "Serial output:"
    cat $exterior_serial_file
    cat /dev/null > $exterior_serial_file
    echo "Emails:"
    cat $email_rcpt_log
    rm -f $email_rcpt_log
    touch $email_rcpt_log
}

function wait_line() {
    path=$1
    pattern=$2
    tail -s0 -fc1T $path | timeout 0.5 grep -am1 "$pattern" > /dev/null
}


kill $shed_pid
wait $shed_pid

dump_state
echo "Starting: $shed $config_location"
$shed $config_location &> $shed_out_file & shed_pid=$!

wait_line $shed_out_file 'Download finished'
wait_line $shed_out_file 'Maintenance Finished'
wait_line $exterior_serial_file 'TEXT_SHOW2$'
dump_state


echo "Making an unknown option"
cat << EOF > $exterior_serial_dev
SCAN_START
OPTION 001
PIN 123456
RFID 000102030405060708090a0b0c0d0e0f1011121314151617
SCAN_FINISHED
EOF
wait_line $shed_out_file 'Got an unknown option'
wait_line $exterior_serial_file 'Unkown option'
dump_state

cat << EOF > $exterior_serial_dev
SCAN_START
OPTION 100
PIN 123456
RFID 000102030405060708090a0b0c0d0e0f1011121314151617
SCAN_FINISHED
EOF
wait_line $shed_out_file 'Requested send'
wait_line $exterior_serial_file 'Request sending'
wait_line $email_rcpt_log 'END MESSAGE'
dump_state

echo "UserAdder: d2db0e01045de5d6c9bcb95ba549bcdf024bf2db2f7974538cb5983fa4d86db2" > $config_dl_location

kill $shed_pid
wait $shed_pid
echo "Starting: $shed $config_location"
$shed $config_location &> $shed_out_file & shed_pid=$!
wait_line $shed_out_file 'Download finished'
wait_line $shed_out_file 'Maintenance Finished'
wait_line $exterior_serial_file 'System restart'
dump_state


cat << EOF > $exterior_serial_dev
SCAN_START
PIN 123456
RFID 000102030405060708090a0b0c0d0e0f1011121314151617
SCAN_FINISHED
EOF
wait_line $shed_out_file 'Requested send'
wait_line $exterior_serial_file 'Request sending'
dump_state




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





