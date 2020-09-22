
#include <string.h>

#include "logging.h"
#include "common.h"

static usz const SALT_BUF_LEN = 64;
static usz const RFID_BUF_LEN = 24;
static usz const PIN_BUF_LEN = 10;

struct HashPayload {
  char salt[SALT_BUF_LEN];
  char rfid[RFID_BUF_LEN];
  char pin[PIN_BUF_LEN];
};

char const salt[SALT_BUF_LEN] = "asdfaew";

u8 access_requested(char * rfid, char * pin);

typedef u8 HashResult[64];

void hash(HashResult * result, void* data, usz data_len);

void hash(HashResult * result, void* data, usz data_len) {
    INFO_HEXBUFFER(data, data_len, "data:");
    memset(result, 0x69, sizeof * result);
}

u8 access_requested(char * rfid, char * pin) {
  struct HashPayload payload = {};
  memcpy(payload.salt, salt, SALT_BUF_LEN);
  memcpy(payload.rfid, rfid, RFID_BUF_LEN);
  memcpy(payload.pin , pin , PIN_BUF_LEN );
  HashResult hash_result;
  hash(&hash_result, &payload, sizeof payload);
  INFO_HEXBUFFER(&hash_result, sizeof hash_result, "hash_result:");
  return 0;
}

int main() {
    access_requested("sdal;asdfasdfasdfadfasdfasdfsskdf;laksdl;fasdf", "12aas'dkfa';dfkl'a123");
}