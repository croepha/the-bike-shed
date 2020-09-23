#pragma once
#include "common.h"

struct access_HashPayload {
  u8 salt[64];
  u8 rfid[24];
  u8 pin[10];
};
typedef u8 access_HashResult[64];
void access_hash(access_HashResult * result, struct access_HashPayload * payload);


