#include <stdio.h>
#include <string.h>
#include "logging.h"
#include "common.h"
#include "io.h"

u64 now_sec();
#ifndef now_sec
#include <time.h>
u64 now_sec() { return time(0); }
#endif

#include "email.h"
// TODO: setup gmail to delete old emails
// LOW_THRESHOLDS: We will start considering sending logs once we have accumulated this much bytes/time
char const *const supr_email_rcpt = "logging@test.test";
u32 const SUPR_EMAIL_LOW_THRESHOLD_BYTES = 1 << 14; // 16 KB
u32 const SUPR_EMAIL_RAPID_THRESHOLD_SECS = 20; // 20 Seconds  Prevent emails from being sent more often than this
u32 const SUPR_EMAIL_LOW_THRESHOLD_SECS = 60 * 1; // 1 Minute
u32 const SUPR_EMAIL_TIMEOUT_SECS = 60 * 2;       // 10 Minutes
u32 const supr_email_buf_SIZE = 1 << 22; // 4 MB  // dont increase over 24 MB, gmail has a hard limit at 25MB
char supr_email_buf[supr_email_buf_SIZE];
u32 supr_email_buf_used;
u64 supr_email_sent_epoch_sec;
u32 supr_email_sent_bytes;
struct email_Send supr_email_ctx;

enum SUPR_LOG_EMAIL_STATE_T {
  SUPR_LOG_EMAIL_STATE_NO_DATA,
  SUPR_LOG_EMAIL_STATE_COLLECTING,
  SUPR_LOG_EMAIL_STATE_SENT,
  SUPR_LOG_EMAIL_STATE_COOLDOWN,
};
enum SUPR_LOG_EMAIL_STATE_T supr_email_state;


static void poke_state_machine() {
  u64 now_epoch_sec = now_sec();
  assert(now_epoch_sec > SUPR_EMAIL_RAPID_THRESHOLD_SECS);
  assert(now_epoch_sec > SUPR_EMAIL_LOW_THRESHOLD_SECS);
  assert(now_epoch_sec > SUPR_EMAIL_TIMEOUT_SECS);

  start:
    switch (supr_email_state) {
      SWITCH_DEFAULT_IS_UNEXPECTED;
    case SUPR_LOG_EMAIL_STATE_NO_DATA: {
      if (supr_email_buf_used) {
        supr_email_sent_epoch_sec = now_epoch_sec;
        supr_email_state = SUPR_LOG_EMAIL_STATE_COLLECTING;
        goto start;
      } else {
        DEBUG("No data, do nothing");
      }
    } break;
    case SUPR_LOG_EMAIL_STATE_COLLECTING: {
      if (supr_email_buf_used >= SUPR_EMAIL_LOW_THRESHOLD_BYTES ||
          now_epoch_sec >=
              supr_email_sent_epoch_sec + SUPR_EMAIL_LOW_THRESHOLD_SECS) {
        DEBUG("one of the low thresholds are met, lets queue up an email");
        email_init(&supr_email_ctx, supr_email_rcpt, supr_email_buf,
                   supr_email_buf_used, "Logs");
        supr_email_sent_bytes = supr_email_buf_used;
        supr_email_sent_epoch_sec = now_epoch_sec;
        IO_TIMER_MS(logging_send) =
            (supr_email_sent_epoch_sec + SUPR_EMAIL_TIMEOUT_SECS) * 1000;
        supr_email_state = SUPR_LOG_EMAIL_STATE_SENT;
      } else {
        DEBUG("Waiting on threshold timeout or more data");
        IO_TIMER_MS(logging_send) =
            (supr_email_sent_epoch_sec + SUPR_EMAIL_LOW_THRESHOLD_SECS) * 1000;
      }
    } break;
    case SUPR_LOG_EMAIL_STATE_SENT: {
      assert(IO_TIMER_MS(logging_send) ==
             (supr_email_sent_epoch_sec + SUPR_EMAIL_TIMEOUT_SECS) * 1000);
      if (supr_email_sent_epoch_sec + SUPR_EMAIL_TIMEOUT_SECS <=
          now_epoch_sec) {
        DEBUG("If our email is timing out, lets abort it");
        fprintf(stderr, "Error: log email took too long, aborting\n");
        email_free(&supr_email_ctx);
        supr_email_sent_bytes = 0;
        IO_TIMER_MS(logging_send) = -1;
        supr_email_state = SUPR_LOG_EMAIL_STATE_COOLDOWN;
        goto start;
      } else {
        DEBUG("Email is in flight, lets not do anything for now");
      }
    } break;
    case SUPR_LOG_EMAIL_STATE_COOLDOWN: {
      if (now_epoch_sec <
          supr_email_sent_epoch_sec + SUPR_EMAIL_RAPID_THRESHOLD_SECS) {
        DEBUG("Were cooling down");
        IO_TIMER_MS(logging_send) =
            (supr_email_sent_epoch_sec + SUPR_EMAIL_RAPID_THRESHOLD_SECS) *
            1000;
      } else {
        if (supr_email_buf_used) {
          supr_email_state = SUPR_LOG_EMAIL_STATE_COLLECTING;
        } else {
          supr_email_state = SUPR_LOG_EMAIL_STATE_NO_DATA;
        }
        goto start;
      }
    } break;
    }
}

void email_done(u8 success) {
  if (success) {
    memmove(supr_email_buf, supr_email_buf + supr_email_sent_bytes,
            supr_email_sent_bytes);
    supr_email_buf_used -= supr_email_sent_bytes;
  }
  supr_email_sent_bytes = 0;
  email_free(&supr_email_ctx);
  supr_email_state = SUPR_LOG_EMAIL_STATE_COOLDOWN;
  poke_state_machine();
}

void logging_send_timeout() { poke_state_machine(); }
void buf_add_step1(char**buf_, usz*buf_space_left) {
  *buf_ = supr_email_buf + supr_email_buf_used;
  *buf_space_left = supr_email_buf_SIZE - supr_email_buf_used;
}
void buf_add_step2(usz new_space_used) {
  assert(new_space_used <= supr_email_buf_SIZE - supr_email_buf_used);
  if (memchr(supr_email_buf + supr_email_buf_used, '\n', new_space_used)) {
    supr_email_buf_used += new_space_used;
    poke_state_machine();
  } else {
    supr_email_buf_used += new_space_used;
  }
}