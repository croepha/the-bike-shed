
#include <string.h>

#include "logging.h"
#include "common.h"
#include "access.h"

static usz const SALT_BUF_LEN = 64;

char const salt[SALT_BUF_LEN] = "saltysalt";

u8 access_requested(char * rfid, char * pin);

void access_hash(access_HashResult * result, struct access_HashPayload * payload) {
    INFO_BUFFER((char*)payload, sizeof * payload, "payload:");
    memset(result, 0x69, sizeof * result);
}

static usz const HASH_MAP_LEN  = 1 << 18; // 256Ki
static usz const USER_TABLE_LEN = 1 << 13; // 8192

struct accessUser {
  u8 is_free;
  union {
    struct {
        access_HashResult hash;
        u16 expire_day;
    };
    u16 free_next;
  };
} access_users[USER_TABLE_LEN];
u16 access_map[HASH_MAP_LEN];

#include <time.h>
u8 access_requested(char * rfid, char * pin) {
  struct access_HashPayload payload = {};
  memcpy(payload.salt, salt, sizeof payload.salt);
  memcpy(payload.rfid, rfid, sizeof payload.rfid);
  memcpy(payload.pin , pin , sizeof payload.pin );
  access_HashResult hash_result;
  access_hash(&hash_result, &payload);
  INFO_HEXBUFFER(&hash_result, sizeof hash_result, "hash_result:");

  assert( ((HASH_MAP_LEN - 1) & HASH_MAP_LEN) == 0); // Assert HASH_MAP_SIZE is power of 2
  assert( HASH_MAP_LEN >= USER_TABLE_LEN);

  // Lets put the cutoff at 1pm Pacific Time, that ensures that fresh
  //  expirations happen when there are likely to be most noisebridgers around
  u64  utc_sec = time(0);
  u64  pt_day =  (utc_sec - 60*60*7 /* 7 hours */ ) / 60*60*24;

  u32 hash_i = (*(u32*)&hash_result) & (HASH_MAP_LEN - 1);
  for (;;hash_i++) {
    if (access_map[hash_i]) {
      struct accessUser * user = &access_users[access_map[hash_i] + 1];
      if (memcmp( user->hash, hash_result, sizeof hash_result) == 0) {
        // Match
        if (user->expire_day <= pt_day) {
          return 1;
        } else {
          return 0; // Expired
        }
      } // else loop
    } else {
      return 0; // Hash not in our map
    }
  }
  return 0; // unreachable
}

int main() {
    access_requested("rfidrfidrfidrfidrfidrf2d", "pin1231231");
}