
// Lite version of logging.h
void _log_lite(const char* severity, const char*file, const char*func, int line, char* fmt, ...)
    __attribute__((__format__ (__printf__, 5, 6)));

#define LOG(severity, ...) _log_lite(severity, __FILE__, __FUNCTION__, __LINE__, "" __VA_ARGS__)

#define  INFO(          ...) LOG(" INFO",  __VA_ARGS__)
#define  WARN(          ...) LOG(" WARN",  __VA_ARGS__)
#define ERROR(          ...) LOG("ERROR",  __VA_ARGS__)
#define FATAL(          ...) LOG("FATAL",  __VA_ARGS__)

#if defined(LOG_DEBUG) &&  BUILD_IS_RELEASE == 0
#define DEBUG(          ...)  LOG("DEBUG",  __VA_ARGS__)
#else
#define DEBUG(          ...)
#endif

#define assert(expr)      ({if (!(expr))     { ERROR("Assert failed: (%s)", #expr); } })
