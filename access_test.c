#define LOG_DEBUG
#include <string.h>

#include <time.h>

#include "logging.h"
#include "common.h"
#include "access.h"

static usz const SALT_BUF_LEN = 64;

char salt[SALT_BUF_LEN] = "saltysalt";

u8 access_requested(char * rfid, char * pin);

void access_hash(access_HashResult result, struct access_HashPayload * payload) {
    memset(result, 0x69, sizeof(access_HashResult));
    *(u64*)result = *(u64*)payload->salt;
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

#define USER (access_users_space[USER_idx])
#define MAP (access_map[MAP_idx])


void access_user_list_init(void) {

  // Initialize the free list
  access_users_first_free_idx = -1;
  for (u16 USER_idx = 0; USER_idx < USER_TABLE_LEN; USER_idx++) {

    USER.debug_is_free = 1;
    USER.next_idx = access_users_first_free_idx;
    access_users_first_free_idx = USER_idx;

  }
}

static u32 get_hash_mask(u32 v) { return v & (HASH_MAP_LEN - 1); }
static u32 get_hash_i(access_HashResult hash_result) { return get_hash_mask(*(u32*)hash_result); }

// // TODO
// access_user_remove_i
// void access_user_remove_i(u32 MAP_idx) {
// }
void access_idle_maintenance(void);
void access_idle_maintenance(void) {

  // Scan for users that need to be pruned

  u16 * prev = &access_users_first_idx;

  if (*prev == (u16)-1) {
    return;
  }

  u16 USER_idx = *prev;


  assert(!USER.debug_is_free);

  u64  utc_sec = time(0);
  u64  pt_day = (utc_sec - 60*60*7 /* 7 hours */ ) / 60*60*24;

  u32 map_first_tombstone = (u32)-1;

  u32 map_rage_start_idx = get_hash_i(USER.hash);
  u32 map_range_end = map_rage_start_idx;

  for (u32 MAP_idx = map_rage_start_idx;; MAP_idx = get_hash_mask(MAP_idx + 1) ) {
    DEBUG("MAP_idx: %u", MAP_idx);
    if (MAP == (u16)-1 && map_first_tombstone == (u32)-1) {
      DEBUG("Hit tombstone");
      map_first_tombstone = MAP_idx;
    } else if (MAP == 0) {
      DEBUG("Hash not in our map");
      assert(0);
      map_range_end = MAP_idx;
      break; // Hash not in our map
    } else {
      map_range_end = MAP_idx;
      assert(USER_idx == MAP - 1);
      if (USER.expire_day < pt_day) { // This entry is expired lets remove it
        USER.debug_is_free = 1;
        USER.next_idx = access_users_first_free_idx;
        access_users_first_free_idx = USER_idx;
        MAP = -1;
        *prev = USER.next_idx;
      } else {
        if (map_first_tombstone != (u32)-1) {
          DEBUG("We hit a tombstone on the way, lets go ahead and swap this entry with that one");
          MAP = (u16)-1;
          MAP_idx = map_first_tombstone;
          MAP = USER_idx + 1;
        }
        prev = &USER.next_idx;
      }
      break;
    }
  }

  // Walk the map range backwards, looking for tombstones
}

void access_user_add(access_HashResult hash, u16 expire_day) {

  DEBUG_HEXBUFFER(hash, sizeof(access_HashResult), "hash:");

  if (access_users_first_free_idx == (u16)-1) {
    ERROR("User list full");
    // Try real hard to fee up some space
    access_idle_maintenance();
    if (access_users_first_free_idx == (u16)-1) {
      INFO("No space could be freed, giving up");
      return;
    }
  }

  u32 map_first_tombstone = (u32)-1;
  for (u32 MAP_idx = get_hash_i(hash);; MAP_idx = get_hash_mask(MAP_idx + 1) ) {
    if (MAP == (u16)-1 && map_first_tombstone == (u32)-1) {
      DEBUG("Hit first tombstone MAP_idx:%u", MAP_idx);
      map_first_tombstone = MAP_idx;
    } else if (MAP == 0) {
      // pop off the free list
      u16 USER_idx = access_users_first_free_idx;
      access_users_first_free_idx = USER.next_idx;
      assert(USER.debug_is_free);

      if (map_first_tombstone != (u32)-1) {
        DEBUG("MAP_idx:%u USER_idx:%hu Adding new, replacing tombstone instead of making new slot, got to %u ", map_first_tombstone, USER_idx, MAP_idx);
        MAP_idx = map_first_tombstone;
      } else {
        DEBUG("MAP_idx:%u USER_idx:%hu Adding new, in new slot", map_first_tombstone, USER_idx);
      }



      USER.debug_is_free = 0;
      USER.next_idx = access_users_first_idx;
      access_users_first_idx = USER_idx;

      MAP = USER_idx + 1;
      memcpy(USER.hash, hash, sizeof USER.hash);

      USER.expire_day = expire_day;

      break;
    } else {
      u16 USER_idx = MAP - 1;
      if (memcmp(USER.hash, hash, sizeof USER.hash) == 0) {
        DEBUG("MAP_idx:%u USER_idx:%hu Update existing", MAP_idx, USER_idx);
        USER.expire_day = expire_day;
        break;
      } else {
        DEBUG("MAP_idx:%u USER_idx:%hu Update existing", MAP_idx, USER_idx);
      }
    }
  }
}

void __access_requested_payload(struct access_HashPayload * payload, char * rfid, char * pin);

void __access_requested_payload(struct access_HashPayload * payload, char * rfid, char * pin) {
  memcpy(payload->salt, salt, sizeof payload->salt);
  memcpy(payload->rfid, rfid, sizeof payload->rfid);
  memcpy(payload->pin , pin , sizeof payload->pin );
}

u8 access_requested(char * rfid, char * pin) {

  struct access_HashPayload payload = {};
  __access_requested_payload(&payload, rfid, pin);
  INFO_BUFFER((char*)&payload, sizeof payload, "payload:");
  access_HashResult hash;
  access_hash(hash, &payload);
  INFO_HEXBUFFER(hash, sizeof(access_HashResult), "hash:");


  assert( !get_hash_mask(HASH_MAP_LEN) ); // Assert HASH_MAP_SIZE is power of 2
  assert( HASH_MAP_LEN >= USER_TABLE_LEN);

  // Lets put the cutoff at 1pm Pacific Time, that ensures that fresh
  //  expirations happen when there are likely to be most noisebridgers around
  u64  utc_sec = time(0);
  u16  pt_day = (utc_sec - 60*60*7 /* 7 hours */ ) / 60*60*24;

  u32 map_first_tombstone = (u32)-1;
  for (u32 MAP_idx = get_hash_i(hash);; MAP_idx = get_hash_mask(MAP_idx + 1) ) {
    DEBUG("MAP_idx: %u", MAP_idx);
    if (MAP == (u16)-1 && map_first_tombstone == (u32)-1) {
      DEBUG("Hit tombstone");
      map_first_tombstone = MAP_idx;
    } else if (MAP == 0) {
      DEBUG("Hash not in our map");
      return 0;
    } else {
      u16 USER_idx = MAP - 1;
      if (memcmp(USER.hash, hash, sizeof USER.hash) == 0) { // Found
        if (map_first_tombstone != (u32)-1) {
          DEBUG("We hit a tombstone on the way, lets go ahead and swap this entry with that one");
          MAP = (u16)-1;
          MAP_idx = map_first_tombstone;
          MAP = USER_idx + 1;
        }
        DEBUG("expire: %u %u", pt_day, USER.expire_day);
        if (pt_day >= USER.expire_day) {
          return 1; // Valid
        } else {
          return 0; // Expired
        }
      } // else continue
    }
  }
  __builtin_unreachable();
  return 0; // unreachable
}



static void set_mock_salt(u32 hash_i, u32 extra) {
  *(u64*)salt = (u64)extra << 32ULL | hash_i;
}

static u8 hex_to_int(char c) {
  if ('0' <= c && c <= '9') {
    return c - '0';
  } else if ('a' <= c && c <= 'f') {
    return 10 + c - 'a';
  } else if ('A' <= c && c <= 'F') {
    return 10 + c - 'A';
  } else {
    assert(0);
    __builtin_unreachable();
  }
}

static void hash_from_string(access_HashResult hash, char * str) {
  for (int i = 0; i < sizeof(access_HashResult); i++) {
    hash[i] = (hex_to_int(str[i*2+0]) << 4) | hex_to_int(str[i*2+1]);
  }
}

int main() {
  struct access_HashPayload payload = {};
  access_HashResult hash;

  set_mock_salt(0x01, 0xffffffff);
  __access_requested_payload(&payload, "rfidrfidrfidrfidrfidrf2d", "pin1231231");
  access_hash(hash, &payload);
  INFO_HEXBUFFER(hash, sizeof hash, "hash:");
  INFO("hash_i: %x", get_hash_i(hash));

  set_mock_salt(0x02, 0xffffffff);
  __access_requested_payload(&payload, "rfidrfidrfidrfidrfidrf2d", "pin1231231");
  access_hash(hash, &payload);
  INFO_HEXBUFFER(hash, sizeof hash, "hash:");
  INFO("hash_i: %x", get_hash_i(hash));

  set_mock_salt(0x01, 0xffffffff);
  __access_requested_payload(&payload, "rfidrfidrfidrfidrfidrf2d", "pin1231231");
  access_hash(hash, &payload);
  INFO_HEXBUFFER(hash, sizeof hash, "hash:");
  INFO("hash_i: %x", get_hash_i(hash));

  access_user_list_init();

  hash_from_string(hash, "02000000ffffffff6969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969");
  access_user_add(hash, 100);
  hash_from_string(hash, "02000000ffffffff6969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969");
  access_user_add(hash, 100);
  hash_from_string(hash, "02000000ff00ffff6969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969");
  access_user_add(hash, 100);
  hash_from_string(hash, "02000000ff01ffff6969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969");
  access_user_add(hash, 100);
  hash_from_string(hash, "02000000ff02ffff6969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969");
  access_user_add(hash, 100);
  hash_from_string(hash, "02000000ff01ffff6969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969");
  access_user_add(hash, 100);

  hash_from_string(hash, "ffffffffff01ffff6969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969");
  access_user_add(hash, 100);
  hash_from_string(hash, "ffffffffff02ffff6969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969");
  access_user_add(hash, 100);
  hash_from_string(hash, "ffffffffff01ffff6969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969");
  access_user_add(hash, 100);
  hash_from_string(hash, "ffffffffff02ffff6969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969");
  access_user_add(hash, 100);
  hash_from_string(hash, "ffffffffff02ffff6969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969");
  access_user_add(hash, 100);


  u8 result;
  set_mock_salt(0x01, 0xffffffff);
  result = access_requested("rfidrfidrfidrfidrfidrf2d", "pin1231231");
  INFO("result: %d", result);



  memset(salt, 0, SALT_BUF_LEN);
  strcpy(salt, "saltysalt");

  access_requested("rfidrfidrfidrfidrfidrf2d", "pin1231231");

}

// 01000000ffffffff6969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969
// 01000000ffffffff6969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969