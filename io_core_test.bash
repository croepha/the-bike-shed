#!/bin/bash

set -eEuo pipefail

script_pid=$$
( sleep 5; echo "TIMEOUT"; kill -INT $script_pid; ) &

function cleanup() {
  kill $( jobs -p ) &> /dev/null || true
  sleep .1
  kill -9 $( jobs -p ) &> /dev/null || true
}

function fail() {
    trap - ERR
    trap - INT
    echo "TEST_IS_FAILURE"
    cleanup
    exit -1
}

trap fail ERR
trap fail INT

EXEC=$1
CHECK_FILE=$2
OUT_FILE=$3
rm -f "$OUT_FILE"

(
  (
    (
      sleep .001
      ( tail -n0 -f "$OUT_FILE" & ) | grep -q 'All test sockets created' > /dev/null
      ( echo "SEND11" | socat stdio unix-connect:/tmp/test_11 & ) | grep --line-buffered -m1 "REPLY11"
    ) &
    /build/io_core_test.dbg.exec
  ) 2>&1 | tee "$OUT_FILE" |
    sed -E $'s/^([0-9a-f]+[.][0-9]{3}:(DEBUG| INFO| WARN|ERROR|FATAL):.*)\t(.*)$/\\1\e[999C\e[50D\\3/'  # Right justify location info

  # remove timestamp, and also line number and addresses, if they shift around, we dont want the test to fail...
  sed -E $'s/^[0-9a-f]+[.][0-9]{3}:((DEBUG| INFO| WARN|ERROR|FATAL):.*\t)\((.*):.* .*:(.*)\)$/\\1(\\3 \\4)/' < "$OUT_FILE" > "$OUT_FILE.cleaned"

  cat "$OUT_FILE.cleaned"

  diff "$OUT_FILE.cleaned" "$CHECK_FILE"

) &
wait $!


