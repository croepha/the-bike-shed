// #define LOG_DEBUG
#include <inttypes.h>
#include <sys/epoll.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>
#include "common.h"
#include "logging.h"
#include "io.h"



#ifndef _IO_TIMERS
#define _IO_TIMERS
#endif

#ifndef _IO_SOCKET_TYPES
#define _IO_SOCKET_TYPES
#endif


/* _IO_SOCKET_TYPES and _IO_TIMERS Defined on command line, like this:
#define _IO_SOCKET_TYPES \
//  _(io_curl) \
//  _(supr_signal) \
// ...
*/

#define _(name) _io_timer_ ## name,
enum _io_timers { _(INVALID) _IO_TIMERS _(idle) _(COUNT) _(NO_TIMER) };
#undef _

#define _(name) void name ## _timeout(void);
_IO_TIMERS _(idle)
#undef _




#define _(name) _io_socket_type_ ## name ## _fd,
enum _io_socket_types { _(INVALID) _IO_SOCKET_TYPES _(COUNT) };
#undef _


#define _(name) void name ## _io_event(u32, s32);
_IO_SOCKET_TYPES
#undef  _

#define _(name) [ _io_timer_ ## name ] = -1,
u64 io_timers_epoch_ms[] = { _(INVALID) _IO_TIMERS  _(idle)};
#undef  _


#define _(name) \
void __io_timer_ms_set__       ## name (u64); \
void __io_timer_ms_set__       ## name (u64 value_ms) { io_timers_epoch_ms[_io_timer_ ## name] = value_ms; } \
u64  __io_timer_ms_debug_get__ ## name (void); \
u64  __io_timer_ms_debug_get__ ## name () { return io_timers_epoch_ms[_io_timer_ ## name]; }

_IO_TIMERS _(idle)
#undef  _


void io_fd_ctl(int flags, int op, enum _io_socket_types type, s32 id, int fd);

#define io_ctl(type, fd, id, flags, op) void __io_ctl__ ## type (s32, s32, s32, s32); __io_ctl__ ## type (fd, id, flags, op)

#define _(type) \
void __io_ctl__ ## type ## _fd (s32, s32, s32, s32); \
void __io_ctl__ ## type ## _fd (s32 fd, s32 id, s32 flags, s32 op) \
{ io_fd_ctl(flags, op, _io_socket_type_ ## type ## _fd, id, fd); }

_IO_SOCKET_TYPES
#undef  _



typedef union {
  epoll_data_t data;
  struct {
    s32 id;
    enum _io_socket_types event_type;
  } my_data;
} io_EPData;


static int _epfd = -1;

u8   io_idle_has_work; // TODO make unit test for this

void io_initialize() {
  io_idle_has_work = 0;
  _epfd = epoll_create1(EPOLL_CLOEXEC);
}

void io_fd_ctl(int flags, int op, enum _io_socket_types type, s32 id, int fd) { int r;
    assert(_epfd != -1);
    io_EPData data = {.my_data = {.id = id, .event_type = type }};
    struct epoll_event epe = {.data = data.data, .events = flags};
    r = epoll_ctl(_epfd, op, fd, &epe);
    error_check(r);
}

#ifdef LOG_DEBUG
static void log_ep_event(struct epoll_event event) {
#define EP_TYPES _(EPOLLIN) _(EPOLLOUT) _(EPOLLRDHUP) _(EPOLLPRI) _(EPOLLERR) _(EPOLLHUP) _(EPOLLET) _(EPOLLONESHOT)
// _(EPOLLWAKEUP)
  char  buf[128];
  char * buf_next = buf;
  char * const buf_END = buf + sizeof buf;
#define _(type) if (event.events & type && buf_next < buf_END) { buf_next += snprintf(buf_next, buf_END - buf_next, #type " "); }
EP_TYPES
#undef _
  if (buf_next >= buf_END) { strcpy(buf_END -3, "..."); }
  DEBUG("%s", buf);
}
#else
#define log_ep_event(...)
#endif // LOG_DEBUG

void io_process_events() { start:;

  u64 now_epoch_ms = IO_NOW_MS();
  if (io_idle_has_work) {
    IO_TIMER_MS_SET(idle, now_epoch_ms + 10);
  }

  // TODO: Instead of calculating the timers each time here, we should really just
  //  have setters for timers and use a min-heap which is inserted into each time we
  //  set a timer, we do that instead of just linear searching the timers for the lowest
  //  whenever we process events, this would be more optimal for several reasons:
  //   - We reuse the sorting work, instead of doing it from scratch
  //   - We move the work from a routine that runs all the time, to a routine that runs rarely
  //   - The complexity of the heap insert should be much better than a linear scan
  //  I am however leaving this terrible implementation as it sits for now, because even
  //   though it is terrible, its super fast for our current needs, as we are operating on
  //   a small scale...

  // TODO: Also I think the timers are using a non monotonic clock, which means that NTP
  //  can mess us up, for internal scheduling, we should really just be using boottime,
  //  or some other monotonic time.  Furthermore, just using a monotonic time is great for
  //  timeouts, throttles, or retry timers, but are actually bad for real time timers.
  //  So, I propose that we have two different APIs, one for "stopwatch" like things, and
  //  annother for "appointments", and we need to get notified on siginificant time changes
  //  We can use timerfd+cancel detection to detect major wall clock adjustments, when we
  //  detect that, we should then call a special callback for "appointments" so that they
  //  can be rescheduled, as there is a new time that they should use as a now reference...
  //  We should start by making some kind of test to prove that this is actually a bug

  enum _io_timers running_timer;
  u64 next_timer_epoch_ms = -1;
  for (int i=1; i < _io_timer_COUNT; i++) {
    DEBUG("timer:%d next:%"PRIu64" timers[i]:%"PRIu64, i, next_timer_epoch_ms, io_timers_epoch_ms[i]);
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

  if (next_timer_epoch_ms < now_epoch_ms) {
    DEBUG("running_timer:%d is in the past:%"PRIu64", not waiting at all", running_timer, next_timer_epoch_ms);
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

  int epoll_ret = epoll_wait(_epfd, epes, COUNT(epes), timeout_interval_ms);
  if (epoll_ret == -1 && errno == EINTR) {
    INFO("Got interrupted system call, restarting wait");
    goto start;
  }
  error_check(epoll_ret);

  if (!epoll_ret) {
    switch (running_timer) {
      #define _(name) case _io_timer_ ## name: { LOGCTX(" timeout:"#name); DEBUG(); \
         io_timers_epoch_ms[_io_timer_ ## name] = -1; assert(name ## _timeout);   name ## _timeout(); } break;
      _IO_TIMERS _(idle)
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
      log_ep_event(epe);
      switch (data.my_data.event_type) {
        #define _(name) case _io_socket_type_ ## name ## _fd: { LOGCTX(" io_event:"#name); DEBUG(); assert(name ## _io_event); name ## _io_event(epe.events, data.my_data.id); } break;
        _IO_SOCKET_TYPES
        #undef  _
        default: ERROR("Unandled switch case: %d", data.my_data.event_type);
      }
    }
  }
}





