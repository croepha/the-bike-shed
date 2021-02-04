
#include <time.h>
#include "common.h"

u32 secs_til_open(u32 day_sec_open_, u32 day_sec_close_, u64 now_ms) {

  s64 DAY_SECS = 24 * 60 * 60;

  struct tm _tm;
  time_t now_sec = now_ms / 1000;
  localtime_r(&now_sec, &_tm);
  s64 day_sec = _tm.tm_hour * 60 * 60 + _tm.tm_min * 60 + _tm.tm_sec;

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