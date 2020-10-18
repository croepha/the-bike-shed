#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include "common.h"
#include "logging.h"
#include "io.h"
#include "serial.h"
#include <inttypes.h>


u64 now_ms() { return real_now_ms(); }


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

int serial_fd = -1;

void serial_io_event(struct epoll_event event_info) {
    log_ep_event(event_info);
    char buf[1024];
    ssz size = read(serial_fd, buf, sizeof buf);
    error_check(size);
    INFO_BUFFER(buf, size);
}

void logging_send_timeout() {
    INFO();
    dprintf(serial_fd, "TEST TEST\n");
    IO_TIMER_MS(logging_send) = (now_sec() + 1) * 1000;
}

int main (int argc, char ** argv) {
    setlinebuf(stderr);
    INFO("STARTED %"PRIu64, now_sec());
    char * dev_path = *++argv;

    assert(dev_path);

    io_initialize();

    serial_fd = serial_open_115200_8n1(dev_path);

    IO_TIMER_MS(logging_send) = (now_sec() + 1) * 1000;

    io_ADD_R(serial_fd);

    for(;;) {
        io_process_events();
    }

}