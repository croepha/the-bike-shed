

#include "io.h"
#include <unistd.h>
#define LOGGING_USE_EMAIL 1
#include <stdio.h>

#include "logging.h"
#include "supervisor.h"
#include <inttypes.h>



void logging_send_timeout();

u32 supr_email_low_threshold_bytes = 1 << 14; // 16 KB
u32 supr_email_rapid_threshold_secs = 20; // 20 Seconds  Prevent emails from being sent more often than this
u32 supr_email_low_threshold_secs = 60 * 1; // 1 Minute
u32 supr_email_timeout_secs = 60 * 2;       // 2 Minutes

void supr_email_done_hook() {}

u64 now_ms_value;
char * email_rcpt = "logging@test.test";

extern u32 supr_email_buf_used;
extern u64 supr_email_sent_epoch_sec;
extern u32 supr_email_sent_bytes;
enum SUPR_LOG_EMAIL_STATE_T {
  SUPR_LOG_EMAIL_STATE_NO_DATA,
  SUPR_LOG_EMAIL_STATE_COLLECTING,
  SUPR_LOG_EMAIL_STATE_SENT,
  SUPR_LOG_EMAIL_STATE_COOLDOWN,
};
extern enum SUPR_LOG_EMAIL_STATE_T supr_email_state;

static void dump_email_state() {
  INFO(  "now:%" PRIu64 " email_state:%d IO_TIMER_MS(logging_send):%" PRIu64
          " sent_epoch_sec:%" PRIu64 " buf_used:%u sent_bytes:%u",
          now_ms_value / 1000, supr_email_state, IO_TIMER_MS_DEBUG_GET(logging_send) / 1000,
          supr_email_sent_epoch_sec, supr_email_buf_used,
          supr_email_sent_bytes);
}

static void reset_email_state() {
    now_ms_value  = 100000000;
    IO_TIMER_MS_SET(logging_send, -1);
    supr_email_sent_epoch_sec = 0;
    supr_email_buf_used = 0;
    supr_email_sent_bytes = 0;
    supr_email_state = 0;
 }



u64 now_ms() {
  return now_ms_value;
}

//#include "logging.c"
#include <string.h>
#include "email.h"
#include "io_curl.h"

struct SupervisorCurlCtx {
  enum _io_curl_type curl_type;
};
extern struct SupervisorCurlCtx supr_email_email_ctx;

CURL* __io_curl_create_handle() {
  return 0;
}

//u64 io_timers_epoch_ms[_io_timer_logging_send + 1];

u64 __fake_io_timers_ms__logging_send;
u64 __io_debug_timer_ms_get__logging_send(void);
u64 __io_debug_timer_ms_get__logging_send(void) { return __fake_io_timers_ms__logging_send; }
void __io_timer_ms_set__logging_send(u64 value_ms);
void __io_timer_ms_set__logging_send(u64 value_ms) { __fake_io_timers_ms__logging_send = value_ms; }



