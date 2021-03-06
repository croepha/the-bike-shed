#define LOG_DEBUG
#include <string.h>

#include <time.h>

#include "logging.h"
#include "common.h"
#include "access.h"

u8 log_trace_enabled = 1;



static void set_mock_salt(u32 hash_i, u32 extra) {
  *(u64 *)access_salt = (u64)extra << 32ULL | hash_i;
}

// static u8 hex_to_int(char c) {
//   if ('0' <= c && c <= '9') {
//     return c - '0';
//   } else if ('a' <= c && c <= 'f') {
//     return 10 + c - 'a';
//   } else if ('A' <= c && c <= 'F') {
//     return 10 + c - 'A';
//   } else {
//     assert(0);
//     __builtin_unreachable();
//   }
// }

// static void hash_from_string(access_HashResult hash, char * str) {
//   for (int i = 0; i < sizeof(access_HashResult); i++) {
//     hash[i] = (hex_to_int(str[i*2+0]) << 4) | hex_to_int(str[i*2+1]);
//   }
// }

static void test_add(u32 hash_i, u32 extra, u16 expire_day) {
  access_HashResult hash;
  INFO("Add: %x %x", hash_i, extra);
  set_mock_salt(hash_i, extra);
  access_hash(hash, "rfidrfidrfidrfidrfidrf2d", "pin1231231", 0);
  access_user_add(hash, expire_day, 0, 1);
}

static u8 access_requested(char * rfid, char * pin, u16 * days_left) {
  *days_left = (u16)-1;

  access_HashResult hash;
  access_hash(hash, rfid, pin, 0);
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


static void test_req(u32 hash_i, u32 extra) {
  u8 result;
  set_mock_salt(hash_i, extra);
  u16 days_left = (u16)-1;
  result = access_requested("rfidrfidrfidrfidrfidrf2d", "pin1231231", &days_left);
  INFO("Request: %x %x result: %d", hash_i, extra, result);
}

u64 now_day = 90;
u64 now_ms(void) {
  return now_day * 24 * 60 * 60 * 1000;
}


void __access_hash(access_HashResult result, struct access_HashPayload * payload) {
    memset(result, 0x69, sizeof(access_HashResult));
    *(u64*)result = *(u64*)payload->salt;
}


int main() {

  // set_mock_salt(0x01, 0xffffffff);
  // __access_requested_payload(&payload, "rfidrfidrfidrfidrfidrf2d", "pin1231231");
  // __access_hash(hash, &payload);
  // INFO_HEXBUFFER(hash, sizeof hash, "hash:");
  // INFO("hash_i: %x", get_hash_i(hash));

  // set_mock_salt(0x02, 0xffffffff);
  // __access_requested_payload(&payload, "rfidrfidrfidrfidrfidrf2d", "pin1231231");
  // __access_hash(hash, &payload);
  // INFO_HEXBUFFER(hash, sizeof hash, "hash:");
  // INFO("hash_i: %x", get_hash_i(hash));

  // set_mock_salt(0x01, 0xffffffff);
  // __access_requested_payload(&payload, "rfidrfidrfidrfidrfidrf2d", "pin1231231");
  // __access_hash(hash, &payload);
  // INFO_HEXBUFFER(hash, sizeof hash, "hash:");
  // INFO("hash_i: %x", get_hash_i(hash));

  access_user_list_init();

  test_add(0x00000001, 0xffffffff, 110);
  test_add(0x00000002, 0xffffffff, 110);
  test_add(0x00000002, 0xffffffff, 110);
  test_add(0x00000002, 0xffff00ff, 110);
  test_add(0x00000002, 0xffff01ff, 110);
  test_add(0x00000002, 0xffff02ff, 110);
  test_add(0x00000002, 0xffff01ff, 100);
  test_add(0xffffffff, 0xffff01ff, 110);
  test_add(0xffffffff, 0xffff02ff, 110);
  test_add(0xffffffff, 0xffff03ff, 100);
  test_add(0xffffffff, 0xffff01ff, 110);
  test_add(0xffffffff, 0xffff02ff, 110);
  test_add(0xffffffff, 0xffff02ff, 110);

  test_req(0x00000001, 0xffffffff);
  test_req(0x00000002, 0xffffffff);
  test_req(0x00000002, 0xffff00ff);
  test_req(0x00000002, 0xffff01ff);
  test_req(0x00000002, 0xffff02ff);
  test_req(0xffffffff, 0xffff01ff);
  test_req(0xffffffff, 0xffff02ff);
  test_req(0xffffffff, 0xffff03ff);

  access_prune_not_new();


  INFO("First pass of maintenance, expect no frees");
  access_idle_maintenance_prev = &access_users_first_idx;
  while (*access_idle_maintenance_prev != (u16)-1) {
    access_idle_maintenance();
  }

  test_req(0x00000001, 0xffffffff);
  test_req(0x00000002, 0xffffffff);
  test_req(0x00000002, 0xffff00ff);
  test_req(0x00000002, 0xffff01ff);
  test_req(0x00000002, 0xffff02ff);
  test_req(0xffffffff, 0xffff01ff);
  test_req(0xffffffff, 0xffff02ff);
  test_req(0xffffffff, 0xffff03ff);

  now_day = 115;
  INFO("pass of maintenance, expect no frees");
  while (*access_idle_maintenance_prev != (u16)-1) {
    access_idle_maintenance();
  }
  INFO("Expect dennies");

  test_req(0x00000001, 0xffffffff);
  test_req(0x00000002, 0xffffffff);
  test_req(0x00000002, 0xffff00ff);
  test_req(0x00000002, 0xffff01ff);
  test_req(0x00000002, 0xffff02ff);
  test_req(0xffffffff, 0xffff01ff);
  test_req(0xffffffff, 0xffff02ff);
  test_req(0xffffffff, 0xffff03ff);

  now_day = 175;
  INFO("pass of maintenance, expect frees");
  access_idle_maintenance_prev = &access_users_first_idx;
  while (*access_idle_maintenance_prev != (u16)-1) {
    access_idle_maintenance();
  }
  INFO("Expect not found");

  test_req(0x00000001, 0xffffffff);
  test_req(0x00000002, 0xffffffff);
  test_req(0x00000002, 0xffff00ff);
  test_req(0x00000002, 0xffff01ff);
  test_req(0x00000002, 0xffff02ff);
  test_req(0xffffffff, 0xffff01ff);
  test_req(0xffffffff, 0xffff02ff);
  test_req(0xffffffff, 0xffff03ff);


  memset(access_salt, 0, SALT_BUF_LEN);
  strcpy(access_salt, "saltysalt");

  u16 days_left = (u16)-1;
  access_requested("rfidrfidrfidrfidrfidrf2d", "pin1231231", &days_left);

}
