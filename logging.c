
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
  u8 recursing_error;
  u8 internal_error;


#define VLOGF(fmt, va)  vbuf_add(fmt, va)
#define  LOGF(fmt, ...)  buf_add(fmt, ##__VA_ARGS__)

void buf_add_step1(char**buf_, usz*buf_space_left);
void buf_add_step2(usz new_space_used);

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
