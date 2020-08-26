

#include "io.h"
#define LOGGING_USE_EMAIL 1

#include "logging.c"

u64 io_timers_epoch_ms[_io_timer_logging_send];


void email_init(struct email_Send *ctx, char const * to_addr, char const * body_,
                size_t body_len_, char const * subject) {
  fprintf(stderr, "email_init: to:%s subject:%s body_len:%zu body:\n",
    to_addr, subject, body_len_ );
  fprintf(stderr, "%s", body_);
  fprintf(stderr, "email_init end\n");
}


#define log_usage(...) fprintf(stderr, "Usage: "#__VA_ARGS__"\n"); __VA_ARGS__
int main () {
  setlinebuf(stderr);

  fprintf(stderr, "Starting logging test\n");

  fprintf(stderr, "Typical usage:\n");
  log_usage( INFO("First line"); );
  log_usage( INFO("Second Line"); );


}