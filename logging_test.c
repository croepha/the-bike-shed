

#include "io.h"
#define LOGGING_USE_EMAIL 1
#define now_sec now_sec

u64 now_sec_value;
u64 now_sec() {
  return now_sec_value++;
}

#include "logging.c"
#include <inttypes.h>

u64 io_timers_epoch_ms[_io_timer_logging_send + 1];


void email_init(struct email_Send *ctx, char const * to_addr, char const * body_,
                size_t body_len_, char const * subject) {
  fprintf(stderr, "email_init: to:%s subject:%s body_len:%zu body:\n",
    to_addr, subject, body_len_ );
  fprintf(stderr, "%s", body_);
  fprintf(stderr, "email_init end\n");
}


void dump_email_state() {
  fprintf(stderr, "email_state: IO_TIMER_MS(logging_send):%"PRIu64" last_sent_epoch_sec:%"PRIu64" log_email_buf_used:%u sent_size:%u\n",
    IO_TIMER_MS(logging_send), last_sent_epoch_sec, log_email_buf_used, sent_size
  );
}

#define log_usage(...) fprintf(stderr, "Usage: "#__VA_ARGS__"\n"); __VA_ARGS__
int main () {
  setlinebuf(stderr);

  fprintf(stderr, "Starting logging test\n");
  dump_email_state();

  fprintf(stderr, "Typical usage:\n");
  log_usage( INFO("First line"); );
  dump_email_state();
  log_usage( INFO("Second Line"); );
  dump_email_state();


}