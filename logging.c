
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

#include "email.h"
// TODO: setup gmail to delete old emails
static  s32 const buf_SIZE = 1<20;
static  s32 const EMAIL_COOLDOWN_SECONDS = 60 * 10; // 10 minutes
static char buf[buf_SIZE];
static  s32 buf_used;
static   u8 recursing_error;
static  u64 last_sent_epoch_sec;
static   u8 internal_error;
static  s32 sent_size;
static char const * const logging_email_rcpt = "logging@test.test";
struct email_Send email_ctx;

static void poke_state_machine() {
  u64 now_epoch_sec = time(0);
  if (!buf_used) {
    // do nothing, no logs
    IO_TIMER(logging_send) = -1;
  } else if (sent_size) {
    // Do nothing... waiting for email to send or timeout
    IO_TIMER(logging_send) = last_sent_epoch_sec + EMAIL_COOLDOWN_SECONDS;
  } else if (last_sent_epoch_sec + EMAIL_COOLDOWN_SECONDS > now_epoch_sec ) {
    // do nothing, cooling down...
  } else {
    email_init(&email_ctx, io_curl_create_handle(), logging_email_rcpt, buf, buf_used, "Logs");
    sent_size = buf_used;
    last_sent_epoch_sec = now_epoch_sec;
  }
}

void email_done(u8 success) {
  if (success) {
    memmove(buf, buf + sent_size, sent_size);
    buf_used -= sent_size;
  }
  sent_size = 0;
  poke_state_machine();
}

void logging_send_timeout() {

}


#define VLOGF(fmt, va)  vbuf_add(fmt, va)
#define  LOGF(fmt, ...)  buf_add(fmt, ##__VA_ARGS__)

static void  vbuf_add(char const * fmt, va_list va) {
  recursing_error = 1;
  s32 r = vsnprintf(buf + buf_used, buf_SIZE - buf_used, fmt, va);
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
  buf_used += r;
  if (buf_used > buf_SIZE) {
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
