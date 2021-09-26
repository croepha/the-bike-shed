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

