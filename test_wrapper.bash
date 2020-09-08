#!/bin/bash
set -eEuo pipefail

EXEC=$1
CHECK_FILE=$2
OUT_FILE=$3

rm -f $OUT_FILE*

function fail() {
    trap - ERR
    echo "FAILED" >> "$OUT_FILE"
    cat "$OUT_FILE.raw"
    exit -1
}
trap fail ERR

$EXEC 2>&1 | tee "$OUT_FILE.raw" |
  sed -E $'s/^([0-9a-f]+[.][0-9]{3}: (DEBUG| INFO| WARN|ERROR|FATAL):.*)\t(.*)$/\\1\e[999C\e[50D\\3/' >/dev/null # Right justify location info

# remove timestamp, and also line number and addresses, if they shift around, we dont want the test to fail...
sed -E $'s/^[0-9a-f]+[.][0-9]{3}: ((DEBUG| INFO| WARN|ERROR|FATAL):.*\t)\((.*):.* .*:(.*)\)$/\\1(\\3 \\4)/' < "$OUT_FILE.raw" |
  sed -E 's/Child:[0-9]+/Child:XXXX/' | sed '/^DEBUG:.*$/d' > "$OUT_FILE.cleaned1"

sed '/^.*test_sort.*$/d' < "$OUT_FILE.cleaned1" >>  "$OUT_FILE.cleaned"
sed -En 's/^(.*(test_sort:[^ ]*) .*)$/\2 \1/p' < "$OUT_FILE.cleaned1" |
  sort --stable --key=1,1 | sed -E 's/^(test_sort:[^ ]* )//' >>  "$OUT_FILE.cleaned"

function fail() {
    trap - ERR
    echo "FAILED" >> "$OUT_FILE"
    cat "$OUT_FILE"
    exit -1
}

echo "to overwrite check: cp $OUT_FILE.cleaned $CHECK_FILE" >> "$OUT_FILE"
diff "$OUT_FILE.cleaned" "$CHECK_FILE" >> "$OUT_FILE"
echo "PASSED" >> "$OUT_FILE"
