#define LOG_DEBUG

#include <string.h>
#include "logging.h"
#include "access.h"

char access_salt[SALT_BUF_LEN] = "saltysalt";
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




static u16 access_users_first_free_idx;
u16 access_users_first_idx;
u16 * access_idle_maintenance_prev;

#define USER (access_users_space[USER_idx])
#define MAP  (        access_map[MAP_idx ])


u16 access_now_day() {
  // Lets put the cutoff at 1pm Pacific Time, that ensures that fresh
  //  expirations happen when there are likely to be most noisebridgers around
  u64  utc_sec = now_sec();
  u64  utc_hour = utc_sec / (60*60);
  u32  pt_hour = utc_hour - 7 /* 7 hours */ ;
  u16  pt_day = pt_hour / 24;
  return pt_day;
}

static u32 get_hash_mask(u32 v) { return v & (HASH_MAP_LEN - 1); }
static u32 get_hash_i(access_HashResult hash_result) { return get_hash_mask(*(u32*)hash_result); }
static u8 user_is_expired(u16 USER_idx, u16 * days_left) {
  u16  pt_day = access_now_day();
  TRACE("expires: %u %u ", pt_day, USER.expire_day);
  if (days_left) {
    if (pt_day <= USER.expire_day) {
      *days_left = USER.expire_day - pt_day;
    } else {
      *days_left = 0;
    }
  }
  return pt_day > USER.expire_day;
}

void __access_requested_payload(struct access_HashPayload * payload, char * rfid, char * pin) {
  memcpy(payload->salt, access_salt, sizeof payload->salt);
  memcpy(payload->rfid, rfid, sizeof payload->rfid);
  memcpy(payload->pin , pin , sizeof payload->pin );
}

void access_user_list_init(void) {

  // Initialize the free list
  access_users_first_idx = -1;
  access_users_first_free_idx = -1;
  for (u16 USER_idx = 0; USER_idx < USER_TABLE_LEN; USER_idx++) {

    USER.debug_is_free = 1;
    USER.next_idx = access_users_first_free_idx;
    access_users_first_free_idx = USER_idx;

  }
  access_idle_maintenance_prev = &access_users_first_idx;
}

void access_idle_maintenance(void) {

  // Scan for users that need to be pruned
  if (*access_idle_maintenance_prev == (u16)-1) {
    return;
  }

  u16 USER_idx = *access_idle_maintenance_prev;
  //DEBUG("USER_idx:%hu", USER_idx);

  assert(!USER.debug_is_free);

  u32 map_first_tombstone = (u32)-1;

  u32 map_rage_start_idx = get_hash_i(USER.hash);
  u32 map_range_end = map_rage_start_idx;

  for (u32 MAP_idx = map_rage_start_idx;; MAP_idx = get_hash_mask(MAP_idx + 1) ) {
    if (MAP == (u16)-1) {
      if (map_first_tombstone == (u32)-1) {
        TRACE("MAP_idx:%x Hit first tombstone ", MAP_idx);
        map_first_tombstone = MAP_idx;
      }
    } else if (MAP == 0) {
      TRACE("MAP_idx:%x Hash not in our map ", MAP_idx);
      assert(0);
      map_range_end = MAP_idx;
      break; // Hash not in our map
    } else {
      map_range_end = MAP_idx;
      if(USER_idx == MAP - 1) {
        u8 is_expired;
        {
          LOGCTX(" MAP_idx:%x USER_idx:%x ", MAP_idx, USER_idx );
          is_expired = user_is_expired(USER_idx, 0);
        }
        if (is_expired) { // This entry is expired lets remove it
          TRACE("MAP_idx:%x USER_idx:%x Expired, freeing", USER_idx, MAP_idx);
          USER.debug_is_free = 1;
          *access_idle_maintenance_prev = USER.next_idx;
          USER.next_idx = access_users_first_free_idx;
          access_users_first_free_idx = USER_idx;
          MAP = (u16)-1;
        } else {
          if (map_first_tombstone != (u32)-1) {
            TRACE("MAP_idx:%x USER_idx:%x We hit a tombstone on the way, lets go ahead and swap this entry(%x) with that one ", map_first_tombstone, USER_idx, MAP_idx);
            MAP = (u16)-1;
            MAP_idx = map_first_tombstone;
            MAP = USER_idx + 1;
          }
          TRACE("MAP_idx:%x USER_idx:%x Not expired, doing nothing", USER_idx, MAP_idx);
          access_idle_maintenance_prev = &USER.next_idx;
        }
        break;
      } else {
        // TODO, should this really be a warning? or at-least an INFO?
        TRACE("MAP_idx:%x USER_idx:%x Collision, continuing", MAP_idx, USER_idx);
      }
    }
  }

  // TODO: We could Walk the map range backwards, looking for tombstones... probably not worth it though

}

