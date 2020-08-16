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

tabs 8

EXEC=$1
CHECK_FILE=$2
OUT_FILE=$3
(
  (
    /build/io_core_test.dbg.exec
  ) 2>&1 | tee "$OUT_FILE" |
    sed -E $'s/^((DEBUG| INFO| WARN|ERROR|FATAL):.*)\t(.*)$/\\1\e[999C\e[50D\\3/'

  # remove line number and addresses, if they shift around, we dont want the test to fail...
  sed -E $'s/^((DEBUG| INFO| WARN|ERROR|FATAL): .*\t\(.*:.*):.*:.*\)$/\\1)/p' < "$OUT_FILE" > "$OUT_FILE.cleaned"

  diff "$OUT_FILE.cleaned" "$CHECK_FILE"

) &
wait $!


