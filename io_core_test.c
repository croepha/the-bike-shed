#define _GNU_SOURCE

#include <sys/epoll.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
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

#define _(name) void name ## _timeout() { INFO(#name " Timeout"); io_timers_epoch_ms[_io_timer_  ## name] = -1; }
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

int sockets_server[20];
int sockets_connected[20];

void io_event_cb(char* name, struct epoll_event epe) {
  io_EPData data = { .data = epe.data };
  int i = data.my_data.id;
  if (sockets_server[data.my_data.id] != -2) {
    sockets_connected[data.my_data.id] = accept4(sockets_server[data.my_data.id], 0, 0, SOCK_NONBLOCK | SOCK_CLOEXEC);
    assert(sockets_connected[data.my_data.id] != -1);
    io_EPData data2;
    data2.my_data.id = i;
    data2.my_data.event_type = socket_types[data.my_data.event_type];
    int r;
    struct epoll_event epe2 = { .events = EPOLLIN, .data = data2.data};
    r = epoll_ctl(io_epoll_fd, EPOLL_CTL_ADD, sockets_server[i], &epe2);
    assert(r != -1);
    r = close(sockets_server[data.my_data.id]);
    assert(r != -1);
    INFO("(server) IO Event %s id:%d type:%d", name, data.my_data.id, data.my_data.event_type);
    log_ep_event(epe);
  } else {
    char buf[256];
    ssize_t r = read(sockets_connected[data.my_data.id], buf, sizeof buf);
    assert(r != -1);
    buf[r] = 0;
    char* nl = strchr(buf, '\n');
    if (nl) {
      *nl = 0;
    }
    INFO("(connected) IO Event %s id:%d type:%d buf:'%s'", name, data.my_data.id, data.my_data.event_type, buf);
    log_ep_event(epe);
    r = dprintf(sockets_connected[data.my_data.id], "REPLY%d", data.my_data.id);
    assert(r != -1);
    r = close(sockets_connected[data.my_data.id]);
    assert(r != -1);
  }
}



int main() {

  io_initialize();

  INFO("Setting timers in acending order:"); { LOGCTX("\t");
    for (int i=0; i < COUNT(timers); i++) {
      *timers[i] = 1000 + i;
      INFO("timer:%ld, %s = %ld", timers[i] - io_timers_epoch_ms, timer_names[i], *timers[i]);
    }
  }

  INFO("Running all timers:"); { LOGCTX("\t");
    for (int i=0; i < COUNT(timers); i++) {
      io_process_events();
    }
  }

  INFO("Setting timers in decending order:"); { LOGCTX("\t");
    for (int i=0; i < COUNT(timers); i++) {
      *timers[i] = 1000 - i;
      INFO("timer:%ld, %s = %ld", timers[i] - io_timers_epoch_ms, timer_names[i], *timers[i]);
    }
  }

  INFO("Running all timers:"); { LOGCTX("\t");
    for (int i=0; i < COUNT(timers); i++) {
      io_process_events();
    }
  }

  for (int i = 0; i < COUNT(sockets_server); i++) {
    int type_i = i % COUNT(socket_types);

    int r;
    struct sockaddr_un addr = { .sun_family = AF_UNIX };
    r = snprintf(addr.sun_path, sizeof addr.sun_path - 1, "/tmp/test_%d", i);
    assert(r > 0);
    assert(r < sizeof addr.sun_path - 1);
    unlink(addr.sun_path);

    sockets_server[i] =
        socket(AF_UNIX, SOCK_SEQPACKET | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    assert(sockets_server[i] != -1);
    r = bind(sockets_server[i], (struct sockaddr const *)&addr, sizeof addr);
    assert(r != -1);
    r = listen(sockets_server[i], 1);
    assert(r != -1);

    io_EPData data;
    data.my_data.id = i;
    data.my_data.event_type = socket_types[type_i];
    struct epoll_event epe = { .events = EPOLLIN, .data = data.data};
    r = epoll_ctl(io_epoll_fd, EPOLL_CTL_ADD, sockets_server[i], &epe);
    assert(r != -1);

    INFO("socket:%d type:%s:%d", i, socket_type_names[type_i], socket_types[type_i]);
  }
  INFO("All test sockets created");

  for (;;) {
    io_process_events();
  }





}