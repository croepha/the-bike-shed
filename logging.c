#define WEAK_NOW_MS __attribute__((weak_import))

#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <inttypes.h>
#include <sys/time.h>
#include "common.h"
#include "io.h"
#include "io_curl.h"
#include "logging.h"

static __thread char _log_ctx_buffer[1024];
static __thread  s32 _log_ctx_len;

__thread s32 log_allowed_fails;

#define VLOGF(fmt, va)  vfprintf(stderr, fmt, va)
#define  LOGF(fmt, ...)  fprintf(stderr, fmt, ##__VA_ARGS__)

u64 real_now_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, 0);
    return (long long)(tv.tv_sec) * 1000 + (long long)(tv.tv_usec) / 1000;
}

void _log(const char* severity, const char*file, const char*func, int line,
          enum _log_options options, char* buf, usz buf_size, char* fmt, ...) {
  u64 now_ms_;
  if (now_ms) {
    now_ms_ = now_ms();
  } else {
    now_ms_ = real_now_ms();
  }

  void * return_address = __builtin_extract_return_addr(__builtin_return_address (0));
  LOGF("%06"PRIx64".%03"PRIu64": %s:%s ", now_ms_ / 1000, now_ms_ % 1000, severity, _log_ctx_buffer);
  //LOGF("%"PRIu64".%03"PRIu64": %s:%s ", now_ms_ / 1000, now_ms_ % 1000, severity, _log_ctx_buffer);
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
