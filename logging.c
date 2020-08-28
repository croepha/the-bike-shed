
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
static  u32 const EMAIL_LOW_THRESHOLD_BYTES = 1<<10   ; // 1 KB
static  u32 const EMAIL_LOW_THRESHOLD_SECS  = 60 * 1  ; // 1 Minute
static  u32 const EMAIL_RAPID_THRESHOLD_SECS = 20    ; // 20 Seconds  Prevent emails from being sent more often than this
static  u32 const EMAIL_TIMEOUT_SECS        = 60 * 2  ; // 10 Minutes
static  u32 const email_buf_SIZE  = 1<<22   ; // 4 MB
static char email_buf[email_buf_SIZE];
static  u32 email_buf_used;
static  u64 email_sent_epoch_sec;
static  u32 email_sent_bytes;
static   u8 recursing_error;
static   u8 internal_error;
struct email_Send email_ctx;

static void poke_state_machine() {
  u64 now_epoch_sec = now_sec();

  if (email_sent_bytes) { // If our email is timing out, lets abort it
    assert( IO_TIMER_MS(logging_send) == (email_sent_epoch_sec + EMAIL_TIMEOUT_SECS) * 1000 );
    if (email_sent_epoch_sec + EMAIL_TIMEOUT_SECS <= now_epoch_sec) {
      fprintf(stderr, "Error: log email took too long, aborting\n");
      email_free(&email_ctx);
      email_sent_epoch_sec = now_epoch_sec + EMAIL_RAPID_THRESHOLD_SECS;
      email_sent_bytes = 0;
    }
  }

  if (email_sent_bytes) {
    // We already have an email in flight, lets not try to queue up annother
  } else if (!email_buf_used) {
    // If we have nothing in the buffer to send, lets turn off the low threshold timer
    email_sent_epoch_sec = 0;
    IO_TIMER_MS(logging_send) = -1;
  } else {
    if (email_sent_epoch_sec == 0) {
      // our buffer just started filling up, lets mark the time, we will probably set the low threshold timer
      email_sent_epoch_sec = now_epoch_sec;
    }
    if (email_buf_used >= EMAIL_LOW_THRESHOLD_BYTES ||
      email_sent_epoch_sec + EMAIL_LOW_THRESHOLD_SECS <= now_epoch_sec) {
        // one of the low thresholds are met
        if (email_sent_epoch_sec  > now_epoch_sec) {
          fprintf(stderr, "Warning: log email cooling down\n");
          IO_TIMER_MS(logging_send) = now_epoch_sec + EMAIL_RAPID_THRESHOLD_SECS;
        } else {
          // lets queue up an email
          email_init(&email_ctx, email_rcpt, email_buf, email_buf_used, "Logs");
          email_sent_bytes     = email_buf_used;
          email_sent_epoch_sec = now_epoch_sec;
          IO_TIMER_MS(logging_send) = (email_sent_epoch_sec + EMAIL_TIMEOUT_SECS) * 1000;
        }
    } else {
      IO_TIMER_MS(logging_send) = (email_sent_epoch_sec + EMAIL_LOW_THRESHOLD_SECS) * 1000;
    }
  }
}

void email_done(u8 success) {
  if (success) {
    memmove(email_buf, email_buf + email_sent_bytes, email_sent_bytes);
    email_buf_used -= email_sent_bytes;
  }
  email_sent_bytes = 0;
  email_free(&email_ctx);
  poke_state_machine();
}

void logging_send_timeout() { poke_state_machine(); }


#define VLOGF(fmt, va)  vbuf_add(fmt, va)
#define  LOGF(fmt, ...)  buf_add(fmt, ##__VA_ARGS__)

static void  vbuf_add(char const * fmt, va_list va) {
  recursing_error = 1;
  s32 r = vsnprintf(email_buf + email_buf_used, email_buf_SIZE - email_buf_used, fmt, va);
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
  email_buf_used += r;
  if (email_buf_used > email_buf_SIZE) {
    internal_error = 1;
    ERROR("Formatted log string is too long %d, dropping...", r);
    recursing_error = 0;
    return;
  }
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
  LOGF("\t(%s:%03d %p:%s)\n", file, line, return_address, func);
  poke_state_machine();
}



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
