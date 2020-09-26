
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
  u8 debug_is_free;
  union {
    struct {
        access_HashResult hash;
        u16 expire_day;
    };
  };
  u16 next_idx;
} access_users_space[USER_TABLE_LEN];
u16 access_map[HASH_MAP_LEN];


void access_user_add(access_HashResult hash, u16 expire_day);
void access_user_list_init(void);


u16 access_users_first_free_idx;
u16 access_users_first_idx;

void access_user_list_init(void) {

  // Initialize the free list
  access_users_first_free_idx = -1;
  for (u16 i = 0; i < USER_TABLE_LEN; i++) {
    access_users_space[i].debug_is_free = 1;
    access_users_space[i].next_idx = access_users_first_free_idx;
    access_users_first_free_idx = i;
  }
}

static u32 get_hash_mask(u32 v) { return v & (HASH_MAP_LEN - 1); }
static u32 get_hash_i(access_HashResult hash_result) { return get_hash_mask(*(u32*)hash_result); }

// static void access_user_remove_i(u32 hash_i) {

// }

#define USER (access_users_space[idx])
void access_user_add(access_HashResult hash, u16 expire_day) {

  if (access_users_first_free_idx == (u16)-1) {
    ERROR("User list full");
    return;
  }

  u32 hash_i = get_hash_i(hash);
  for (;;hash_i = get_hash_mask(hash_i + 1) ) {
    if (access_map[hash_i] != 0 && access_map[hash_i] != (u16)-1) {
      u16 idx = access_map[hash_i] + 1;
      if (memcmp(USER.hash, hash, sizeof USER.hash) == 0) {
        // Update existing
        USER.expire_day = expire_day;

      }
    } else {  // Found empty space make new:
      // pop off the free list
      u16 idx = access_users_first_free_idx;
      access_users_first_free_idx = USER.next_idx;
      assert(USER.debug_is_free);
      USER.debug_is_free = 0;

      USER.next_idx = access_users_first_idx;
      access_users_first_idx = idx;

      memcpy(USER.hash, hash, sizeof USER.hash);
      USER.expire_day = expire_day;

    }
  }

}


#include <time.h>
u8 access_requested(char * rfid, char * pin) {
  struct access_HashPayload payload = {};
  memcpy(payload.salt, salt, sizeof payload.salt);
  memcpy(payload.rfid, rfid, sizeof payload.rfid);
  memcpy(payload.pin , pin , sizeof payload.pin );
  access_HashResult hash_result;
  access_hash(&hash_result, &payload);
  INFO_HEXBUFFER(&hash_result, sizeof hash_result, "hash_result:");

  assert( !get_hash_mask(HASH_MAP_LEN) ); // Assert HASH_MAP_SIZE is power of 2
  assert( HASH_MAP_LEN >= USER_TABLE_LEN);

  // Lets put the cutoff at 1pm Pacific Time, that ensures that fresh
  //  expirations happen when there are likely to be most noisebridgers around
  u64  utc_sec = time(0);
  u64  pt_day = (utc_sec - 60*60*7 /* 7 hours */ ) / 60*60*24;

  u32 hash_i = get_hash_i(hash_result);
  for (;;hash_i = get_hash_mask(hash_i + 1) ) {
    if (access_map[hash_i] != 0 && access_map[hash_i] != (u16)-1) {
      struct accessUser * user = &access_users_space[access_map[hash_i] + 1];
      if (memcmp(user->hash, hash_result, sizeof user->hash) == 0) {
        // Found
        if (user->expire_day <= pt_day) {
          return 1; // Valid
        } else {
          return 0; // Expired
        }
      } // else continue
    } else {
      return 0; // Hash not in our map
    }
  }
  return 0; // unreachable
}

int main() {
    access_requested("rfidrfidrfidrfidrfidrf2d", "pin1231231");
}