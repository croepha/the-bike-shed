
#pragma once

#ifndef WEAK_NOW_MS
#define WEAK_NOW_MS
#endif

#define STRINGIFY2(a) #a
#define STRINGIFY(a) STRINGIFY(a)
#define COUNT(array) (sizeof(array)/sizeof((array)[0]))
#define MIN(a,b) (a < b ? a : b)

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

u64 now_ms(void) WEAK_NOW_MS;
u64 real_now_ms(void);
void supr_email_add_data_start(char**buf_, usz*buf_space_left);
void supr_email_add_data_finish(usz new_space_used);

__attribute__((unused))
static u64 now_sec() { return now_ms() / 1000; } ;


#if BUILD_IS_RELEASE
#define SWITCH_DEFAULT_IS_UNEXPECTED default: { ERROR("Got unexpected switch case "); } break;
#else
#define SWITCH_DEFAULT_IS_UNEXPECTED { ERROR("Got unexpected switch case"); } break;
#endif


void shed_add_philantropist_hex(char* hex);
