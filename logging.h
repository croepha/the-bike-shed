#pragma once
#include "common.h"

int  _log_context_push(char* fmt, ...) __attribute__((__format__ (__printf__, 1, 2)));
void _log_context_pop(int*original_len);

void _log(const char* severity, const char*file, const char*func,
          int line, char* fmt, ...)
          __attribute__((__format__ (__printf__, 5, 6)));
void _log_buffer(const char* severity, const char*file, const char*func, int line, char* buf, usz buf_size,  char* fmt, ...)
          __attribute__((__format__ (__printf__, 7, 8)));

extern __thread int log_allowed_fails;

#define TOKENPASTE2(a, b) a ## b
#define TOKENPASTE(a, b) TOKENPASTE2(a,b)
#define LOGCTX(fmt, ...) \
  __attribute__((unused)) \
  __attribute__((cleanup(_log_context_pop))) \
  int TOKENPASTE(original_length, __COUNTER__) = \
    _log_context_push(fmt,  ##__VA_ARGS__);

#define LOG(severity, fmt, ...) \
  _log(severity, __FILE__, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_BUFFER(severity, fmt, buf, buf_size, ...) \
  _log_buffer(severity, __FILE__, __FUNCTION__, __LINE__, buf, buf_size, fmt, ##__VA_ARGS__)
#define  INFO(fmt, ...)  LOG(" INFO", fmt, ##__VA_ARGS__)
#define  INFO_BUFFER(fmt, buf, buf_size, ...)  LOG_BUFFER(" INFO", fmt, buf, buf_size, ##__VA_ARGS__)
#define DEBUG(...) LOG("DEBUG", "" __VA_ARGS__)
#if ABORT_ON_ERROR
#include <stdlib.h>
#define  WARN(...) { LOG(" WARN", "" __VA_ARGS__); if(log_allowed_fails-- <0) { abort(); } }
#define ERROR(...) { LOG("ERROR", "" __VA_ARGS__); if(log_allowed_fails-- <0) { abort(); } }
#define FATAL(...) { LOG("FATAL", "" __VA_ARGS__); abort(); }
#else
#define  WARN(...) LOG(" WARN", "" __VA_ARGS__)
#define ERROR(...) LOG("ERROR", "" __VA_ARGS__)
#define FATAL(...) LOG("FATAL", "" __VA_ARGS__)
#endif


#ifndef LOG_DEBUG
#undef DEBUG
#define DEBUG(...)
#endif

extern const char * const sys_errlist[];
extern int sys_nerr;
#define assert(expr)      ({if (!(expr))    { ERROR("Assert failed: (%s)", #expr); } })
#define error_check(err)  ({if ((err) == -1) { ERROR("Posix like error:(%s)==-1 errno:%d:%s", #err, errno, errno < sys_nerr ? sys_errlist[errno] : "Unkown Error"); } })
