


#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/epoll.h>
#include "common.h"
#include "logging.h"
#include "io.h"

static void _lcd_set_linenf(u8 line_i, char const *fmt, ...) __attribute__ ((format (printf, 2, 3)));
#define lcd_set_line0(fmt, ...) _lcd_set_linenf(0, fmt, ##__VA_ARGS__)
#define lcd_set_line1(fmt, ...) _lcd_set_linenf(1, fmt, ##__VA_ARGS__)
#define lcd_set_line2(fmt, ...) _lcd_set_linenf(2, fmt, ##__VA_ARGS__)
#define lcd_set_line3(fmt, ...) _lcd_set_linenf(3, fmt, ##__VA_ARGS__)


#include <termios.h>
#include <stdio.h>

u64 now_ms() { return real_now_ms(); }
IO_TIMEOUT_CALLBACK(idle) {}


static char lcd_buffer[4][20];
void _lcd_set_linenf(u8 line_i, char const *fmt, ...) {

  assert(line_i < 4);

  char buf[21];
  memset(buf, ' ', sizeof buf);

  va_list va;
  va_start(va, fmt);
  int needed_len = vsnprintf(buf, sizeof buf, fmt, va);
  va_end(va);

  if (needed_len > sizeof lcd_buffer[line_i]) {
    WARN("lcd line too long");
  }
  memcpy(lcd_buffer[line_i], buf, 20);
}

u16 ansi_used;
char ansi_buf[1<<15];

__attribute__ ((format (printf, 1, 2)))
static void ansi_fmt(char const *fmt, ...)  {
  va_list va;
  va_start(va, fmt);
  assert(ansi_used < sizeof ansi_buf);
  int needed_len = vsnprintf(ansi_buf + ansi_used, sizeof ansi_buf - ansi_used, fmt, va);
  va_end(va);

  error_check(needed_len);

  ansi_used += needed_len;
  assert(ansi_used < sizeof ansi_buf);
}

static void ansi_flush() {
  fwrite(ansi_buf, ansi_used, 1, stderr);
  ansi_used = 0;
  fflush(stderr);
}

static void ansi_cursor_move_up(int line_count) {
  ansi_fmt("\033[%dA", line_count);
}

static void ansi_clear_lines(int line_count) {
  for (int li = 0; li < line_count; li++) {
    ansi_fmt("\033[K\n"); // clear line, move to next
  }
  ansi_cursor_move_up(line_count);
}



void pin_backlight_pwm_set(u32 value);

void got_keypad_input(char key);
void loop(void);

long till_autosleep_ms = 0;
unsigned int backlight_level = 0;
unsigned int backlight_level_MAX = 32 * 4 * 256;


void got_keypad_input(char key) {
  till_autosleep_ms = 5000;
  backlight_level = backlight_level_MAX;
}

unsigned long last_uptime_ms = 0;

void loop() {

  unsigned long uptime_delta_ms_old = last_uptime_ms;
  last_uptime_ms = now_ms();
  long uptime_delta_ms = last_uptime_ms - uptime_delta_ms_old;
  if (uptime_delta_ms > 10000) { uptime_delta_ms = 10; } // Overflow or reset...

  if (till_autosleep_ms > 0) {
    till_autosleep_ms -= uptime_delta_ms;
    if (till_autosleep_ms <= 0) {
       till_autosleep_ms = 0;
       //reset_input();
    }
  }

  pin_backlight_pwm_set(backlight_level);

  if (till_autosleep_ms <= 0 && backlight_level > 0) {
    unsigned long delta_level = (backlight_level_MAX / 1000) * uptime_delta_ms;
    if (delta_level > backlight_level) {
      backlight_level = 0;
    } else {
      backlight_level -= delta_level;
    }
  }

  fprintf(stderr, "sim loop\n");
}

// --------------

u32 _pin_backlight_pwm_value;
char last_input[20];

void pin_backlight_pwm_set(u32 value) {
  _pin_backlight_pwm_value = value;

}

IO_EVENT_CALLBACK(sim_stdin, events, unused) {
  int r = read(0, last_input, sizeof last_input);
  error_check(r);
  last_input[r] = 0;
  got_keypad_input(last_input[0]);

  //INFO_HEXBUFFER(last_input, r);
}

IO_TIMEOUT_CALLBACK(sim_loop) {
  loop();
}

static void print_state() {

  for (int li = 0; li < 4; li++) {
    for (int ci = 0; ci < 20; ci++) {
        if (!isprint(lcd_buffer[li][ci])) {
          if (lcd_buffer[li][ci]) {
            WARN("Non printing character in LCD buffer...");
          }
          lcd_buffer[li][ci] = ' ';
        }
    }
  }

  const u64 backlight_columns_total = 30;
  u64 backlight_columns_filled = _pin_backlight_pwm_value * backlight_columns_total / backlight_level_MAX;
  char backlight_colums[backlight_columns_total + 1];
  for (int i = 0; i < backlight_columns_total; i++) {
    if (i < backlight_columns_filled) {
      backlight_colums[i] = '#';
    } else {
      backlight_colums[i] = ' ';
    }
  }
  backlight_colums[backlight_columns_total] = 0;


  ansi_clear_lines(8);
  ansi_fmt("\n");
  //            |12345678901234567890|
  ansi_fmt("    |----LCD SCREEN------|\n");
  // _pin_backlight_pwm_value
  ansi_fmt("    |%.20s"             "|  backlight: %s\n", lcd_buffer[0], backlight_colums);
  ansi_fmt("    |%.20s"             "|\n", lcd_buffer[1]);
  ansi_fmt("    |%.20s"             "|\n", lcd_buffer[2]);
  ansi_fmt("    |%.20s"             "|\n", lcd_buffer[3]);
  ansi_fmt("    |--------------------|\n");
  ansi_fmt("\n");
  ansi_cursor_move_up(8);
  ansi_flush();
}


int main () { int r;

  { // get term settings, and then update them
    static struct termios ts;
    r = tcgetattr(0, &ts);
    error_check(r);
    ts.c_lflag &= ~ICANON; // disable line buffering
    ts.c_lflag &= ~ECHO;   // disable echo
    r = tcsetattr(0, TCSANOW, &ts);
    error_check(r);
  }

  io_initialize();
  fprintf(stderr, "ASDFASDFA\n");

  int i = 0;
  io_ctl(sim_stdin_fd, 0, 0, EPOLLIN, EPOLL_CTL_ADD);
  for (;;) {

    lcd_set_line0("test %d", i++);
    lcd_set_line1("d %02x%02x", last_input[0], last_input[1]);
    lcd_set_line2("test2 %d", 4333);
    lcd_set_line3("12345678901234567890");

    print_state();
    IO_TIMER_MS_SET(sim_loop, IO_NOW_MS() + 20);
    io_process_events();
  }

}