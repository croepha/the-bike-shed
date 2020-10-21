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
typedef u8 access_HashResult[64];
void __access_hash(access_HashResult result, struct access_HashPayload * payload);

void access_user_add(access_HashResult hash, u16 expire_day);
void access_user_list_init(void);
void access_idle_maintenance(void);
u8   access_requested(char * rfid, char * pin, u16 * days_left);
extern u16 * access_idle_maintenance_prev;
extern u16 access_users_first_idx;
u16 access_now_day(void);


extern char access_salt[];
void __access_requested_payload(struct access_HashPayload * payload, char * rfid, char * pin);
