
#include <stdio.h>
#include <assert.h>
#include "common.h"

static __thread char _log_ctx_buffer[1024];
static __thread  s32 _log_ctx_len;

__thread s32 log_allowed_fails;


#if LOGGING_USE_EMAIL
// TODO: setup gmail to delete old emails
// static const s32 buf_SIZE = 1<20;
// static        char buf[buf_SIZE];
// static       s32 buf_used;
// static       u8  recursing_error;

// void buf_add(char const * fmt, va_list va) {
//   s32 starting_used = buf_used;
//   s32 r = vsnprintf(buf + buf_used, buf_SIZE - buf_used, fmt, va);
//   if (r < 0) {
//     if (recursing)
//     return;
//   }
//   buf_used +=





// }

#define LOGF(fmt, va) dprintf(2, fmt, va)
//#define LOGF(fmt, va) buf_add(fmt, va)

#else // LOGGING_USE_EMAIL
#define LOGF(fmt, va) dprintf(2, fmt, va)
#endif // LOGGING_USE_EMAIL

void _log(const char* severity, const char*file, const char*func, int line, char* fmt, ...) {

  fprintf(stderr, "%s:%s ", severity, _log_ctx_buffer);
  va_list va; va_start(va, fmt);
  vfprintf(stderr, fmt, va);
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
