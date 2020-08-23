#pragma once
#include "common.h"

enum _log_options {
  _log_options_plain,
  _log_options_buffer_string,
  _log_options_buffer_hex,
};

int  _log_context_push(char* fmt, ...) __attribute__((__format__ (__printf__, 1, 2)));
void _log_context_pop(int*original_len);

void _log(const char* severity, const char*file, const char*func, int line,
                 enum _log_options options, u8* buf, usz buf_size, char* fmt, ...)
          __attribute__((__format__ (__printf__, 8, 9)));

extern __thread int log_allowed_fails;


#define TOKENPASTE2(a, b) a ## b
#define TOKENPASTE(a, b) TOKENPASTE2(a,b)
#define LOGCTX(fmt, ...) \
  __attribute__((unused)) \
  __attribute__((cleanup(_log_context_pop))) \
  int TOKENPASTE(original_length, __COUNTER__) = \
    _log_context_push(fmt,  ##__VA_ARGS__);


#define LOG(severity, options, buf, buf_size, ...) \
  _log(severity, __FILE__, __FUNCTION__, __LINE__, _log_options_ ## options, (u8*)(buf), buf_size, "" __VA_ARGS__)
#if ABORT_ON_ERROR
#define FLOG(...) ({ LOG(__VA_ARGS__); if(log_allowed_fails-- <0) { abort(); }})
#else
#define FLOG(...) LOG(__VA_ARGS__)
#endif


#define  INFO(          ...)  LOG(" INFO", plain, 0, 0,   __VA_ARGS__)
#define  INFO_BUFFER(   ...)  LOG(" INFO", buffer_string, __VA_ARGS__)
#define  INFO_HEXBUFFER(...)  LOG(" INFO", buffer_hex   , __VA_ARGS__)
#define DEBUG(          ...)  LOG("DEBUG", plain, 0, 0,   __VA_ARGS__)
#define DEBUG_BUFFER(   ...)  LOG("DEBUG", buffer_string, __VA_ARGS__)
#define  WARN(          ...) FLOG(" WARN", plain, 0, 0,   __VA_ARGS__)
#define ERROR(          ...) FLOG("ERROR", plain, 0, 0,   __VA_ARGS__)
#define FATAL(          ...) FLOG("FATAL", plain, 0, 0,   __VA_ARGS__)


#ifndef LOG_DEBUG
#undef DEBUG
#define DEBUG(...)
#endif

extern const char * const sys_errlist[];
extern int sys_nerr;
#define assert(expr)      ({if (!(expr))    { ERROR("Assert failed: (%s)", #expr); } })
#define error_check(err)  ({if ((err) == -1) { ERROR("Posix like error:(%s)==-1 errno:%d:%s", #err, errno, errno < sys_nerr ? sys_errlist[errno] : "Unkown Error"); } })
