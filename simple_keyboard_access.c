

#include <stdio.h>
#include <termios.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "config.h"
#include "logging.h"
#include "io.h"
#include "pwm.h"

int console_fd = -1;

char enterred_pin[4] = {};

u32 shed_door_unlock_ms = 2500;

IO_TIMEOUT_CALLBACK(shed_pwm) {
    gpio_pwm_set(0);
}

static void unlock_door() {
    gpio_pwm_set(1);
    //DEBUG("shed_door_unlock_ms:%u", shed_door_unlock_ms);
    IO_TIMER_MS_SET(shed_pwm, IO_NOW_MS() + shed_door_unlock_ms);
}


IO_EVENT_CALLBACK(console, events, ignored_id) {

  memmove(enterred_pin, enterred_pin + 1, sizeof enterred_pin - 1);

  char data = 0;
  int r = read(console_fd, &data, 1);
  error_check(r);

  enterred_pin[3] = data;
  INFO_BUFFER(enterred_pin, sizeof enterred_pin, "key %c", data);

  if (memcmp(enterred_pin, "1848", sizeof enterred_pin) == 0) {
      unlock_door();
  }
}

u64 now_ms() { return real_now_ms(); }
IO_TIMEOUT_CALLBACK(idle) {}


int main () { int r;
  io_initialize();
  fprintf(stderr, "ASDFASDFA\n");
  gpio_pwm_initialize();

  console_fd = open("/dev/tty1", O_RDWR);
  error_check(console_fd);

  { // get term settings, and then update them
    static struct termios ts;
    r = tcgetattr(console_fd, &ts);
    error_check(r);
    ts.c_lflag &= ~ICANON; // disable line buffering
    ts.c_lflag &= ~ECHO;   // disable echo
    r = tcsetattr(console_fd, TCSANOW, &ts);
    error_check(r);
  }

  io_ctl(console_fd, console_fd, 0, EPOLLIN, EPOLL_CTL_ADD);

  for (;;) {
    io_process_events();
  }



}