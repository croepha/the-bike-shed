#pragma once
#include "common.h"

static usz const SALT_BUF_LEN = 64;
static usz const rfid_LEN = 24;
static usz const pin_LEN  = 10;

struct access_HashPayload {
  u8 salt [SALT_BUF_LEN];
  u8 rfid [rfid_LEN    ];
  u8 pin  [pin_LEN     ];
};

enum access_expire_day_magics {
  access_expire_day_magics_Adder       = (u16)-1,
  access_expire_day_magics_Extender    = (u16)-2,
  access_expire_day_magics_NewAdder    = (u16)-3,
  access_expire_day_magics_NewExtender = (u16)-4,
};

typedef u16 access_user_IDX;
enum {
  access_user_NOT_FOUND = (u16)-1,
};


typedef u8 access_HashResult[64];

struct accessUser {
  u8 debug_is_free;
  union {
    struct {
        access_HashResult hash;
        u16 expire_day;
    };
  };
  u16 next_idx;
} extern access_users_space[];


void __access_hash(access_HashResult result, struct access_HashPayload * payload);
void access_hash(access_HashResult hash, char * rfid, char * pin);

char const * access_user_add(access_HashResult hash, u16 expire_day, u8 extend_only, u8 overwrite_admin);
void access_user_list_init(void);
void access_idle_maintenance(void);
u8   access_requested(char * rfid, char * pin, u16 * days_left);
access_user_IDX   access_user_lookup(access_HashResult hash);
extern u16 * access_idle_maintenance_prev;
extern u16 access_users_first_idx;
u16 access_now_day(void);

extern char access_salt[];
void __access_requested_payload(struct access_HashPayload * payload, char * rfid, char * pin);
