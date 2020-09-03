
#include "common.h"

#ifndef now_sec
#include <time.h>
u64 now_sec() { return time(0); }
#endif

#include "email.h"
// TODO: setup gmail to delete old emails
// LOW_THRESHOLDS: We will start considering sending logs once we have accumulated this much bytes/time
static char const * const email_rcpt = "logging@test.test";
static  u32 const EMAIL_LOW_THRESHOLD_BYTES = 1<<14   ; // 16 KB
static  u32 const EMAIL_RAPID_THRESHOLD_SECS = 20    ; // 20 Seconds  Prevent emails from being sent more often than this
static  u32 const EMAIL_LOW_THRESHOLD_SECS  = 60 * 1  ; // 1 Minute
static  u32 const EMAIL_TIMEOUT_SECS        = 60 * 2  ; // 10 Minutes
static  u32 const email_buf_SIZE  = 1<<22   ; // 4 MB  // dont increase over 24 MB, gmail has a hard limit at 25MB
static char email_buf[email_buf_SIZE];
static  u32 email_buf_used;
static  u64 email_sent_epoch_sec;
static  u32 email_sent_bytes;
static   u8 recursing_error;
static   u8 internal_error;
struct email_Send email_ctx;

enum LOG_EMAIL_STATE_T {
  LOG_EMAIL_STATE_NO_DATA,
  LOG_EMAIL_STATE_COLLECTING,
  LOG_EMAIL_STATE_SENT,
  LOG_EMAIL_STATE_COOLDOWN,
};
enum LOG_EMAIL_STATE_T email_state;


static void poke_state_machine() {
  u64 now_epoch_sec = now_sec();
  assert(now_epoch_sec > EMAIL_RAPID_THRESHOLD_SECS);
  assert(now_epoch_sec > EMAIL_LOW_THRESHOLD_SECS);
  assert(now_epoch_sec > EMAIL_TIMEOUT_SECS);

  start: switch (email_state) {  SWITCH_DEFAULT_IS_UNEXPECTED;
    case LOG_EMAIL_STATE_NO_DATA: {
      if (email_buf_used) {
        email_sent_epoch_sec = now_epoch_sec;
        email_state = LOG_EMAIL_STATE_COLLECTING;
        goto start;
      } else {
        DEBUG("No data, do nothing");
      }
    } break;
    case LOG_EMAIL_STATE_COLLECTING: {
      if (email_buf_used >= EMAIL_LOW_THRESHOLD_BYTES ||
        now_epoch_sec >= email_sent_epoch_sec + EMAIL_LOW_THRESHOLD_SECS) {
          DEBUG("one of the low thresholds are met, lets queue up an email");
          email_init(&email_ctx, email_rcpt, email_buf, email_buf_used, "Logs");
          email_sent_bytes     = email_buf_used;
          email_sent_epoch_sec = now_epoch_sec;
          IO_TIMER_MS(logging_send) = (email_sent_epoch_sec + EMAIL_TIMEOUT_SECS) * 1000;
          email_state = LOG_EMAIL_STATE_SENT;
      } else {
        DEBUG("Waiting on threshold timeout or more data");
        IO_TIMER_MS(logging_send) = (email_sent_epoch_sec + EMAIL_LOW_THRESHOLD_SECS) * 1000;
      }
    } break;
    case LOG_EMAIL_STATE_SENT: {
      assert( IO_TIMER_MS(logging_send) == (email_sent_epoch_sec + EMAIL_TIMEOUT_SECS) * 1000 );
      if (email_sent_epoch_sec + EMAIL_TIMEOUT_SECS <= now_epoch_sec) {
        DEBUG("If our email is timing out, lets abort it");
        fprintf(stderr, "Error: log email took too long, aborting\n");
        email_free(&email_ctx);
        email_sent_bytes = 0;
        IO_TIMER_MS(logging_send) = -1;
        email_state = LOG_EMAIL_STATE_COOLDOWN;
        goto start;
      } else {
        DEBUG("Email is in flight, lets not do anything for now");
      }
    } break;
    case LOG_EMAIL_STATE_COOLDOWN: {
      if ( now_epoch_sec < email_sent_epoch_sec + EMAIL_RAPID_THRESHOLD_SECS) {
        DEBUG("Were cooling down");
        IO_TIMER_MS(logging_send) = ( email_sent_epoch_sec + EMAIL_RAPID_THRESHOLD_SECS ) * 1000;
      } else {
        if (email_buf_used) {
          email_state = LOG_EMAIL_STATE_COLLECTING;
        } else {
          email_state = LOG_EMAIL_STATE_NO_DATA;
        }
        goto start;
      }
    } break;
  }
}

void email_done(u8 success) {
  if (success) {
    memmove(email_buf, email_buf + email_sent_bytes, email_sent_bytes);
    email_buf_used -= email_sent_bytes;
  }
  email_sent_bytes = 0;
  email_free(&email_ctx);
  email_state = LOG_EMAIL_STATE_COOLDOWN;
  poke_state_machine();
}

void logging_send_timeout() { poke_state_machine(); }

static void buf_add_step1(char**buf_, usz*buf_space_left) {
  *buf_ = email_buf + email_buf_used;
  *buf_space_left = email_buf_SIZE - email_buf_used;
}
static void buf_add_step2(usz new_space_used) {
  assert(new_space_used <= email_buf_SIZE - email_buf_used);
  email_buf_used += new_space_used;
}


