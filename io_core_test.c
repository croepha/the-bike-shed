
#include "io.h"
#include "io_test.h"

u64 now_ms() { return real_now_ms(); }


static void log_ep_event(u32 events) {
#define EP_TYPES _(EPOLLIN) _(EPOLLOUT) _(EPOLLRDHUP) _(EPOLLPRI) _(EPOLLERR) _(EPOLLHUP) _(EPOLLET) _(EPOLLONESHOT)
// _(EPOLLWAKEUP)
  char  buf[128];
  char * buf_next = buf;
  char * const buf_END = buf + sizeof buf;
#define _(type) if (events & type && buf_next < buf_END) { buf_next += snprintf(buf_next, buf_END - buf_next, #type " "); }
EP_TYPES
#undef _
  if (buf_next >= buf_END) { strcpy(buf_END -3, "..."); }
  INFO("%s", buf);
}

void io_event_cb(char* name, u32 events, s32 id);

#define TEST_TIMER_TYPES \
 _(test0) \
 _(test1) \
 _(test2) \
 _(test3) \
 _(test4) \
 _(test5) \
 _(test6) \
 _(test7) \

static int const test_timer_type_count = 8;

#define TEST_SOCKET_TYPES \
 _(test0) \
 _(test1) \
 _(test2) \
 _(test3) \
 _(test4) \
 _(test5) \
 _(test6) \
 _(test7) \


IO_TIMEOUT_CALLBACK(idle) {}

#define _(name) \
static void timeout_set_ ## name (u64 value) { IO_TIMER_MS_SET(name, value); } \
static u64 timeout_get_ ## name (void) { return IO_TIMER_MS_DEBUG_GET(name); } \
IO_TIMEOUT_CALLBACK(name) { IO_TIMER_MS_SET(name, -1); events_pending--; INFO("%s Timeout", #name);} \
//end define
TEST_TIMER_TYPES
#undef  _

#define _(name) test_timer_enum__ ## name,
enum { TEST_TIMER_TYPES };
#undef  _

#define _(name) timeout_set_ ## name,
void(*timeout_setters[])(u64) = { TEST_TIMER_TYPES };
#undef  _
#define _(name) timeout_get_ ## name,
u64(*timeout_getters[])(void) = { TEST_TIMER_TYPES };
#undef  _
# define _(name) #name,
char const * const timer_names[] = { TEST_TIMER_TYPES };
# undef  _


#define _(name) IO_EVENT_CALLBACK(name, events, id) { io_event_cb(#name, events, id); }
TEST_SOCKET_TYPES
#undef  _


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

static void echo_test_socket(int i, int type, char const * name) { int r;
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
    for (int i=0; i < test_timer_type_count; i++) {
      timeout_setters[i](1000 + i);
      events_pending ++;
      INFO("timer:%d, %s = %"PRId64, i, timer_names[i], timeout_getters[i]());
    }
  }

  INFO("Running all timers:"); { LOGCTX("\t");
    while (events_pending > 0) { io_process_events(); }
  }

  INFO("Setting timers in decending order:"); { LOGCTX("\t");
    for (int i=0; i < test_timer_type_count; i++) {
      timeout_setters[i](1000 - i);
      events_pending ++;
      INFO("timer:%d, %s = %"PRId64, i, timer_names[i], timeout_getters[i]());
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

  INFO("Setting timers in decending order:"); { LOGCTX("\t");
    for (int i=0; i < test_timer_type_count; i++) {
      timeout_setters[i](1000 - i);
      events_pending ++;
      INFO("timer:%d, %s = %"PRId64, i, timer_names[i], timeout_getters[i]());
    }
  }

  INFO("Running all events:"); { LOGCTX("\t");
    while (events_pending > 0) { io_process_events(); }
  }

}