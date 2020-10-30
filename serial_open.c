
#include <errno.h>
#include <fcntl.h>
#include <termios.h>
#define CRTSCTS       020000000000  /* flow control */
#include "logging.h"
#include "serial.h"


int serial_open_115200_8n1(char const * path) { int r;
  int fd = open(path, O_RDWR | O_NOCTTY | O_SYNC ); // | O_NONBLOCK
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
//  tty.c_cflag |= IGNPAR;

//  cfmakeraw(&tty);

  r = tcsetattr (fd, TCSANOW, &tty);
  error_check(r);

  return fd;

}

