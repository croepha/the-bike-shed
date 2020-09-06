

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/un.h>
#include "logging.h"
#include <sys/socket.h>
#include <fcntl.h>



int events_pending;

u64 start_time;

int sockets[20];

void sock_read_line(int fd, char * buf, size_t buf_size) {
  ssize_t r = read(fd, buf, buf_size);
  error_check(r);
  //DEBUG("fd:%d buf_size:`%.*s`", fd, (int)r, buf);
  assert(r > 0);
  assert(buf[r-1] == '\n');
  buf[r-1] = 0;
}

int echo_test_socket(int i, int type) { int r;
    int sv[2] = {-1,-1};
    r = socketpair(AF_UNIX, SOCK_DGRAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0, sv); error_check(r);
    sockets[i] = sv[0];

    pid_t fork_pid = fork();
    error_check(fork_pid);

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
      error_check(r);
      char buf[256]; sock_read_line(sock, buf, sizeof buf);
      exit(0);
    }

    r = close(sv[1]); error_check(r);

    io_EPData data;
    data.my_data.id = i;
    data.my_data.event_type = socket_types[type_i];
    struct epoll_event epe = { .events = EPOLLIN, .data = data.data};
    r = epoll_ctl(io_epoll_fd, EPOLL_CTL_ADD, sockets[i], &epe); error_check(r);

    INFO("socket id:%02d type:%s:%d", i, socket_type_names[type_i], socket_types[type_i]);
    events_pending++;
}


void test_main() {
    start_time = utc_ms_since_epoch() + 50;
}


int main() { int r;
  setlinebuf(stderr);
  r = alarm(1); error_check(r);

  test_main();

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