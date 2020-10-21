
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "common.h"
#include "logging.h"
#include "access.h"


static void access_hash_test(char * salt, char * rfid, char *  pin) {
    struct access_HashPayload payload = {};

#define _(v) \
    usz v ## _len = strlen(v); \
    assert(v ## _len <= sizeof payload. v); \
    memcpy(payload. v, v, v ## _len);

    _(salt);
    _(rfid);
    _(pin);
#undef _

    access_HashResult result = {};
    INFO_HEXBUFFER(&payload, sizeof payload, "payload:");
    __access_hash(result, &payload);
    INFO_HEXBUFFER(&result, sizeof result, "result:");
}

int main () {
    setbuf(stdout, 0);
    access_hash_test("00000000000001", "000000000000001", "0000000001");
    access_hash_test("00000000000002", "000000000000001", "0000000001");
    access_hash_test("00000000000001", "000000000000002", "0000000001");
    access_hash_test("00000000000001", "000000000000001", "0000000002");

}