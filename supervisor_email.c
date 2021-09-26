//#define LOG_DEBUG

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "io_curl.h"
#include "logging.h"
#include "common.h"
#include "config.h"
#include "io.h"

#include "email.h"
#include "supervisor.h"

// TODO: setup gmail to delete old emails

u32 const supr_email_buf_SIZE = 1 << 22; // 4 MB  // dont increase over 24 MB, gmail has a hard limit at 25MB
char supr_email_buf[supr_email_buf_SIZE];
u32 supr_email_buf_used;
u64 supr_email_sent_epoch_sec;
u32 supr_email_sent_bytes;
u32 supr_email_push_requested;
struct email_Send supr_email_ctx;

enum SUPR_LOG_EMAIL_STATE_T {
  SUPR_LOG_EMAIL_STATE_NO_DATA,
  SUPR_LOG_EMAIL_STATE_COLLECTING,
  SUPR_LOG_EMAIL_STATE_SENT,
  SUPR_LOG_EMAIL_STATE_COOLDOWN,
};
enum SUPR_LOG_EMAIL_STATE_T supr_email_state;

struct SupervisorCurlCtx {
  enum _io_curl_type curl_type;
};
struct SupervisorCurlCtx supr_email_email_ctx;
IO_CURL_SETUP(supervisor_email, struct SupervisorCurlCtx, curl_type);


static void poke_state_machine() {
  u64 now_epoch_sec = IO_NOW_MS() / 1000;
  assert(now_epoch_sec > supr_email_rapid_threshold_secs);
  assert(now_epoch_sec > supr_email_low_threshold_secs);
  assert(now_epoch_sec > supr_email_timeout_secs);

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
      if (supr_email_push_requested ||
          supr_email_buf_used >= supr_email_low_threshold_bytes ||
          now_epoch_sec >=
              supr_email_sent_epoch_sec + supr_email_low_threshold_secs) {
        DEBUG("one of the low thresholds are met, lets queue up an email");
        usz email_size = supr_email_buf_used;
        email_init(&supr_email_ctx,
                   supervisor_email_io_curl_create_handle(&supr_email_email_ctx),
                   email_rcpt, supr_email_buf,
                   email_size, "Logs");
        supr_email_sent_bytes = email_size;
        supr_email_sent_epoch_sec = now_epoch_sec;
        IO_TIMER_MS_SET(logging_send, (supr_email_sent_epoch_sec + supr_email_timeout_secs) * 1000);
        supr_email_state = SUPR_LOG_EMAIL_STATE_SENT;
      } else {
        DEBUG("Waiting on threshold timeout or more data");
        IO_TIMER_MS_SET(logging_send, (supr_email_sent_epoch_sec + supr_email_low_threshold_secs) * 1000);
      }
    } break;
    case SUPR_LOG_EMAIL_STATE_SENT: {
      assert(IO_TIMER_MS_DEBUG_GET(logging_send) == (supr_email_sent_epoch_sec + supr_email_timeout_secs) * 1000);
      if (supr_email_sent_epoch_sec + supr_email_timeout_secs <=
          now_epoch_sec) {
        // TODO: I think we should have error logs here...
        DEBUG("If our email is timing out, lets abort it");
        ERROR("log email took too long, aborting");
        email_free(&supr_email_ctx);
        supr_email_sent_bytes = 0;
        IO_TIMER_MS_SET(logging_send, -1);
        supr_email_state = SUPR_LOG_EMAIL_STATE_COOLDOWN;
        goto start;
      } else {
        DEBUG("Email is in flight, lets not do anything for now");
      }
    } break;
    case SUPR_LOG_EMAIL_STATE_COOLDOWN: {
      if (now_epoch_sec <
          supr_email_sent_epoch_sec + supr_email_rapid_threshold_secs) {
        DEBUG("Were cooling down");
        IO_TIMER_MS_SET(logging_send, (supr_email_sent_epoch_sec + supr_email_rapid_threshold_secs) * 1000);
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

void supervisor_email_io_curl_complete(CURL *easy, CURLcode result, struct SupervisorCurlCtx * ctx) {
  DEBUG("easy:%p result:%d", easy, result);
  if (result == CURLE_OK) {
    memmove(supr_email_buf, supr_email_buf + supr_email_sent_bytes,
            supr_email_sent_bytes);
    supr_email_buf_used -= supr_email_sent_bytes;
  } else {
    INFO("failed %s", curl_easy_strerror(result));
  }
  supr_email_push_requested = 0;
  supr_email_sent_bytes = 0;
  email_free(&supr_email_ctx);
  supr_email_state = SUPR_LOG_EMAIL_STATE_COOLDOWN;
  poke_state_machine();
  supr_email_done_hook();
}

void supr_email_push() {
  supr_email_push_requested = 1;
  poke_state_machine();
}

IO_TIMEOUT_CALLBACK(logging_send) {
  DEBUG();
  poke_state_machine();
}

void supr_email_add_data_start(char**buf_, usz*buf_space_left) {
  *buf_ = supr_email_buf + supr_email_buf_used;
  *buf_space_left = supr_email_buf_SIZE - supr_email_buf_used;
  //DEBUG("%zu", *buf_space_left);
}
void supr_email_add_data_finish(usz new_space_used) {
  assert(new_space_used <= supr_email_buf_SIZE - supr_email_buf_used);
  // TODO save everything after newline for next send,  make sure we still send if its too much after ? maybe not
  //DEBUG_BUFFER(supr_email_buf + supr_email_buf_used, (int)new_space_used, "%zu", new_space_used);
  if (memchr(supr_email_buf + supr_email_buf_used, '\n', new_space_used)) {
    supr_email_buf_used += new_space_used;
    poke_state_machine();
  } else {
    supr_email_buf_used += new_space_used;
  }
}
