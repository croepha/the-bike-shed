set -eEuo pipefail

function cleanup() {
    set +eEu
    kill $( jobs -p ) &> /dev/null || true
    sleep .1
    kill -9 $( jobs -p ) &> /dev/null || true
}
function fail() {
    trap - ERR
    cleanup
    echo "TEST_IS_FAILURE"
    exit -1
}
trap fail ERR
trap fail INT
script_pid=$$
(
    sleep 5
    kill -INT $script_pid
 ) &

rm -f /build/email_test.smtp.out*
python -u -m smtpd --nosetuid --class DebuggingServer --debug &> /build/email_test.smtp.out1 &
EMAIL_PID=$!
sleep .1
/build/hello_email.dbg.exec
sleep .1
kill $EMAIL_PID || true
wait $EMAIL_PID || true

cat /build/email_test.smtp.out1 |
    grep -v '^DebuggingServer started at' |
    grep -v '^Incoming connection from' |
    grep -v '^Peer:' > /build/email_test.smtp.out || true

diff /build/email_test.smtp.out email_test.expected_output

cleanup
echo "TEST_IS_SUCCESS"


