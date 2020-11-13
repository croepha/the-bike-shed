#define LOG_DEBUG

#include <string.h>
#include "logging.h"
#include "access.h"

char access_salt[SALT_BUF_LEN] = "saltysalt";
static usz const HASH_MAP_LEN  = 1 << 18; // 256Ki
static access_user_IDX const USER_TABLE_LEN = 1 << 13; // 8192


struct accessUser access_users_space[USER_TABLE_LEN];
access_user_IDX access_map[HASH_MAP_LEN];




static access_user_IDX access_users_first_free_idx;
access_user_IDX access_users_first_idx;
access_user_IDX * access_idle_maintenance_prev;

// TODO make sure that maintenence is unit tested

#define USER (access_users_space[USER_idx])
#define MAP  (        access_map[MAP_idx ])


u16 access_now_day() {
  // Lets put the cutoff at 1pm Pacific Time, that ensures that fresh
  //  expirations happen when there are likely to be most noisebridgers around
  u64  utc_sec = now_sec();
  u64  utc_hour = utc_sec / (60*60);
  u32  pt_hour = utc_hour - 7 /* 7 hours */ ;
  u32  access_hour = pt_hour + 13 /* 1PM */ ;
  u16  access_day = access_hour / 24;
  return access_day;
}

static u32 get_hash_mask(u32 v) { return v & (HASH_MAP_LEN - 1); }
static u32 get_hash_i(access_HashResult hash_result) { return get_hash_mask(*(u32*)hash_result); }
s32 access_user_days_left(access_user_IDX USER_idx) {
  u16  pt_day = access_now_day();
  TRACE("expires: %u %u ", pt_day, USER.expire_day);

  u8 is_admin = (
    USER.expire_day == access_expire_day_magics_Adder       ||
    USER.expire_day == access_expire_day_magics_NewAdder    ||
    USER.expire_day == access_expire_day_magics_Extender    ||
    USER.expire_day == access_expire_day_magics_NewExtender
  );
  if (is_admin) {
    return (u16)-1;
  } else {
    return USER.expire_day - pt_day;
  }
}


void access_hash(access_HashResult hash, char * rfid, char * pin) {
  struct access_HashPayload payload = {};
  memcpy(payload.salt, access_salt, sizeof payload.salt);
  memcpy(payload.rfid, rfid, sizeof payload.rfid);
  memcpy(payload.pin , pin , sizeof payload.pin );
  __access_hash(hash, &payload);
}

u8 access_requested(char * rfid, char * pin, u16 * days_left) {
  *days_left = (u16)-1;
  access_HashResult hash;
  access_hash(hash, rfid, pin);
  access_user_IDX USER_idx = access_user_lookup(hash);
  LOGCTX(" USER_idx:%x ", USER_idx );
  if (USER_idx != access_user_NOT_FOUND) {
    s32 _dl = access_user_days_left(USER_idx);
    if (_dl < 0) {
      if (days_left) *days_left = 0;
      return 0;
    } else {
      if (days_left) *days_left = _dl;
      return 1;
    }
  }
  return 0;
}


void access_user_list_init(void) {

  // Initialize the free list
  access_users_first_idx = access_user_NOT_FOUND;
  access_users_first_free_idx = access_user_NOT_FOUND;
  for (access_user_IDX USER_idx = 0; USER_idx < USER_TABLE_LEN; USER_idx++) {

    USER.debug_is_free = 1;
    USER.next_idx = access_users_first_free_idx;
    access_users_first_free_idx = USER_idx;

  }
  access_idle_maintenance_prev = &access_users_first_idx;
}

enum {
  map_EMPTY = (u16) 0,
  map_TOMB  = (u16)-1,
};


// void access_prune_not_new(void) {
//   for (access_user_IDX user_next = access_users_first_idx;;) {
//     access_user_IDX USER_idx = user_next;
//     user_next = USER.next_idx;
//     if (USER_idx == access_user_NOT_FOUND) { break; }

//     switch (USER.expire_day) {
//       case access_expire_day_magics_Adder: {
//         access_delete_user(USER_idx);
//       } break;
//       case access_expire_day_magics_NewAdder: {
//         USER.expire_day = access_expire_day_magics_Adder;
//       } break;
//       case access_expire_day_magics_Extender: {
//         access_delete_user(USER_idx);
//       } break;
//       case access_expire_day_magics_NewExtender: {
//         USER.expire_day = access_expire_day_magics_Extender;
//       } break;
//     }
//   }
// }


