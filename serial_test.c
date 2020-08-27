
/*

socat PTY,link=test_pty STDIO &


/build/hello_serial.exec test_pty

stty -aF test_pty > /tmp/stty_out
grep --  'speed 115200 baud' /tmp/stty_out
grep --  '-cstopb'            /tmp/stty_out
grep --  '-parenb'           /tmp/stty_out
grep --  'cs8'               /tmp/stty_out

*/

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

//#define error_message(...) ({ printf("ERROR:"); printf(__VA_ARGS__); printf("\n"); fflush(stdout); assert(0); })

// void error_message(const char* fmt, ...) {
//   fprintf(stdout, "ERROR:");
//   va_list va;
//   va_start(va, fmt);
//   vfprintf(stdout, fmt, va);
//   va_end(va);
//   fprintf(stdout, "\n");
//   fflush(stdout);
//   assert(0);
// }


int set_interface_attribs (int fd, int speed, int parity) { int r;
  struct termios tty;
  r = tcgetattr (fd, &tty);
  error_check(r);

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

  r = tcsetattr (fd, TCSANOW, &tty);
  error_check(r);

  return 0;
}

void set_blocking (int fd, int should_block) { int r;
  struct termios tty;
  memset (&tty, 0, sizeof tty);
  r = tcgetattr (fd, &tty);
  error_check(r);

  tty.c_cc[VMIN]  = should_block ? 1 : 0;
  tty.c_cc[VTIME] = 5;      // 0.5 seconds read timeout

  r = tcsetattr (fd, TCSANOW, &tty);
  error_check(r);

}

int open_serial(char const *dev_path) {
  int fd = open(dev_path, O_RDWR | O_NOCTTY | O_SYNC);
  if (fd < 0) {
    ERROR("error %d opening %s: %s", errno, dev_path, strerror(errno));
  }
  error_check(fd);
  return fd;
}


#include <pty.h>


void dump_fds() { int r;
  char cmd[1024];
  r = snprintf(cmd, sizeof cmd, "lsof -p '%d'", getpid());
  error_check(r);
  INFO_BUFFER(cmd, r, "START cmd:");
  assert(r < sizeof cmd);
  r = system(cmd);
  error_check(r);
  INFO("END");
}

void fopen_serial_115200_8n1(char const * path, FILE**inf, FILE**outf) {

  //int fd = open_serial(path);

  int fd, follower;

  //dump_fds();

  int r = openpty(&fd, &follower, 0,0,0);
  error_check(r);

  DEBUG("fd:%d follower:%d", fd, follower);
  //dump_fds();

  pid_t child = fork();
  error_check(child);
  if (!child) {
    r = alarm(10); error_check(r);

    DEBUG("fd:%d follower:%d", fd, follower);
    //dump_fds();

    r = close(fd);
    error_check(r);

    set_interface_attribs (follower, B115200, 0);  // set speed to 115,200 bps, 8n1 (no parity)
    set_blocking (follower, 1);
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

    exit(0);
  }
  r = close(follower);
  error_check(r);


  error_check(fd);
  set_interface_attribs (fd, B115200, 0);  // set speed to 115,200 bps, 8n1 (no parity)
  set_blocking (fd, 1);
  *inf = fdopen(fd, "r");
  int fd2= dup(fd);
  assert(fd2 != -1);
  *outf = fdopen(fd2, "a");
}

void test_main() {
  //char*tty_path = *++argv;
  char* tty_path = 0;
  printf("ASDFASDF\n");

  FILE*inf = 0, *outf = 0;

  fopen_serial_115200_8n1(tty_path, &inf, &outf);

  for (int i=0;i<5;i++) {
    static char *buf = 0;
    static size_t n = 0;
    size_t r1 = getline(&buf, &n, inf);
    error_check(r1);
    //if (buf[n-2] == '\n') buf[n-2] = 0;
    size_t len = strlen(buf);

    INFO_BUFFER(buf, len, "READ: len:%zd buf:", len);

    int r = fprintf(outf, "Message %d:", i);
    error_check(r);
    r = fflush(outf);
    error_check(r);

  }

}


#include <sys/wait.h>

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
