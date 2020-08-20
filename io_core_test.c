#define _GNU_SOURCE
#define LOG_DEBUG
#include <sys/epoll.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/un.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>

#include "logging.h"
#include "io.h"


static void log_ep_event(struct epoll_event event) {
#define EP_TYPES _(EPOLLIN) _(EPOLLOUT) _(EPOLLRDHUP) _(EPOLLPRI) _(EPOLLERR) _(EPOLLHUP) _(EPOLLET) _(EPOLLONESHOT) _(EPOLLWAKEUP)
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

const int socket_COUNT = 20;
int sockets[socket_COUNT];

void sock_read_line(int fd, char * buf, size_t buf_size) {
  ssize_t r = read(fd, buf, buf_size);
  error_check(r);
  //DEBUG("fd:%d buf_size:`%.*s`", fd, (int)r, buf);
  assert(r > 0);
  assert(buf[r-1] == '\n');
  buf[r-1] = 0;
}

void io_event_cb(char* name, struct epoll_event epe) { int r;
  io_EPData data = { .data = epe.data };
  int i = data.my_data.id;
  char buf[256]; sock_read_line(sockets[i], buf, sizeof buf);

  INFO("IO Event %s type:%d buf:'%s'", name, data.my_data.event_type, buf);
  { LOGCTX("\t"); log_ep_event(epe); }
  r = dprintf(sockets[data.my_data.id], "REPLY%02d\n", data.my_data.id);
  assert(r != -1);
  r = close(sockets[data.my_data.id]);
  assert(r != -1);
  events_pending--;
}


int main() { int r;
  setlinebuf(stderr);
  r = alarm(1); error_check(r);
  io_initialize();

  INFO("Setting timers in acending order:"); { LOGCTX("\t");
    for (int i=0; i < COUNT(timers); i++) {
      *timers[i] = 1000 + i;
      events_pending ++;
      INFO("timer:%ld, %s = %ld", timers[i] - io_timers_epoch_ms, timer_names[i], *timers[i]);
    }
  }

  INFO("Running all timers:"); { LOGCTX("\t");
    while (events_pending > 0) { io_process_events(); }
  }

  INFO("Setting timers in decending order:"); { LOGCTX("\t");
    for (int i=0; i < COUNT(timers); i++) {
      *timers[i] = 1000 - i;
      events_pending ++;
      INFO("timer:%ld, %s = %ld", timers[i] - io_timers_epoch_ms, timer_names[i], *timers[i]);
    }
  }

  INFO("Running all timers:"); { LOGCTX("\t");
    while (events_pending > 0) { io_process_events(); }
  }

  u64 start_time = utc_ms_since_epoch() + 50;

  for (int i = 0; i < socket_COUNT; i++) {
    int type_i = i % COUNT(socket_types);

    int sv[2] = {-1,-1};
    r = socketpair(AF_UNIX, SOCK_DGRAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0, sv); error_check(r);
    sockets[i] = sv[0];

    pid_t fork_pid = fork();
    assert(fork_pid != -1);
    if (!fork_pid) { LOGCTX("forked:%02d", i);
      r = close(sv[0]); error_check(r);

      u64 now = utc_ms_since_epoch();
      if (start_time > now) {
        r = usleep((start_time - now) * 1000); error_check(r);
      }

      int sock = sv[1];
      long initial_flags = fcntl(sock, F_GETFL, 0); error_check(initial_flags);
      r = fcntl(sock, F_SETFL, initial_flags & ~O_NONBLOCK); //set nonblock
      error_check(r);

      r = dprintf(sock, "SEND%02d\n", i); error_check(r);
      assert(r != -1);
      char buf[256]; sock_read_line(sock, buf, sizeof buf);
      exit(0);
    }

    r = close(sv[1]); error_check(r);

    io_EPData data;
    data.my_data.id = i;
    data.my_data.event_type = socket_types[type_i];
    struct epoll_event epe = { .events = EPOLLIN, .data = data.data};
    r = epoll_ctl(io_epoll_fd, EPOLL_CTL_ADD, sockets[i], &epe); error_check(r);

    INFO("socket:%d type:%s:%d", i, socket_type_names[type_i], socket_types[type_i]);
    events_pending++;
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

  INFO("Reaping child procs");
  u8 had_error = 0;
  for (;;) {
    int wstatus;
    //DEBUG("Waiting for child");
    pid_t child = wait(&wstatus);
    if (child == -1 && errno == ECHILD) {
      break;
    }
    error_check(child);
    //INFO("Child exit:%d", WEXITSTATUS(wstatus));
    if (! WIFEXITED(wstatus) || WEXITSTATUS(wstatus) != 0) {
      had_error = 1;
    }
  }
  if (had_error) {
    ERROR("Atleast one child process had an error");
  }
}