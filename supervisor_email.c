#define LOG_DEBUG

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "io_curl.h"
#include "logging.h"
#include "common.h"
#include "config.h"
#include "io.h"

u64 now_sec();

#include "email.h"
#include "supervisor.h"
// TODO: setup gmail to delete old emails
// LOW_THRESHOLDS: We will start considering sending logs once we have accumulated this much bytes/time
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

struct SupervisorEmailCtx {
  enum _io_curl_type curl_type;
};
struct SupervisorEmailCtx supr_email_email_ctx;
IO_CURL_SETUP(supervisor_email, struct SupervisorEmailCtx, curl_type);


static void poke_state_machine() {
  u64 now_epoch_sec = now_sec();
  assert(now_epoch_sec > SUPR_EMAIL_RAPID_THRESHOLD_SECS);
  assert(now_epoch_sec > SUPR_EMAIL_LOW_THRESHOLD_SECS);
  assert(now_epoch_sec > SUPR_EMAIL_TIMEOUT_SECS);

  start:
    DEBUG("state:%d now_sec:%"PRIu64" sent:%"PRIu64, supr_email_state, now_epoch_sec, supr_email_sent_epoch_sec);
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
        usz email_size = supr_email_buf_used;
        email_init(&supr_email_ctx,
                   supervisor_email_io_curl_create_handle(&supr_email_email_ctx),
                   email_rcpt, supr_email_buf,
                   email_size, "Logs");
        supr_email_sent_bytes = email_size;
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
        ERROR("log email took too long, aborting");
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

void supervisor_email_io_curl_complete(CURL *easy, CURLcode result, struct SupervisorEmailCtx * ctx) {
  DEBUG("easy:%p result:%d", easy, result);
  if (result == CURLE_OK) {
    memmove(supr_email_buf, supr_email_buf + supr_email_sent_bytes,
            supr_email_sent_bytes);
    supr_email_buf_used -= supr_email_sent_bytes;
  } else {
    INFO("failed %s", curl_easy_strerror(result));
  }
  supr_email_sent_bytes = 0;
  email_free(&supr_email_ctx);
  supr_email_state = SUPR_LOG_EMAIL_STATE_COOLDOWN;
  poke_state_machine();

}

void logging_send_timeout() {
  DEBUG();
  poke_state_machine();
}

void supr_email_add_data_start(char**buf_, usz*buf_space_left) {
  *buf_ = supr_email_buf + supr_email_buf_used;
  *buf_space_left = supr_email_buf_SIZE - supr_email_buf_used;
  DEBUG("%zu", *buf_space_left);
}
void supr_email_add_data_finish(usz new_space_used) {
  assert(new_space_used <= supr_email_buf_SIZE - supr_email_buf_used);
  // TODO save everything after newline for next send,  make sure we still send if its too much after ? maybe not
  DEBUG_BUFFER(supr_email_buf + supr_email_buf_used, (int)new_space_used, "%zu", new_space_used);
  if (memchr(supr_email_buf + supr_email_buf_used, '\n', new_space_used)) {
    supr_email_buf_used += new_space_used;
    poke_state_machine();
  } else {
    supr_email_buf_used += new_space_used;
  }
}
