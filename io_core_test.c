
#include "io_test.h"

static void log_ep_event(struct epoll_event event) {
#define EP_TYPES _(EPOLLIN) _(EPOLLOUT) _(EPOLLRDHUP) _(EPOLLPRI) _(EPOLLERR) _(EPOLLHUP) _(EPOLLET) _(EPOLLONESHOT)
// _(EPOLLWAKEUP)
  char  buf[128];
  char * buf_next = buf;
  char * const buf_END = buf + sizeof buf;
#define _(type) if (event.events & type && buf_next < buf_END) { buf_next += snprintf(buf_next, buf_END - buf_next, #type); }
EP_TYPES
#undef _
  if (buf_next >= buf_END) { strcpy(buf_END -3, "..."); }
  INFO("%s", buf);
}

int events_pending;

void timeout_cb(char* name, enum _io_timers timer) {
  io_timers_epoch_ms[timer] = -1;
  events_pending --;
  INFO("%s Timeout", name);
}

#define _(name) void name ## _timeout() { timeout_cb(#name, _io_timer_  ## name); }
_IO_TIMERS
#undef  _

void io_event_cb(char* name, struct epoll_event epe);

#define _(name) void name ## _io_event(struct epoll_event epe) { io_event_cb(#name, epe); }
_IO_SOCKET_TYPES
#undef  _

# define _(name) #name,
char const * const timer_names[] = { _IO_TIMERS };
# undef  _

# define _(name) &io_timers_epoch_ms[_io_timer_ ## name],
u64 * const timers[] = { _IO_TIMERS };
# undef  _

# define _(name) #name,
char const * const socket_type_names[] = { _IO_SOCKET_TYPES };
# undef  _

# define _(name) _io_socket_type_ ## name,
enum _io_socket_types const socket_types[] = { _IO_SOCKET_TYPES };
# undef  _

void io_event_cb(char* name, struct epoll_event epe) { int r;
  io_EPData data = { .data = epe.data };
  int i = data.my_data.id;
  LOGCTX("test_sort:id:%02d", i);
  char buf[256]; sock_read_line(sockets[i], buf, sizeof buf);

  INFO("IO Event %s type:%d buf:'%s'", name, data.my_data.event_type, buf);
  { LOGCTX("\t"); log_ep_event(epe); }
  r = dprintf(sockets[data.my_data.id], "REPLY%02d\n", data.my_data.id);
  error_check(r);
  r = close(sockets[data.my_data.id]);
  error_check(r);
  events_pending--;
}


void test_main() {
  io_initialize();

  INFO("Setting timers in acending order:"); { LOGCTX("\t");
    for (int i=0; i < COUNT(timers); i++) {
      *timers[i] = 1000 + i;
      events_pending ++;
      INFO("timer:%d, %s = %"PRId64, (s32)(timers[i] - io_timers_epoch_ms), timer_names[i], *timers[i]);
    }
  }

  INFO("Running all timers:"); { LOGCTX("\t");
    while (events_pending > 0) { io_process_events(); }
  }

  INFO("Setting timers in decending order:"); { LOGCTX("\t");
    for (int i=0; i < COUNT(timers); i++) {
      *timers[i] = 1000 - i;
      events_pending ++;
      INFO("timer:%d, %s = %"PRId64, (int)(timers[i] - io_timers_epoch_ms), timer_names[i], *timers[i]);
    }
  }

  INFO("Running all timers:"); { LOGCTX("\t");
    while (events_pending > 0) { io_process_events(); }
  }

  start_time = utc_ms_since_epoch() + 50;

  for (int i = 0; i < socket_COUNT; i++) {
    int type_i = i % COUNT(socket_types);
    echo_test_socket(i, socket_types[type_i], socket_type_names[type_i]);
  }

  INFO("Setting timers in decending order:"); { LOGCTX("\t");
    for (int i=0; i < COUNT(timers); i++) {
      *timers[i] = start_time - i;
      events_pending ++;
    }
  }

  INFO("Running all events:"); { LOGCTX("\t");
    while (events_pending > 0) { io_process_events(); }
  }

}