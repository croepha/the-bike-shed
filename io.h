#pragma once

#include <sys/epoll.h>
#include "common.h"

typedef union {
  epoll_data_t data;
  struct {
    s32 id;
    u8 event_type;
  } my_data;
} io_EPData;

#define _GLOBAL_TIMERS \
 _(logging_send)

#define _(name) _io_timer_ ## name,
enum _io_global_timers { _(INVALID) _GLOBAL_TIMERS _(COUNT) };
#undef _

extern u64 io_global_timers[];
#define IO_TIMER(name) io_global_timers[_io_timer_ ## name]

void io_process_events();


