

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

// TODO Maybe do a checksum of body, make sure it wasn't changed while email was in flight...
void email_init(struct email_Send *ctx, char const * to_addr, char const * body_,
                size_t body_len_, char const * subject) {
  fprintf(stderr, "email_init: to:%s subject:%s body_len:%zu body:\n",
    to_addr, subject, body_len_ );
  fprintf(stderr, "%s", body_);
  fprintf(stderr, "email_init end\n");
}

void email_free(struct email_Send *ctx) {
  fprintf(stderr, "email_free\n");
}

void dump_email_state() {
  fprintf(stderr, "now:%"PRIu64" email_state: IO_TIMER_MS(logging_send):%"PRIu64" sent_epoch_sec:%"PRIu64" buf_used:%u sent_bytes:%u\n",
    now_sec_value, IO_TIMER_MS(logging_send)/1000, email_sent_epoch_sec, email_buf_used, email_sent_bytes
  );
}

#define log_usage(...)   dump_email_state(); fprintf(stderr, "Usage: "#__VA_ARGS__"\n"); __VA_ARGS__
int main () {
  setlinebuf(stderr);

  fprintf(stderr, "Starting logging test\n");
  now_sec_value = 100000;

  fprintf(stderr, "Typical usage:\n");
  log_usage( INFO("First line"); );
  log_usage( INFO("Second Line"); );
  dump_email_state();

}