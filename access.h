#pragma once
#include "common.h"


struct access_HashPayload {
  u8 salt[64];
  u8 rfid[24];
  u8 pin[10];
};
typedef u8 access_HashResult[64];
void access_hash(access_HashResult result, struct access_HashPayload * payload);

void access_user_add(access_HashResult hash, u16 expire_day);
void access_user_list_init(void);
void access_idle_maintenance(void);
u8   access_requested(char * rfid, char * pin);
extern u16 * access_idle_maintenance_prev;
extern u16 access_users_first_idx;


static usz const SALT_BUF_LEN = 64;
extern char access_salt[];
void __access_requested_payload(struct access_HashPayload * payload, char * rfid, char * pin);
