#pragma once
#include <stdarg.h>
#include <errno.h>
#include "common.h"

enum _log_options {
  _log_options_plain,
  _log_options_buffer_string,
  _log_options_buffer_hex,
};

int  _log_context_push(char* fmt, ...) __attribute__((__format__ (__printf__, 1, 2)));
void _log_context_pop(int*original_len);

void _log(const char* severity, const char*file, const char*func, int line,
                 enum _log_options options, char* buf, usz buf_size, char* fmt, ...)
          __attribute__((__format__ (__printf__, 8, 9)));

extern __thread int log_allowed_fails;

#ifndef LOGF_CARBON_COPY_IMPL
#define LOGF_CARBON_COPY_IMPL __attribute__((weak_import))
#endif
void logf_carbon_copy_v(char const * fmt, va_list va) LOGF_CARBON_COPY_IMPL ;
void logf_carbon_copy_end(void) LOGF_CARBON_COPY_IMPL ;


#define TOKENPASTE2(a, b) a ## b
#define TOKENPASTE(a, b) TOKENPASTE2(a,b)
#define LOGCTX(fmt, ...) \
  __attribute__((unused)) \
  __attribute__((cleanup(_log_context_pop))) \
  int TOKENPASTE(original_length, __COUNTER__) = \
    _log_context_push(fmt,  ##__VA_ARGS__);


#define LOG(severity, options, buf, buf_size, ...) \
  _log(severity, __FILE__, __FUNCTION__, __LINE__, _log_options_ ## options, (buf), buf_size, "" __VA_ARGS__)
#if ABORT_ON_ERROR
#define FLOG(...) ({ LOG(__VA_ARGS__); if(--log_allowed_fails <0) { LOG("ERROR", plain,0,0, "Aborting due to error"); abort(); }})
#else
#define FLOG(...) LOG(__VA_ARGS__)
#endif

/*

INFO    General information that we want to convey examples: Health info, tracking siginificant events

ERROR   Something is wrong, a condition that we shouldn't see during normal operation has occurred,
        but the programmer has accounted for this and the system should be able to carry on.
        Examples: We cant connect to a server.  We got a corrupted configuration value.  We ran out of some
          resource
        On Debug builds, ERRORS will cause aborts, unless log_allowed_fails is greater than zero, every error
        will decrement the number of allowed fails

WARN    Just like ERROR, but slightly less severe.  Also things that might be ERRORs but we aren't certain
        Example: We got a config value we didn't recognize

FATAL   Something really unexpected has happened, execution cannot continue... Process will abort and restart

DEBUG   Hints to the developer these are turned of in production builds, ignored for tests

TRACE   An internal dump of an algorithms state or event tracing, useful for tests, these are turned of in production builds


*/

#define  INFO(          ...)  LOG(" INFO", plain, 0, 0,      __VA_ARGS__)
#define  INFO_BUFFER(   ...)  LOG(" INFO", buffer_string,    __VA_ARGS__)
#define  INFO_HEXBUFFER(...)  LOG(" INFO", buffer_hex,(char*)__VA_ARGS__)
#define  WARN(          ...) FLOG(" WARN", plain, 0, 0,      __VA_ARGS__)
#define ERROR(          ...) FLOG("ERROR", plain, 0, 0,      __VA_ARGS__)
#define ERROR_BUFFER(   ...)  LOG("ERROR", buffer_string,    __VA_ARGS__)
#define ERROR_HEXBUFFER(...)  LOG("ERROR", buffer_hex,(char*)__VA_ARGS__)
#define FATAL(          ...) FLOG("FATAL", plain, 0, 0,      __VA_ARGS__)

#if BUILD_IS_RELEASE == 1
#define TRACE(          ...)
#define TRACE_BUFFER(   ...)
#define TRACE_HEXBUFFER(...)
#else
extern u8 log_trace_enabled;
#define TRACE(          ...) ({ if (log_trace_enabled) { LOG("TRACE", plain, 0, 0,      __VA_ARGS__); } })
#define TRACE_BUFFER(   ...) ({ if (log_trace_enabled) { LOG("TRACE", buffer_string,    __VA_ARGS__); } })
#define TRACE_HEXBUFFER(...) ({ if (log_trace_enabled) { LOG("TRACE", buffer_hex,(char*)__VA_ARGS__); } })
#endif

#if defined(LOG_DEBUG) &&  BUILD_IS_RELEASE == 0
#define DEBUG(          ...)  LOG("DEBUG", plain, 0, 0,      __VA_ARGS__)
#define DEBUG_BUFFER(   ...)  LOG("DEBUG", buffer_string,    __VA_ARGS__)
#define DEBUG_HEXBUFFER(...)  LOG("DEBUG", buffer_hex,(char*)__VA_ARGS__)
#else
#define DEBUG(          ...)
#define DEBUG_BUFFER(   ...)
#define DEBUG_HEXBUFFER(...)
#endif

#ifdef LOG_NO_TRACE
#undef TRACE
#define TRACE(...)
#endif

#define assert(expr)      ({if (!(expr))     { ERROR("Assert failed: (%s)", #expr); } })
// extern const char * const sys_errlist[];
// extern int sys_nerr;
// strerror(errno) -> errno < sys_nerr ? sys_errlist[errno] : "Unkown Error"
extern char * strerror (int __errnum) __THROW;
#define error_check(err)  ({if ((err) == -1) { ERROR("Posix like error:(%s)==-1 errno:%d:%s", #err, errno, strerror(errno)); } })
