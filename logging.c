
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "common.h"
#include "io.h"
#include "io_curl.h"
#include "logging.h"

static __thread char _log_ctx_buffer[1024];
static __thread  s32 _log_ctx_len;

__thread s32 log_allowed_fails;

#if LOGGING_USE_EMAIL
// TODO, for robustness, we should install some exception handlers for segfaults and aborts, attempt to send final log buffers, and if that fails
//       dump to disk

#ifndef now_sec
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

#undef  DEBUG
#undef  ERROR
//#define DEBUG(...) fprintf(stderr, "TRACE: " __VA_ARGS__); fprintf(stderr, "\n")
#define DEBUG(...)
#define ERROR(...) fprintf(stderr, "ERROR: " __VA_ARGS__); fprintf(stderr, "\n"); abort();

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


#define VLOGF(fmt, va)  vbuf_add(fmt, va)
#define  LOGF(fmt, ...)  buf_add(fmt, ##__VA_ARGS__)

static void buf_add_step1(char**buf_, usz*buf_space_left) {
  *buf_ = email_buf + email_buf_used;
  *buf_space_left = email_buf_SIZE - email_buf_used;
}
static void buf_add_step2(usz new_space_used) {
  assert(new_space_used <= email_buf_SIZE - email_buf_used);
  if(memchr(email_buf + email_buf_used, '\n', new_space_used)) {
    email_buf_used += new_space_used;
    poke_state_machine();
  } else {
    email_buf_used += new_space_used;
  }
}

static void  vbuf_add(char const * fmt, va_list va) {
  recursing_error = 1;
  char * buf; usz    space_left;
  buf_add_step1(&buf, &space_left);

  s32 r = vsnprintf(buf, space_left, fmt, va);
  if (r < 0) { // Pretty rare error, but want to handle this case since we need to be super robust
    if (recursing_error) { // Hitting this case would be even rarer
      internal_error = 1;
      fprintf(stderr, "Recursive error, not recursing further... fmt:`%s' \n", fmt);
      vfprintf(stderr, fmt, va);
      fprintf(stderr, "\nEnd\n");
    } else {
      ERROR("Failed to format log string: `%s'", fmt);
    }
    recursing_error = 0;
    return;
  }
  if (r >= space_left) {
    internal_error = 1;
    ERROR("Formatted log string is too long %d, dropping...", r);
    recursing_error = 0;
    return;
  }
  buf_add_step2(r);
  recursing_error = 0;
}

__attribute__((__format__ (__printf__, 1, 2)))
static void  buf_add(char const * fmt, ...) {
  va_list va;
  va_start(va, fmt);
  vbuf_add(fmt, va);
  va_end(va);
}

#else // LOGGING_USE_EMAIL
#define VLOGF(fmt, va)  vfprintf(stderr, fmt, va)
#define  LOGF(fmt, ...)  fprintf(stderr, fmt, ##__VA_ARGS__)
void poke_state_machine() {};
#endif // LOGGING_USE_EMAIL


void _log(const char* severity, const char*file, const char*func, int line,
          enum _log_options options, char* buf, usz buf_size, char* fmt, ...) {
  struct timespec tp;
  int r = clock_gettime(CLOCK_MONOTONIC_COARSE, &tp);
  if (r != 0) {
    tp.tv_sec  = 0;
    tp.tv_nsec = 0;
    assert(0);
  }

  void * return_address = __builtin_extract_return_addr(__builtin_return_address (0));
  LOGF("%06lx.%03ld: %s:%s ", tp.tv_sec, tp.tv_nsec / 1000000, severity, _log_ctx_buffer);
  va_list va; va_start(va, fmt); VLOGF(fmt, va); va_end(va);

  switch (options) { SWITCH_DEFAULT_IS_UNEXPECTED;
    case _log_options_plain: break;
    case _log_options_buffer_string: {
      LOGF(" `");
      for (usz i = 0; i < buf_size; i++) {
        if (isprint(buf[i])) {
          LOGF("%c", buf[i]);
        } else {
          LOGF("\\x%02x", buf[i]);
        }
      }
      LOGF("`");
    } break;
    case _log_options_buffer_hex: {
      LOGF(" [");
      for (usz i = 0; i < buf_size; i++) {
        LOGF("%02hhx", buf[i]);
      }
      LOGF("]");
    } break;
  }
  LOGF("\t(%s:%03d %p:%s)\n", file, line, return_address, func);}



int _log_context_push(char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  int r = vsnprintf(_log_ctx_buffer + _log_ctx_len,
                    sizeof _log_ctx_buffer - _log_ctx_len, fmt, args);
  va_end(args);
  assert(r>=0);

  //printf("DEBUG: %d %d\n", _log_ctx_len, r);

  int old_len = _log_ctx_len;
  _log_ctx_len += r;
  if (_log_ctx_len > sizeof _log_ctx_buffer - 1) {
    _log_ctx_len = sizeof _log_ctx_buffer - 1;
    //printf("buffer out of space\n");
  }
  return old_len;
}

void _log_context_pop(int *original_len) {
  _log_ctx_len = *original_len;
  _log_ctx_buffer[_log_ctx_len] = 0;
}
