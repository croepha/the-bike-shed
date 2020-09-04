

#include "io.h"
#include <unistd.h>
#define LOGGING_USE_EMAIL 1
#define now_sec now_sec
#include <stdio.h>

#include "logging.h"
#include <inttypes.h>

#undef  DEBUG
#undef  ERROR
//#define DEBUG(...) fprintf(stderr, "TRACE: " __VA_ARGS__); fprintf(stderr, "\n")
#define DEBUG(...)
#define ERROR(...) fprintf(stderr, "ERROR: " __VA_ARGS__); fprintf(stderr, "\n"); abort();

u64 now_sec_value;

extern u32 email_buf_used;
extern u64 email_sent_epoch_sec;
extern u32 email_sent_bytes;
enum LOG_EMAIL_STATE_T {
  LOG_EMAIL_STATE_NO_DATA,
  LOG_EMAIL_STATE_COLLECTING,
  LOG_EMAIL_STATE_SENT,
  LOG_EMAIL_STATE_COOLDOWN,
};
extern enum LOG_EMAIL_STATE_T email_state;


void dump_email_state() {
  fprintf(stderr, "now:%"PRIu64" email_state:%d IO_TIMER_MS(logging_send):%"PRIu64" sent_epoch_sec:%"PRIu64" buf_used:%u sent_bytes:%u\n",
    now_sec_value, email_state, IO_TIMER_MS(logging_send)/1000, email_sent_epoch_sec, email_buf_used, email_sent_bytes
  );
}

void reset_email_state() {
    now_sec_value  = 100000;
    IO_TIMER_MS(logging_send) = -1;
    email_sent_epoch_sec = 0;
    email_buf_used = 0;
    email_sent_bytes = 0;
    email_state = 0;
 }



u64 now_sec() {
  return now_sec_value++;
}

//#include "logging.c"
#include <string.h>
#include "email.h"
void email_done(u8 success);

u64 io_timers_epoch_ms[_io_timer_logging_send + 1];

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

void test_SPLIT_MEM() {
  fprintf(stderr, "test_SPLIT_MEM 1:");

  char const * s = "1  2 3 4 5 6 7 8 9 ";
  SPLIT_MEM(s, s + strlen(s), ' ', num) {
    int num_len = num_end - num;
    fprintf(stderr, "%.*s|", num_len, num);
  }
  fprintf(stderr, "\n");

}

// TODO Maybe do a checksum of body, make sure it wasn't changed while email was in flight...
void email_init(struct email_Send *ctx, char const * to_addr, char const * body_,
                size_t body_len_, char const * subject) {
  fprintf(stderr, "email_init: to:%s subject:%s body_len:%zu body:\n",
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
      fprintf(stderr, "%.*s...Truncated...%.*s\n",
        (int)40, line, (int)40, line+(line_len-40) );
    } else {
      fprintf(stderr, "%.*s\n", (int)line_len, line);
    }
  }
  fprintf(stderr, "email_init end\n");
}

void email_free(struct email_Send *ctx) {
  fprintf(stderr, "email_free\n");
}

void dump_email_state();
void reset_email_state();

void timer_skip() {
  now_sec_value = IO_TIMER_MS(logging_send)/1000;
  if (now_sec_value > 100000000000) {
    fprintf(stderr, "now_sec_value > 100000000000\n");
    abort();
  }


  logging_send_timeout();
}


void buf_add_step1(char**buf_, usz*buf_space_left);
void buf_add_step2(usz new_space_used);

__attribute__((__format__ (__printf__, 1, 2)))
void test_printf(char const * fmt, ...) {
  char * buf; usz    space_left;
  buf_add_step1(&buf, &space_left);
  va_list va;
  va_start(va, fmt);
  s32 r = vsnprintf(buf, space_left, fmt, va);
  va_end(va);
  if (r >= space_left) {
    fprintf(stderr, "r >= space_left\n");
    abort();
  }
  buf_add_step2(r);
}
#undef INFO
#define INFO(fmt, ...) test_printf(fmt "\n" , ##__VA_ARGS__)


#define log_usage(...)   fprintf(stderr, "Usage: %s\n", #__VA_ARGS__); __VA_ARGS__; dump_email_state()
int main () {
  setlinebuf(stderr); alarm(1);

  test_SPLIT_MEM();

  fprintf(stderr, "Starting logging test\n");

  if (1) {
    fprintf(stderr, "Typical usage:\n");
    log_usage( reset_email_state(); );
    log_usage( INFO("First line"); );
    log_usage( INFO("Second Line"); );

    log_usage( timer_skip(); );

    log_usage( INFO("Third line"); );
    log_usage( INFO("Fourth Line"); );

    log_usage( email_done(1); );

    log_usage( INFO("line 6"); );
    log_usage( INFO("line 7"); );

    DEBUG("Expect cooldown finish");
    log_usage( timer_skip(); );
    DEBUG("Expect email send");
    log_usage( timer_skip(); );
    DEBUG("Expect email timeout");
    log_usage( timer_skip(); );

  }
  // return -1;

  if (1) {
    fprintf(stderr, "Large logs\n");
    log_usage( reset_email_state(); );

    char big_log[1024];
    memset(big_log, 'B', sizeof big_log);
    big_log[sizeof big_log - 1] = 0;

    for (int i=0; i<20; i++) {
      log_usage( INFO("Big: %s", big_log); );
    }
  }

//  return -1;
}