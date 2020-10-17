
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>



#include "line_accumulator.h"
#include "logging.h"
#include "serial.h"


u8 lines_seen;
int fd = -1;

// static void handle_line(char* buf) { int r;
//   size_t len = strlen(buf);

//   INFO_BUFFER(buf, len, "READ: len:%zd buf:", len);

//   r = dprintf(fd, "Message %d:", lines_seen);
//   error_check(r);
//   lines_seen++;
// }


int main() { int r;
  setlinebuf(stderr);
  setlinebuf(stdout);
  r = alarm(3); error_check(r);

  // Clean out the pipes
  system("timeout .1 cat /build/exterior_mock.pts2 > /dev/null; timeout .1 cat /build/exterior_mock.pts > /dev/null");

  char* tty_path = "/build/exterior_mock.pts";
  printf("ASDFASDF\n");

  fd = serial_open_115200_8n1(tty_path);
  FILE * inf = fdopen(fd, "r");


  int epoll_fd = epoll_create1(EPOLL_CLOEXEC);
  error_check(epoll_fd);
  struct epoll_event epe = { .events = EPOLLIN};
  r = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &epe);
  error_check(r);
  r = epoll_wait(epoll_fd, &epe, 1, 0);
  error_check(r);



  {
    pid_t child = fork();
    error_check(child);
    if (!child) {
      int follower = open("/build/exterior_mock.pts2", O_RDWR);

      r = alarm(10); error_check(r);

      DEBUG("fd:%d follower:%d", fd, follower);

      char sbuf[1024];
      int sbuf_len = snprintf(sbuf, sizeof sbuf, "INITIAL\n");
      error_check(sbuf_len);
      assert(sbuf_len < sizeof sbuf);
      ssz sb = write(follower, sbuf, sbuf_len);
      error_check(sb);
      assert(sb == sbuf_len);

      for (int i = 0; i<5; i++) { LOGCTX("forked:");
        char rbuf[512];
        DEBUG("follower:%d", follower);
        //dump_fds();

        ssz read_count = read(follower, rbuf, sizeof rbuf -1);
        error_check(read_count);
        INFO_BUFFER(rbuf, read_count, "Child did read: len:%zd rbuf:", read_count);
        rbuf[read_count] = 0;
        sbuf_len = snprintf(sbuf, sizeof sbuf, "REPLY: %s\n", rbuf);
        error_check(sbuf_len);
        assert(sbuf_len < sizeof sbuf);
        sb = write(follower, sbuf, sbuf_len);
        error_check(sb);
        assert(sb == sbuf_len);
      }
      r = close(follower);
      error_check(r);
      exit(0);
    }
  }

  // struct line_accumulator_Data line_data = {};

  // while(lines_seen < 5) {
  //   char buf[line_accumulator_Data_SIZE/2];
  //   ssz len = read(fd, buf, sizeof buf);
  //   error_check(len);
  //   line_accumulator(&line_data, buf, len, handle_line);
  // }



  for (int i=0;i<5;i++) {
    static char *buf = 0;
    static size_t n = 0;
    size_t r1 = getline(&buf, &n, inf);
    error_check(r1);
    //if (buf[n-2] == '\n') buf[n-2] = 0;
    size_t len = strlen(buf);

    INFO_BUFFER(buf, len, "READ: len:%zd buf:", len);

    r = dprintf(fd, "Message %d:", i);
    error_check(r);

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
