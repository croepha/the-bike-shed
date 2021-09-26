
#include "io.h"
#include "io_test.h"

u64 now_ms() { return real_now_ms(); }


static void log_ep_event(u32 events) {
#define EP_TYPES _(EPOLLIN) _(EPOLLOUT) _(EPOLLRDHUP) _(EPOLLPRI) _(EPOLLERR) _(EPOLLHUP) _(EPOLLET) _(EPOLLONESHOT)
// _(EPOLLWAKEUP)
  char  buf[128];
  char * buf_next = buf;
  char * const buf_END = buf + sizeof buf;
// TODO: Put space here
#define _(type) if (events & type && buf_next < buf_END) { buf_next += snprintf(buf_next, buf_END - buf_next, #type); }
EP_TYPES
#undef _
  if (buf_next >= buf_END) { strcpy(buf_END -3, "..."); }
  INFO("%s", buf);
}


static void timeout_cb(char* name, enum _io_timers timer) {
  io_timers_epoch_ms[timer] = -1;
  events_pending --;
  INFO("%s Timeout", name);
}

#define _(name) void name ## _timeout() { timeout_cb(#name, _io_timer_  ## name); }
_IO_TIMERS
#undef  _

void io_event_cb(char* name, u32 events, s32 id);

// // TODO remove
// #define _(name) void name ## _io_event(struct epoll_event) __attribute__((weak_import));
// _IO_SOCKET_TYPES
// #undef  _

// #define _(name) void name ## _io_event(struct epoll_event epe) { io_event_cb(#name, epe); }
// _IO_SOCKET_TYPES
// #undef  _


#define TEST_SOCKET_TYPES \
 _(test0) \
 _(test1) \
 _(test2) \
 _(test3) \
 _(test4) \
 _(test5) \
 _(test6) \
 _(test7) \


#define _(name) IO_EVENT_CALLBACK(name, events, id) { io_event_cb(#name, events, id); }
TEST_SOCKET_TYPES
#undef  _


# define _(name) #name,
char const * const timer_names[] = { _IO_TIMERS };
# undef  _

# define _(name) &io_timers_epoch_ms[_io_timer_ ## name],
u64 * const timers[] = { _IO_TIMERS };
# undef  _

# define _(name) #name,
char const * const socket_type_names[] = { TEST_SOCKET_TYPES };
# undef  _

void io_event_cb(char* name, u32 events, s32 id) { int r;
  int i = id;
  LOGCTX("test_sort:id:%02d", i);
  char buf[256]; sock_read_line(sockets[i], buf, sizeof buf);

  INFO("IO Event %s buf:'%s'", name, buf);
  { LOGCTX("\t"); log_ep_event(events); }
  r = dprintf(sockets[id], "REPLY%02d\n", id);
  error_check(r);
  r = close(sockets[id]);
  error_check(r);
  events_pending--;
}

__attribute__((unused)) static void echo_test_socket(int i, int type, char const * name) { int r;
    int sv[2] = {-1,-1};
    r = socketpair(AF_UNIX, SOCK_DGRAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0, sv); error_check(r);
    sockets[i] = sv[0];

    pid_t fork_pid = fork();
    error_check(fork_pid);

    if (!fork_pid) { LOGCTX("\ttest_sort:id:%02d forked", i);
      r = close(sv[0]); error_check(r);

      u64 now = now_ms();
      if (start_time > now) {
        r = usleep((start_time - now) * 1000); error_check(r);
      }

      int sock = sv[1];
      long initial_flags = fcntl(sock, F_GETFL, 0); error_check(initial_flags);
      r = fcntl(sock, F_SETFL, initial_flags & ~O_NONBLOCK); //set nonblock
      error_check(r);

      r = dprintf(sock, "SEND%02d\n", i); error_check(r);
      error_check(r);
      char buf[256]; sock_read_line(sock, buf, sizeof buf);
      exit(0);
    }

    switch (type) {
      case 0: { io_ctl(test0_fd, sockets[i], i, EPOLLIN, EPOLL_CTL_ADD); } break;
      case 1: { io_ctl(test1_fd, sockets[i], i, EPOLLIN, EPOLL_CTL_ADD); } break;
      case 2: { io_ctl(test2_fd, sockets[i], i, EPOLLIN, EPOLL_CTL_ADD); } break;
      case 3: { io_ctl(test3_fd, sockets[i], i, EPOLLIN, EPOLL_CTL_ADD); } break;
      case 4: { io_ctl(test4_fd, sockets[i], i, EPOLLIN, EPOLL_CTL_ADD); } break;
      case 5: { io_ctl(test5_fd, sockets[i], i, EPOLLIN, EPOLL_CTL_ADD); } break;
      case 6: { io_ctl(test6_fd, sockets[i], i, EPOLLIN, EPOLL_CTL_ADD); } break;
      case 7: { io_ctl(test7_fd, sockets[i], i, EPOLLIN, EPOLL_CTL_ADD); } break;
    }

    INFO("socket id:%02d type:%s", i, name);

    r = close(sv[1]); error_check(r);

    events_pending++;
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

  start_time = now_ms() + 50;

  for (int i = 0; i < socket_COUNT; i++) {
    int type_i = i % 8;
    echo_test_socket(i, type_i, socket_type_names[type_i]);
  }

  // io_ctl(test0_fd, sockets[0], 0, EPOLLIN, EPOLL_CTL_ADD);
  // io_ctl(test1_fd, sockets[1], 1, EPOLLIN, EPOLL_CTL_ADD);
  // io_ctl(test2_fd, sockets[2], 2, EPOLLIN, EPOLL_CTL_ADD);
  // io_ctl(test3_fd, sockets[3], 3, EPOLLIN, EPOLL_CTL_ADD);
  // io_ctl(test4_fd, sockets[4], 4, EPOLLIN, EPOLL_CTL_ADD);
  // io_ctl(test5_fd, sockets[5], 5, EPOLLIN, EPOLL_CTL_ADD);
  // io_ctl(test6_fd, sockets[6], 6, EPOLLIN, EPOLL_CTL_ADD);
  // io_ctl(test7_fd, sockets[7], 7, EPOLLIN, EPOLL_CTL_ADD);

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