#define SPLIT_MEM(s, s_end, chr, var) for (typeof(*s) * var = s, * var ## _end; \
  __SPLIT_MEM(s_end, &var, &var ## _end, chr) ; var = var ## _end + 1)
static u8 __SPLIT_MEM(char const * s_end, char const ** var, char const ** var_end, char chr) {
start:
  if (s_end <= *var) { return 0; }
  *var_end = memchr(*var, chr, s_end-*var);
  if (!*var_end) { *var_end = s_end; }
  if (*var == *var_end) {
    *var = *var_end + 1;
    goto start;
  }
  return 1;
}

static void test_SPLIT_MEM() {
  INFO("test_SPLIT_MEM 1:");
  char const * s = "1  2 3 4 5 6 7 8 9 ";
  SPLIT_MEM(s, s + strlen(s), ' ', num) {
    int num_len = num_end - num;
    INFO("%.*s|", num_len, num);
  }
  INFO("test_SPLIT_MEM done");

}

// TODO Maybe do a checksum of body, make sure it wasn't changed while email was in flight...
void email_init(struct email_Send *ctx, CURL*easy, char const * to_addr, char const * body_,
                size_t body_len_, char const * subject) {
  INFO("email_init: to:%s subject:%s body_len:%zu body:",
    to_addr, subject, body_len_ );

  // char const * body_end = body_ + body_len_;
  // while (body_end > line && (line_end = memchr(line, '\n', body_end-line))) {
  //   int line_len = line_end - line;
  //   fprintf(stderr, "%.*s", line_len, body_);
  //   line = line_end + 1;
  // }

  // for (char const * line_end = mem)

  // for (char const* line = body_; line && (line-body_) < body_len_; line = memchr(line, '\n', body_end-line) )  {
  //   body_
  // }

  SPLIT_MEM(body_, body_ + body_len_, '\n', line) {
    int line_len = line_end - line;
    if (line_len > 128) {
      INFO("%.*s...Truncated...%.*s",
        (int)40, line, (int)40, line+(line_len-40) );
    } else {
      INFO("%.*s", (int)line_len, line);
    }
  }
  INFO("email_init end");
}

void email_free(struct email_Send *ctx) {
  INFO("email_free");
}

void dump_email_state();
void reset_email_state();

static void timer_skip() {
  now_ms_value = IO_TIMER_MS_DEBUG_GET(logging_send);
  assert(now_ms_value < 1000000000000);
  logging_send_timeout();
  now_ms_value += 1000;
}


__attribute__((__format__ (__printf__, 1, 2)))
static void test_printf(char const * fmt, ...) {
  char * buf; usz    space_left;
  supr_email_add_data_start(&buf, &space_left);
  va_list va;
  va_start(va, fmt);
  s32 r = vsnprintf(buf, space_left, fmt, va);
  va_end(va);
  if (r >= space_left) {
    fprintf(stderr, "r >= space_left\n");
    abort();
  }
  supr_email_add_data_finish(r);
  now_ms_value += 1000;

}

#define TEST_INFO(fmt, ...) test_printf(fmt "\n" , ##__VA_ARGS__)

//#define log_usage(...)   INFO("Usage: %s", #__VA_ARGS__); __VA_ARGS__; dump_email_state()
#define log_usage(...)   fprintf(stderr, "Usage: %s\n", #__VA_ARGS__); __VA_ARGS__; dump_email_state()
int main () {
  setlinebuf(stderr); alarm(1);

  test_SPLIT_MEM();

//  INFO("Starting logging test");
  fprintf(stderr, "Starting logging test\n");

  if (1) {
    //INFO("Typical usage:");
    fprintf(stderr, "Typical usage:\n");
    log_usage( reset_email_state(); );
    log_usage( TEST_INFO("First line"); );
    log_usage( TEST_INFO("Second Line"); );

    log_usage( timer_skip(); );

    log_usage( TEST_INFO("Third line"); );
    log_usage( TEST_INFO("Fourth Line"); );

    now_ms_value += 1000;
    log_usage( __supervisor_email_io_curl_complete(0, CURLE_OK, &supr_email_email_ctx.curl_type) );

    log_usage( TEST_INFO("line 6"); );
    log_usage( TEST_INFO("line 7"); );
    now_ms_value += 1000;

    INFO("Expect cooldown finish");
    log_usage( timer_skip(); );
    INFO("Expect email send");
    log_usage( timer_skip(); );
    INFO("Expect email timeout");
    log_allowed_fails = 1;
    log_usage( timer_skip(); );
    assert(log_allowed_fails == 0);

  }
  // return -1;

  if (1) {
    fprintf(stderr, "Large logs\n");
    //INFO("Large logs");
    log_usage( reset_email_state(); );

    char big_log[1024];
    memset(big_log, 'B', sizeof big_log);
    big_log[sizeof big_log - 1] = 0;

    for (int i=0; i<20; i++) {
      log_usage( TEST_INFO("Big: %s", big_log); );
    }
  }

//  return -1;
}