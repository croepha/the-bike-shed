
#include <string.h>

#include "logging.h"
#include "common.h"
#include "access.h"

static usz const SALT_BUF_LEN = 64;
static usz const RFID_BUF_LEN = 24;
static usz const PIN_BUF_LEN = 10;

char const salt[SALT_BUF_LEN] = "saltysalt";

u8 access_requested(char * rfid, char * pin);

void access_hash(access_HashResult * result, struct access_HashPayload * payload) {
    INFO_BUFFER((char*)payload, sizeof * payload, "payload:");
    memset(result, 0x69, sizeof * result);
}

u8 access_requested(char * rfid, char * pin) {
  struct access_HashPayload payload = {};
  memcpy(payload.salt, salt, SALT_BUF_LEN);
  memcpy(payload.rfid, rfid, RFID_BUF_LEN);
  memcpy(payload.pin , pin , PIN_BUF_LEN );
  access_HashResult hash_result;
  access_hash(&hash_result, &payload);
  INFO_HEXBUFFER(&hash_result, sizeof hash_result, "hash_result:");
  return 0;
}

int main() {
    access_requested("rfidrfidrfidrfidrfidrf2d", "pin1231231");
}