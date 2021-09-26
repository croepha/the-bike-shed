#pragma once

#include <sys/epoll.h>
#include "common.h"

//#define _io_timers_FIRST _io_timer_logging_send
#define _IO_TIMERS \
 _(logging_send) \
 _(shed_pwm) \
 _(io_curl) \
 _(config_download) \
 _(clear_display) \
 _(idle) \

//#define _io_socket_type_FIRST _io_socket_type_io_curl
#define _IO_SOCKET_TYPES \
 _(io_curl) \
 _(supr_signal) \
 _(supr_read_from_child) \
 _(test1) \
 _(serial) \



#define IO_TIMEOUT_CALLBACK(m_name) void m_name ## _timeout(void); void m_name ## _timeout()
//#define IO_EVENT_CALLBACK(m_name, m_arg0) void m_name ## _io_event(struct epoll_event); void m_name ## _io_event(struct epoll_event m_arg0)
#define IO_EVENT_CALLBACK(m_name, m_arg_epoll_events, m_arg_id) void m_name ## _io_event(u32, s32); void m_name ## _io_event(u32 m_arg_epoll_events, s32 m_arg_id)

#define _(name) _io_timer_ ## name,
enum _io_timers { _(INVALID) _IO_TIMERS _(COUNT) _(NO_TIMER) };
#undef _

#define _(name) void name ## _timeout(void) __attribute__((weak_import));
_IO_TIMERS
#undef _

extern u64 io_timers_epoch_ms[];
//#define IO_TIMER_MS(name) io_timers_epoch_ms[_io_timer_ ## name]
// #define IO_TIMER_SET_MS(name, value_ms) ({ io_timers_epoch_ms[_io_timer_ ## name] = value_ms; })
// #define IO_DEBUG_TIMER_MS_GET(name) io_timers_epoch_ms[_io_timer_ ## name]
#define IO_TIMER_SET_MS(name, value_ms) ({ void __io_timer_ms_set__ ## name (u64); __io_timer_ms_set__ ## name (value_ms); })
#define IO_DEBUG_TIMER_MS_GET(name) ({ u64 __io_debug_timer_ms_get__ ## name (void); __io_debug_timer_ms_get__ ## name (); })
#define IO_NOW_MS() now_ms()

#define _(name) _io_socket_type_ ## name ## _fd,
enum _io_socket_types { _(INVALID) _IO_SOCKET_TYPES _(COUNT) };
#undef _

// #define _(name) void name ## _io_event(struct epoll_event) __attribute__((weak_import));
// _IO_SOCKET_TYPES
// #undef  _

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


#define io_ADD_R(fd) io_fd_ctl(EPOLLIN, EPOLL_CTL_ADD, _io_socket_type_ ## fd, 0, fd)
void io_fd_ctl(int flags, int op, enum _io_socket_types type, s32 id, int fd);



void io_initialize(void);
void io_process_events(void);
extern u8  io_idle_has_work;
