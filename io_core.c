#include <sys/epoll.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include "common.h"
#include "logging.h"
#include "io.h"

#define _(name) [ _io_timer_ ## name ] = -1,
u64 io_timers_epoch_ms[] = { _(INVALID) _IO_TIMERS};
#undef  _

int io_epoll_fd = -1;

void io_initialize() {
  io_epoll_fd = epoll_create1(EPOLL_CLOEXEC);
}


void io_process_events() { start:;
  enum _io_timers running_timer;
  u64 next_timer_epoch_ms = -1;
  for (int i=1; i < _io_timer_COUNT; i++) {
    DEBUG("timer:%d next:%ld timers[i]:%ld", i, next_timer_epoch_ms, io_timers_epoch_ms[i]);
    if (next_timer_epoch_ms > io_timers_epoch_ms[i]) {
      next_timer_epoch_ms = io_timers_epoch_ms[i];
      running_timer = i;
      DEBUG("running_timer: %d", running_timer);
    }
  }
  if (next_timer_epoch_ms == -1) {
    running_timer = _io_timer_NO_TIMER;
  }

  s32 timeout_interval_ms;
  u64 now_epoch_ms = now_ms();
  if (next_timer_epoch_ms < now_epoch_ms) {
    // next timer is in the past, lets not wait at all
    timeout_interval_ms = 0;
  } else if (next_timer_epoch_ms == -1) {
    // No timers set, wait infinite time
    timeout_interval_ms = -1;
  } else if (next_timer_epoch_ms > INT_MAX + now_epoch_ms) {
    WARN("edge case: timer set would overflow original timer: %d", running_timer);
    running_timer = _io_timer_NO_TIMER;
    timeout_interval_ms = INT_MAX;
  } else {
    timeout_interval_ms = next_timer_epoch_ms - now_epoch_ms;
  }

  struct epoll_event epes[16];

  int epoll_ret = epoll_wait(io_epoll_fd, epes, COUNT(epes), timeout_interval_ms);
  if (epoll_ret == -1 && errno == EINTR) {
    INFO("Got interrupted system call, restarting wait");
    goto start;
  }
  error_check(epoll_ret);

  if (!epoll_ret) {
    switch (running_timer) {
      #define _(name) case _io_timer_ ## name: { DEBUG(#name "_timeout"); assert(name ## _timeout); name ## _timeout(); } break;
      _IO_TIMERS
      #undef  _
      case _io_timer_NO_TIMER: { WARN("edge case: timer wake up for no timer"); } break;
      case _io_timer_COUNT: case _io_timer_INVALID: default: {
        ERROR("Got wierd value for enum: %d", running_timer);
      } break;
    }
  } else {
    for (int i = 0; i < epoll_ret; i++) {
      struct epoll_event epe = epes[i];
      io_EPData data = {.data = epe.data};
      switch (data.my_data.event_type) {
        #define _(name) case _io_socket_type_ ## name: { DEBUG(#name "_io_event"); assert(name ## _io_event); name ## _io_event(epe); } break;
        _IO_SOCKET_TYPES
        #undef  _
        default: ERROR("Unandled switch case: %d", data.my_data.event_type);
      }
    }
  }
}





