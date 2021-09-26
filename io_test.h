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
#include <inttypes.h>

#include "logging.h"
#include "io.h"


const int socket_COUNT = 20;
int sockets[socket_COUNT];
int events_pending;

__attribute__((unused)) static void sock_read_line(int fd, char * buf, size_t buf_size) {
  ssize_t r = read(fd, buf, buf_size);
  error_check(r);
  INFO_BUFFER(buf, r);
  assert(r > 0);
  assert(buf[r-1] == '\n');
  buf[r-1] = 0;
}

u64 start_time;

// TODO: remove
void io_fd_ctl(int flags, int op, enum _io_socket_types type, s32 id, int fd);


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

    r = close(sv[1]); error_check(r);

    io_fd_ctl(EPOLLIN, EPOLL_CTL_ADD, type, i, sockets[i]);
    INFO("socket id:%02d type:%s:%d", i, name, type);
    events_pending++;
}

void test_main(void);
int main() { int r;
  setlinebuf(stderr);
  r = alarm(3); error_check(r);

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

