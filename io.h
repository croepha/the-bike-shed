#pragma once

#include <sys/epoll.h>
#include "common.h"

#define IO_TIMEOUT_CALLBACK(m_name) void m_name ## _timeout(void); void m_name ## _timeout()
#define IO_EVENT_CALLBACK(m_name, m_arg_epoll_events, m_arg_id) void m_name ## _io_event(u32, s32); void m_name ## _io_event(u32 m_arg_epoll_events, s32 m_arg_id)
#define IO_TIMER_SET_MS(name, value_ms) ({ void __io_timer_ms_set__ ## name (u64); __io_timer_ms_set__ ## name (value_ms); })
#define IO_DEBUG_TIMER_MS_GET(name) ({ u64 __io_debug_timer_ms_get__ ## name (void); __io_debug_timer_ms_get__ ## name (); })
#define IO_NOW_MS() now_ms()

#define io_ADD_R(fd) io_ctl(fd, fd, 0, EPOLLIN, EPOLL_CTL_ADD)
#define io_ctl(type, fd, id, flags, op) void __io_ctl__ ## type (s32, s32, s32, s32); __io_ctl__ ## type (fd, id, flags, op)


void io_initialize(void);
void io_process_events(void);
extern u8  io_idle_has_work;
