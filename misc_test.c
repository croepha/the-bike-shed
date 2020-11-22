

#include <time.h>
#include <inttypes.h>
#include "logging.h"

static void test_is_open(u32 day_sec_open, u32 day_sec_close) {
    u64 start_time_ms = 1600000000000;
    u64 end_time_ms   = start_time_ms + MS_PER_DAY * 2;

    INFO("open:%02d:%02d:%02d closed:%02d:%02d:%02d",
        day_sec_open/60/60,
        day_sec_open/60%60,
        day_sec_open%60,
        day_sec_close/60/60,
        day_sec_close/60%60,
        day_sec_close%60);

    setenv("TZ", "PST8PDT", 1);
    tzset();

    enum { state_start, state_open, state_closed } last_state = state_start;

    for (u64 sim_ms = start_time_ms; sim_ms < end_time_ms; sim_ms += 1000) {
        char ftime[30];

        setenv("TZ", "PST8PDT", 1);
        tzset();
        time_t mtt = sim_ms / 1000;
        struct tm * mt = localtime(&mtt);
        strftime(ftime, sizeof(ftime), "%Z %Y-%m-%d %H:%M:%S", mt);

        u32 secs_til_open_ = secs_til_open(day_sec_open, day_sec_close, sim_ms);
        if (!secs_til_open_ && last_state != state_open) {
            last_state = state_open;
            INFO("%s %"PRIu64" NOW OPEN", ftime, sim_ms);
        } else if (secs_til_open_ && last_state != state_closed) {
            last_state = state_closed;
            INFO("%s %"PRIu64" NOW CLOSED", ftime, sim_ms);
        }

        if (0 <  secs_til_open_ && secs_til_open_ < 10) {
            INFO("%s %"PRIu64" Secs till open %u", ftime, sim_ms, secs_til_open_);
        }
    }
}

int main () {
    test_is_open( 6 * SEC_PER_HOUR,  7 * SEC_PER_HOUR);
    test_is_open( 7 * SEC_PER_HOUR,  6 * SEC_PER_HOUR);
    test_is_open(16 * SEC_PER_HOUR, 17 * SEC_PER_HOUR);
    test_is_open(17 * SEC_PER_HOUR, 16 * SEC_PER_HOUR);
}