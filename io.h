#pragma once

#include <sys/epoll.h>
#include "common.h"

extern int io_epoll_fd;

//#define _io_timers_FIRST _io_timer_logging_send
#define _IO_TIMERS \
 _(logging_send) \
 _(io_curl) \

//#define _io_socket_type_FIRST _io_socket_type_io_curl
#define _IO_SOCKET_TYPES \
 _(io_curl) \
 _(supr_signal) \
 _(supr_read_from_child) \
 _(test1) \


#define _(name) _io_timer_ ## name,
enum _io_timers { _(INVALID) _IO_TIMERS _(COUNT) _(NO_TIMER) };
#undef _
#define _(name) void name ## _timeout(void) __attribute__((weak_import));
_IO_TIMERS
#undef _
extern u64 io_timers_epoch_ms[];
#define IO_TIMER_MS(name) io_timers_epoch_ms[_io_timer_ ## name]


#define _(name) _io_socket_type_ ## name ## _fd,
enum _io_socket_types { _(INVALID) _IO_SOCKET_TYPES _(COUNT) };
#undef _
#define _(name) void name ## _io_event(struct epoll_event) __attribute__((weak_import));
_IO_SOCKET_TYPES
#undef  _
typedef union {
  epoll_data_t data;
  struct {
    s32 id;
    enum _io_socket_types event_type;
  } my_data;
} io_EPData;

#include <errno.h>
#include <sys/epoll.h>
#include "logging.h"
__attribute__((unused))
static void __io_set(int flags, int op, enum _io_socket_types type, int fd) { int r;
    io_EPData data = {.my_data = { .event_type = type }};
    struct epoll_event epe = {.data = data.data, .events = flags};
    r = epoll_ctl(io_epoll_fd, op, fd, &epe); error_check(r);
}

__attribute__((unused))
static void __io_set(int flags, int op, enum _io_socket_types type, int fd) { int r;
    io_EPData data = {.my_data = { .event_type = type }};
    struct epoll_event epe = {.data = data.data, .events = flags};
    r = epoll_ctl(io_epoll_fd, op, fd, &epe); error_check(r);
}


#define io_ADD_R(fd) __io_set(EPOLLIN, EPOLL_CTL_ADD, _io_socket_type_ ## fd, fd)




void io_initialize(void);
void io_process_events(void);


