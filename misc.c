
#include "common.h"

u32 secs_til_open(u32 day_sec_open_, u32 day_sec_close_, u64 now_ms) {
  s64 DAY_SECS = 24 * 60 * 60;
  s64 pt_sec = now_ms / 1000 - (7 * 60 * 60);
  s32 day_sec = pt_sec % (DAY_SECS);
  if (day_sec_open_ < day_sec_close_) {
    if (day_sec_open_ <= day_sec && day_sec < day_sec_close_) {
      return 0;
    }
  } else if (day_sec_open_ > day_sec_close_) {
    if (day_sec_open_ <= day_sec || day_sec < day_sec_close_) {
      return 0;
    }
  }
  return (day_sec_open_ + DAY_SECS - day_sec) % DAY_SECS;
}