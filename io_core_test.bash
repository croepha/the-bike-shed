#!/bin/bash
set -eEuo pipefail
exit
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

script_pid=$$
{ sleep 5 & wait $!; echo "TIMEOUT"; kill -INT $script_pid; } &
timeout_pid=$!


EXEC=$1
CHECK_FILE=$2
OUT_FILE=$3

rm -f $OUT_FILE{,.raw,.cleaned,.cleaned1}
exec > $OUT_FILE
exec 2>&1

(
  (
    (
      sleep .001
      ( tail -s.001 -n0 -f "$OUT_FILE.raw" & ) | grep -q 'All test sockets created' > /dev/null # wait for line
      for i in {0..19}; do {
        ( echo "SEND$i" | socat stdio unix-connect:/tmp/test_$i & ) |
          grep --line-buffered -m1 "REPLY$i" |
          sed -E 's/(.*)/test_sort:id:'$( printf "%02d" "$i")' \1/'&
      }; done
    ) &
    /build/io_core_test.dbg.exec
  ) 2>&1 | tee -a "$OUT_FILE.raw" |
    sed -E $'s/^([0-9a-f]+[.][0-9]{3}: (DEBUG| INFO| WARN|ERROR|FATAL):.*)\t(.*)$/\\1\e[999C\e[50D\\3/' # >/dev/null # Right justify location info

  # remove timestamp, and also line number and addresses, if they shift around, we dont want the test to fail...
  sed -E $'s/^[0-9a-f]+[.][0-9]{3}: ((DEBUG| INFO| WARN|ERROR|FATAL):.*\t)\((.*):.* .*:(.*)\)$/\\1(\\3 \\4)/' < "$OUT_FILE.raw" > "$OUT_FILE.cleaned1"
  {
    grep -v "test_sort:" < "$OUT_FILE.cleaned1"
    sed -En 's/^(.*(test_sort:[^ ]*) .*)$/\2 \1/p' < "$OUT_FILE.cleaned1" | sort --stable --key=1,1 | sed -E 's/^(test_sort:[^ ]* )//'
  } >  "$OUT_FILE.cleaned"

  #echo "CLEANED: $OUT_FILE.cleaned"; cat "$OUT_FILE.cleaned"

  diff "$OUT_FILE.cleaned" "$CHECK_FILE"
  jobs

) &
wait $!
kill -9 $timeout_pid
jobs
echo PASSED


