
set -eEuo pipefail

function cleanup() {
    set +eEu
    kill $SHED_PID &> /dev/null || true
    timeout 0.5s cat 0>&$SHED_OUT
    kill -9 $SHED_PID &> /dev/null || true
}
function fail() {
    trap - ERR
    cleanup
    echo "TEST_IS_FAILURE"
    exit -1
}
trap fail ERR

coproc /build/shed.exec
SHED_PID=$!
SHED_OUT=${COPROC[0]}

timeout 0.1s grep -am1 "Started" 0>&$SHED_OUT

echo "Adfasdfasd" | socat STDIO UDP:127.0.0.1:9162 | grep -am1 "some response"
timeout 0.1s grep -am1 "Got a UDP packet" 0>&$SHED_OUT

cleanup
echo "TEST_IS_SUCCESS"