void access_user_add(access_HashResult hash, u16 expire_day) {

  TRACE_HEXBUFFER(hash, 64 / 8, "hash:");

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
    if (MAP == (u16)-1) {
      if(  map_first_tombstone == (u32)-1) {
        TRACE("MAP_idx:%x Hit first tombstone ", MAP_idx);
        map_first_tombstone = MAP_idx;
      }
    } else if (MAP == 0) {
      // pop off the free list
      u16 USER_idx = access_users_first_free_idx;
      access_users_first_free_idx = USER.next_idx;
      assert(USER.debug_is_free);

      if (map_first_tombstone != (u32)-1) {
        TRACE("MAP_idx:%x USER_idx:%x Adding new, replacing tombstone instead of making new slot, got to %x ", map_first_tombstone, USER_idx, MAP_idx);
        MAP_idx = map_first_tombstone;
      } else {
        TRACE("MAP_idx:%x USER_idx:%x Adding new, in new slot", MAP_idx, USER_idx);
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
      assert(!USER.debug_is_free);
      if (memcmp(USER.hash, hash, sizeof USER.hash) == 0) {

        if (map_first_tombstone != (u32)-1) {
          TRACE("MAP_idx:%x USER_idx:%x We hit a tombstone on the way, lets go ahead and swap this entry(%x) with that one ", map_first_tombstone, USER_idx, MAP_idx);
          MAP = (u16)-1;
          MAP_idx = map_first_tombstone;
          MAP = USER_idx + 1;
        } else {
          TRACE("MAP_idx:%x USER_idx:%x Adding new, in new slot", MAP_idx, USER_idx);
        }

        TRACE("MAP_idx:%x USER_idx:%x Update existing", MAP_idx, USER_idx);
        USER.expire_day = expire_day;
        break;
      } else {
        // TODO, should this really be a warning? or at-least an INFO?
        TRACE("MAP_idx:%x USER_idx:%x Collision, continuing", MAP_idx, USER_idx);
      }
    }
  }
}

u8 access_requested(char * rfid, char * pin, u16 * days_left) {
  *days_left = (u16)-1;
  struct access_HashPayload payload = {};
  __access_requested_payload(&payload, rfid, pin);
  // INFO_BUFFER((char*)&payload, sizeof payload, "payload:");
  access_HashResult hash;
  access_hash(hash, &payload);

  TRACE_HEXBUFFER(hash, 64 / 8, "hash:");

  assert( !get_hash_mask(HASH_MAP_LEN) ); // Assert HASH_MAP_SIZE is power of 2
  assert( HASH_MAP_LEN >= USER_TABLE_LEN);

  u32 map_first_tombstone = (u32)-1;
  for (u32 MAP_idx = get_hash_i(hash);; MAP_idx = get_hash_mask(MAP_idx + 1) ) {
    if (MAP == (u16)-1) {
      if(map_first_tombstone == (u32)-1) {
        TRACE("MAP_idx:%x Hit first tombstone ", MAP_idx);
        map_first_tombstone = MAP_idx;
      }
    } else if (MAP == 0) {
      TRACE("MAP_idx:%x Hash not in our map ", MAP_idx);
      return 0;
    } else {
      u16 USER_idx = MAP - 1;
      assert(!USER.debug_is_free);
      if (memcmp(USER.hash, hash, sizeof USER.hash) == 0) { // Found
        if (map_first_tombstone != (u32)-1) {
          TRACE("MAP_idx:%x USER_idx:%x We hit a tombstone on the way, lets go ahead and swap this entry(%x) with that one ",
               map_first_tombstone, USER_idx, MAP_idx);
          MAP = (u16)-1;
          MAP_idx = map_first_tombstone;
          MAP = USER_idx + 1;
        }
        LOGCTX(" MAP_idx:%x USER_idx:%x ", MAP_idx, USER_idx );
        return !user_is_expired(USER_idx, days_left);
      } else {
        // TODO, should this really be a warning? or at-least an INFO?
        TRACE("MAP_idx:%x USER_idx:%x Collision, continuing", MAP_idx, USER_idx);
      }
    }
  }
  __builtin_unreachable();
  return 0; // unreachable
}