static u32 __user_map_lookup(access_HashResult hash) {
  TRACE_HEXBUFFER(hash, 64 / 8, "hash:");

  assert( !get_hash_mask(HASH_MAP_LEN) ); // Assert HASH_MAP_SIZE is power of 2
  assert( HASH_MAP_LEN >= USER_TABLE_LEN);

  u32 map_first_tombstone = (u32)-1;
  for (u32 MAP_idx = get_hash_i(hash);; MAP_idx = get_hash_mask(MAP_idx + 1) ) {
    if (MAP == map_TOMB) {
      if(map_first_tombstone == (u32)-1) {
        TRACE("MAP_idx:%x Hit first tombstone ", MAP_idx);
        map_first_tombstone = MAP_idx;
      }
    } else if (MAP == map_EMPTY) {
      TRACE("MAP_idx:%x Hash not in our map ", MAP_idx);
      return MAP_idx;
    } else {
      u16 USER_idx = MAP - 1;
      assert(!USER.debug_is_free);
      if (memcmp(USER.hash, hash, sizeof USER.hash) == 0) { // Found
        if (map_first_tombstone != (u32)-1) {
          TRACE("MAP_idx:%x USER_idx:%x We hit a tombstone on the way, lets go ahead and swap this entry(%x) with that one ",
               map_first_tombstone, USER_idx, MAP_idx);
          MAP = map_TOMB;
          MAP_idx = map_first_tombstone;
          MAP = USER_idx + 1;
        }
        TRACE("returning MAP_idx:%x", MAP_idx);
        return MAP_idx;
      } else {
        // TODO, should this really be a warning? or at-least an INFO?
        TRACE("MAP_idx:%x USER_idx:%x Collision, continuing", MAP_idx, USER_idx);
      }
    }
  }
  __builtin_unreachable();
  ERROR("got to unreachable code");
  return -1; // unreachable
}


void access_idle_maintenance(void) {

  // Scan for users that need to be pruned
  if (*access_idle_maintenance_prev == access_user_NOT_FOUND) {
    return;
  }

  access_user_IDX USER_idx = *access_idle_maintenance_prev;
  //DEBUG("USER_idx:%hu", USER_idx);
  assert(!USER.debug_is_free);

  LOGCTX(" USER_idx:%x ", USER_idx );

  u32 MAP_idx = __user_map_lookup(USER.hash);
  LOGCTX(" MAP_idx:%x ", MAP_idx );

  // Lets remove people if they have been expired for at-least 60 days
  u8 is_expired = access_user_days_left(USER_idx) <  -60;

  if (is_expired) { // This entry is expired lets remove it


    if (MAP_idx == (u32)-1) {
      access_idle_maintenance_prev = &USER.next_idx;
      ERROR();
      return;
    }
    if (MAP == map_EMPTY || MAP == map_TOMB) {
      ERROR();
      return;
    }

    TRACE("Expired, freeing");
    USER.debug_is_free = 1;
    *access_idle_maintenance_prev = USER.next_idx;
    USER.next_idx = access_users_first_free_idx;
    access_users_first_free_idx = USER_idx;
    MAP = (u16)-1;

  } else {
    TRACE("Not expired, doing nothing");
    access_idle_maintenance_prev = &USER.next_idx;
  }

  // TODO: We could Walk the map range backwards, looking for tombstones... probably not worth it though

}



// Returns error message if any

static char const * const _msg_no_space_left = "No space left";
static char const * const _msg_extend_only = "User can only extend";
static char const * const _msg_overwrite_admin = "Already admin";
static char const * const _msg_unknown_error = "Unknown error";

char const * access_user_add(access_HashResult hash, u16 expire_day, u8 extend_only, u8 overwrite_admin) {

  if (access_users_first_free_idx == access_user_NOT_FOUND) {
    ERROR("User list full");
    // Try real hard to fee up some space
    access_idle_maintenance();
    // TODO, remove this heroic code.... just give up... no sense optimizing a rare case...
    if (access_users_first_free_idx == access_user_NOT_FOUND) {
      INFO("No space could be freed, giving up");
      return _msg_no_space_left;
    }
  }

  u32 MAP_idx = __user_map_lookup(hash);
  if (MAP_idx == (u32)-1) {
    return _msg_unknown_error;
  }
  if (MAP == map_EMPTY || MAP == map_TOMB) {
    if (extend_only) {
        ERROR("extend_only");
        return _msg_extend_only;
    } else {
      u16 USER_idx = access_users_first_free_idx;
      access_users_first_free_idx = USER.next_idx;
      assert(USER.debug_is_free);

      USER.debug_is_free = 0;
      USER.next_idx = access_users_first_idx;
      access_users_first_idx = USER_idx;

      MAP = USER_idx + 1;
      memcpy(USER.hash, hash, sizeof USER.hash);

      USER.expire_day = expire_day;
      return 0;
    }
  } else {
    u16 USER_idx = MAP - 1;
    assert(!USER.debug_is_free);

    u8 is_admin = (
      USER.expire_day == access_expire_day_magics_Adder       ||
      USER.expire_day == access_expire_day_magics_NewAdder    ||
      USER.expire_day == access_expire_day_magics_Extender    ||
      USER.expire_day == access_expire_day_magics_NewExtender
    );

    if (is_admin && !overwrite_admin ) {
      ERROR("overwrite_admin");
      return _msg_overwrite_admin;
    } else {
      TRACE("MAP_idx:%x USER_idx:%x Update existing", MAP_idx, USER_idx);
      USER.expire_day = expire_day;
      return 0;
    }

  }

}

access_user_IDX access_user_lookup(access_HashResult hash) {

  u32 MAP_idx = __user_map_lookup(hash);
  if (MAP_idx == (u32)-1) {
    return access_user_NOT_FOUND;
  }
  if (MAP == map_EMPTY || MAP == map_TOMB) {
    return access_user_NOT_FOUND;
  }
  u16 USER_idx = MAP - 1;
  return USER_idx;
}

