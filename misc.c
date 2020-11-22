
#include "common.h"

u8 time_is_open(u32 day_sec_open, u32 day_sec_close, u64 now_ms) {
    s64 DAY_SECS = 24 * 60 * 60;
    s64 pt_sec = now_ms / 1000 - (7 * 60 * 60);
    s32 day_sec = pt_sec % (DAY_SECS);
    if (day_sec_open < day_sec_close) {
        if (day_sec_open <= day_sec && day_sec < day_sec_close) {
            return 1;
        }
    } else if (day_sec_open > day_sec_close) {
        if (day_sec_open <= day_sec || day_sec < day_sec_close) {
            return 1;
        }
    }
    return 0;
}

