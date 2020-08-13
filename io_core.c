#include <sys/epoll.h>

#include <errno.h>
#include <limits.h>
#include <assert.h>
#include "common.h"
#include "logging.h"
#include "io.h"

// main loop should call back downloader_timeout() when we are past this time, ignore if zero
// Measured in miliseconds since  00:00:00 UTC on 1 January 1970.
void download_io_event(struct epoll_event epe); // Call when we get an epoll event where (epe.data.u64 & ((1ull<<32)-1) == EVENT_TYPE_DOWNLOAD
void download_timeout(); // Call on timeout, see download_timer_epoch_ms

#define _(name) [ _io_timer_ ## name ] = -1,
u64 io_timers[] = { _IO_TIMERS };
#undef  _

int io_epoll_fd;

void io_process_events() {
  enum _io_timers running_timer = _io_timer_INVALID;
  u64 next_timer_epoch_ms = -1;
  for (int i=0; i < _io_timer_COUNT; i++) {
    if (next_timer_epoch_ms > io_timers[i]) {
      next_timer_epoch_ms = io_timers[i];
      running_timer = i;
    }
  }

  s32 timeout_ms = -1;
  u64 now_ms = utc_ms_since_epoch();
  if (next_timer_epoch_ms < now_ms) {
    timeout_ms = 0;
  } else if (next_timer_epoch_ms > INT_MAX + now_ms) {
    timeout_ms = INT_MAX;
  } else {
    timeout_ms = next_timer_epoch_ms - now_ms;
  }

  struct epoll_event epes[16];
  int r1 = epoll_wait(io_epoll_fd, epes, COUNT(epes), timeout_ms);
  assert(r1 != -1 || errno == EINTR);

  if (!r1) {
    switch (running_timer) {
      #define _(name) case _io_timer_ ## name: name ## _timeout(); break;
      _IO_TIMERS
      #undef  _
      case _io_timer_COUNT:
      case _io_timer_INVALID:
      default: {
        ERROR("Got wierd value for enum");
      }
    }
  } else {
    for (int i = 0; i < r1; i++) {
      struct epoll_event epe = epes[i];
      io_EPData data = {.data = epe.data};
      switch (data.my_data.event_type) {
        #define _(name) case _io_socket_type_ ## name: name ## _io_event(epe); break;
        _IO_SOCKET_TYPES
        #undef  _
        default: ERROR("Unandled switch case");
      }
    }
  }

}





