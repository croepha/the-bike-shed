#!/bin/bash
set -eEuo pipefail

EXEC=$1
CHECK_FILE=$2
OUT_FILE=$3

exec > $OUT_FILE
exec 2>&1

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

EMAIL_PATH='/build/email_mock_email_test_to@longlonglonglonglonglonglonglonghost.com'
rm -f "$EMAIL_PATH"
$EXEC
sleep .1

diff "$EMAIL_PATH" "$CHECK_FILE"

cleanup
echo "TEST_IS_SUCCESS"


