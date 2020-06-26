
/*

socat PTY,link=test_pty STDIO &


/build/hello_serial.exec test_pty

stty -aF test_pty > /tmp/stty_out
grep --  'speed 115200 baud' /tmp/stty_out
grep --  '-cstopb'            /tmp/stty_out
grep --  '-parenb'           /tmp/stty_out
grep --  'cs8'               /tmp/stty_out

*/

#include <stddef.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <stdarg.h>

#define error_message(...) ({ printf("ERROR:"); printf(__VA_ARGS__); printf("\n"); fflush(stdout); assert(0); })

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


int set_interface_attribs (int fd, int speed, int parity) {
  struct termios tty;
  if (tcgetattr (fd, &tty) != 0)
  {
    error_message ("error %d from tcgetattr", errno);
    return -1;
  }

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

  if (tcsetattr (fd, TCSANOW, &tty) != 0)
  {
    error_message ("error %d from tcsetattr", errno);
    return -1;
  }
  return 0;
}

void set_blocking (int fd, int should_block) {
  struct termios tty;
  memset (&tty, 0, sizeof tty);
  if (tcgetattr (fd, &tty) != 0)
  {
    error_message ("error %d from tggetattr", errno);
    return;
  }

  tty.c_cc[VMIN]  = should_block ? 1 : 0;
  tty.c_cc[VTIME] = 5;      // 0.5 seconds read timeout

  if (tcsetattr (fd, TCSANOW, &tty) != 0)
    error_message ("error %d setting term attributes", errno);
}

int open_serial(char const *dev_path) {
  int fd = open(dev_path, O_RDWR | O_NOCTTY | O_SYNC);
  if (fd < 0) {
    error_message("error %d opening %s: %s", errno, dev_path, strerror(errno));
  }
  return fd;
}


void fopen_serial_115200_8n1(char const * path, FILE**inf, FILE**outf) {
  int fd = open_serial(path);
  set_interface_attribs (fd, B115200, 0);  // set speed to 115,200 bps, 8n1 (no parity)
  set_blocking (fd, 1);
  *inf = fdopen(fd, "r");
  int fd2= dup(fd);
  assert(fd2 != -1);
  *outf = fdopen(fd2, "a");
}

int main(int argc, char**argv) {
  setbuf(stdout,0);
  setbuf(stderr,0);
  char*tty_path = *++argv;
  printf("ASDFASDF\n");

  FILE*inf = 0, *outf = 0;

  fopen_serial_115200_8n1(tty_path, &inf, &outf);

  for (;;) {
    char *buf = 0;
    size_t n = 0;
    size_t r1 = getline(&buf, &n, inf);
    assert(r1>=0);
    if (buf[n-2] == '\n') buf[n-2] = 0;

    printf("ECHO: %s", buf);

    int r_int = fprintf(outf, "ECHO: %s", buf);
    if (r_int == -1) {
      printf("fprintf error? %s .. %s\n", strerror(errno), strerror(ferror(outf)));
    }

  }

}


