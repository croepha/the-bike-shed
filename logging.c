
#include <time.h>
#include <stdio.h>
#include <assert.h>
#include "common.h"
#include "logging.h"

static __thread char _log_ctx_buffer[1024];
static __thread  s32 _log_ctx_len;

__thread s32 log_allowed_fails;

#if LOGGING_USE_EMAIL
// TODO: setup gmail to delete old emails
static  s32 const buf_SIZE = 1<20;
static char buf[buf_SIZE];
static  s32 buf_used;
static   u8 recursing_error;
//static  u64 last_email_send_utc_sec;
static   u8 internal_error;

static void poke_state_machine() {
  if (!buf_used) {
    // do nothing, no logs
  } else if (email_sent_size) {
    // Do nothing... waiting for email to send or timeout
  } else if (last_email_send_epoch_sec + EMAIL_COOLDOWN_SECONDS > now_epoch_seconds() ) {
    // do nothing, cooling down...
  } else {
    email_send(buf, buf_used);
    email_sent_size = buf_used;
    last_email_send_epoch_sec = now_epoch_seconds();
  }
  io_loggging_timer = last_email_send_epoch_sec + EMAIL_COOLDOWN_SECONDS;
}

static void email_done(u8 success) {
  if (success) {
    memmove(buf, buf+email_sent_size, email_sent_size);
    buf_used -= email_sent_size;
  }
  email_sent_size = 0;
  poke_state_machine();
}


#define VLOGF(fmt, ...) vbuf_add(fmt, #__VA_ARGS__)
#define  LOGF(fmt, va)   buf_add(fmt, va)

static void vbuf_add(char const * fmt, va_list va) {
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
#define  LOGF(fmt, ...)  dprintf(2, fmt, #__VA_ARGS__)
#define VLOGF(fmt, va)  vdprintf(2, fmt, va)
#endif // LOGGING_USE_EMAIL

void _log(const char* severity, const char*file, const char*func, int line, char* fmt, ...) {
  LOGCTX("logging");
  fprintf(stderr, "%s:%s ", severity, _log_ctx_buffer);
  va_list va; va_start(va, fmt);
  LOGF(fmt, va);
  va_end(va);
  fprintf(stderr, "\t(%s:%s:%d)\n", file, func, line);
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
