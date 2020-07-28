
#pragma once


#define STRINGIFY2(a) #a
#define STRINGIFY(a) STRINGIFY(a)
#define COUNT(array) (sizeof(array)/sizeof((array)[0]))
#define MIN(a,b) (a < b ? a : b)


enum {
  EVENT_TYPE_DOWNLOAD
};

#include <stdlib.h>

typedef u_int8_t   u8;
typedef u_int16_t u16;
typedef u_int32_t u32;
typedef u_int64_t u64;
typedef   int8_t   s8;
typedef   int16_t s16;
typedef   int32_t s32;
typedef   int64_t s64;
typedef  ssize_t  ssz;
typedef   size_t  usz;

long long utc_ms_since_epoch();
extern int epoll_fd;
