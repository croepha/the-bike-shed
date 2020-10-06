
#include <curl/curl.h>
#include <stddef.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>

#include "logging.h"

#define CRTSCTS       020000000000  /* flow control */




static int open_serial_115200_8n1(char const * path) { int r;
  int fd = open(path, O_RDWR | O_NOCTTY | O_SYNC);
  if (fd < 0) {
    ERROR("error %d opening %s: %s", errno, path, strerror(errno));
  }
  error_check(fd);

  struct termios tty;
  r = tcgetattr (fd, &tty);
  error_check(r);

  // set speed to 115,200 bps, 8n1 (no parity)
  int const speed = B115200;
  int const parity = 0;

  cfsetospeed (&tty, speed);
  cfsetispeed (&tty, speed);

  tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
  // disable IGNBRK for mismatched speed tests; otherwise receive break
  // as \000 chars
  tty.c_iflag &= ~IGNBRK;   // disable break processing
  tty.c_lflag = 0;    // no signaling chars, no echo,
    // no canonical processing
  tty.c_oflag = 0;    // no remapping, no delays
  tty.c_cc[VMIN]  = 0;      // read doesn't block
  tty.c_cc[VTIME] = 5;      // 0.5 seconds read timeout

  tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

  tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
    // enable reading
  tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
  tty.c_cflag |= parity;
  tty.c_cflag &= ~CSTOPB;
  tty.c_cflag &= ~CRTSCTS;

  int should_block = 0;
  tty.c_cc[VMIN]  = should_block ? 1 : 0;
  tty.c_cc[VTIME] = 5;      // 0.5 seconds read timeout

  r = tcsetattr (fd, TCSANOW, &tty);
  error_check(r);

  return fd;

}




#include <sys/wait.h>

int main() { int r;
  setlinebuf(stderr);
  setlinebuf(stdout);
  r = alarm(3); error_check(r);

  // Clean out the pipes
  system("timeout .1 cat /build/exterior_mock.pts2 > /dev/null; timeout .1 cat /build/exterior_mock.pts > /dev/null");

  char* tty_path = "/build/exterior_mock.pts";
  printf("ASDFASDF\n");

  int fd = open_serial_115200_8n1(tty_path);
  FILE * inf = fdopen(fd, "r");
  int fd2= dup(fd);
  assert(fd2 != -1);
  FILE * outf = fdopen(fd2, "a");

//  read()

  {
    pid_t child = fork();
    error_check(child);
    if (!child) {
      int follower = open("/build/exterior_mock.pts2", O_RDWR);

      r = alarm(10); error_check(r);

      DEBUG("fd:%d follower:%d", fd, follower);
      //dump_fds();

      //r = close(fd);
      //error_check(r);

      //set_interface_attribs (follower, B115200, 0);  // set speed to 115,200 bps, 8n1 (no parity)
      //set_blocking (follower, 1);
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


  for (int i=0;i<5;i++) {
    static char *buf = 0;
    static size_t n = 0;
    size_t r1 = getline(&buf, &n, inf);
    error_check(r1);
    //if (buf[n-2] == '\n') buf[n-2] = 0;
    size_t len = strlen(buf);

    INFO_BUFFER(buf, len, "READ: len:%zd buf:", len);

    r = fprintf(outf, "Message %d:", i);
    error_check(r);
    r = fflush(outf);
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
