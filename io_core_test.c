
#include <sys/epoll.h>
#include <stdio.h>
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

}

#define _(name) void name ## _timeout() { INFO(#name " Timeout"); }
_IO_TIMERS
#undef  _

#define _(name) void name ## _io_event(struct epoll_event epe) { INFO(#name " IO Event"); log_ep_event(epe); }
_IO_SOCKET_TYPES
#undef  _


int main() {

  INFO("We have these timers:");
# define _(name)  INFO(#name);
  _IO_TIMERS
# undef  _


}