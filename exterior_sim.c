
/*

This is a tool that will run the exterior code on a dev machine

Here are some shell commands to run the simulator:

# Set some variables
exterior_serial_dev="/build/sim_exterior.pts"
interior_serial_dev="/build/sim_interior.pts"
shed=/build/shed3.dbg.exec
config_location=/workspaces/the-bike-shed/sim-shed-config
serial_log="/build/sim_serial.log"
shed_log="/build/sim_shed.log"

# Tail all relavant logs in the background
tail -f /build/sim_*.log &

# Startup the simulated serial port, using socat
socat -d -d -v PTY,link="$exterior_serial_dev",rawer,echo=0 PTY,link="$interior_serial_dev",rawer,echo=0 >> $serial_log 2>&1  &

# Start a SHED instance on your dev machine:
SHED_TRACE=1 $shed $config_location &> $shed_log & shed_pid=$

# run the actual simulator:
/build/exterior_sim.dbg.exec



When the simulator is running, you can use the number keys as your keypad
To simulate scanning an RFID card, press r, and then select a number to pick
a simulated RFID card

The simulated RFID cards are pulled from the `/build/quick_hashes.text` file, it's content is like this:

2: 000000000000003
4: 000000000000001
6: f00ba4
9: 000000000000002


*/

#define LOG_DEBUG


#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/epoll.h>
#include "common.h"
#include "logging.h"
#include "io.h"
#include "serial.h"
#include <inttypes.h>



#include <termios.h>
#include <stdio.h>

u64 now_ms() { return real_now_ms(); }
IO_TIMEOUT_CALLBACK(idle) {}

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

#include "exterior/exterior.h"

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

// --------------


s32 serial_fd;
u32 _pin_backlight_pwm_value;
char last_input[20];
enum ui_mode {
  UI_MODE_NORMAL,
  UI_MODE_RFID,
} current_ui_mode;
char quick_hashes[10][128];
s32 quick_hashes_recently_selected; // idx + 1, zero indicates no selection

static const char quick_hashes_path[] = "/build/quick_hashes.text";

static int base16_to_int(char c) {
    if ('A' <= c && c <= 'F') {
        return 10 + c - 'A';
    } else if ('a' <= c && c <= 'f') {
        return 10 + c - 'a';
    } else if ('0' <= c && c <= '9') {
        return c - '0';
    } else {
        ERROR("bad char: %c", c);
        return 0;
    }
}

static s32 buf_from_hex(void* buf_, usz buf_len, char * hex) {
    u8 * buf = buf_;
    for (int i = 0; i < buf_len; i++) {
        if (!hex[2*i+0] || !hex[2*i+1]) {
            return i;
        }
        buf[i] = (base16_to_int(hex[2*i+0]) << 4)
          | (base16_to_int(hex[2*i+1]) << 0);
    }
    return 0;
}

s32 rfid_id_scan(char* buf, s32 buf_len) {
  memset(buf, 0, buf_len);
  if (quick_hashes_recently_selected) {
    s32 entry = quick_hashes_recently_selected - 1;
    s32 len = buf_from_hex(buf, buf_len, quick_hashes[entry]);
    quick_hashes_recently_selected = 0;
    return len;
  }
  return 0;
}

void pin_backlight_pwm_set(u32 value) {
  _pin_backlight_pwm_value = value;
}

void _log_lite(const char* severity, const char*file, const char*func, int line, char* fmt, ...)
    __attribute__((__format__ (__printf__, 5, 6)));
void _log_lite(const char* severity, const char*file, const char*func, int line, char* fmt, ...) {
  char log_buffer[1024];
  va_list va; va_start(va, fmt);
  int r = snprintf(log_buffer, sizeof log_buffer, fmt, va);
  va_end(va);
  error_check(r);
  assert(r < sizeof log_buffer);
  u64 now_ms_ = now_ms();
  fprintf(stderr, "%06"PRIx64".%03"PRIu64": %s: %s ", now_ms_ / 1000, now_ms_ % 1000, severity, log_buffer);
}


IO_EVENT_CALLBACK(sim_stdin, events, unused) {
  int r = read(0, last_input, sizeof last_input);
  error_check(r);
  last_input[r] = 0;
  char c = last_input[0];

  switch (current_ui_mode) {
    case UI_MODE_NORMAL: {
      if (c >= '0' && c <= '9' ) {
        got_keypad_input(c);
      } else if (c == '-') {
        got_keypad_input('#');
      } else if (c == '*') {
        got_keypad_input('*');
      } else if (c == 'r') {
        current_ui_mode = UI_MODE_RFID;
        IO_TIMER_MS_SET(sim_loop, -1);
      }
    } break;
    case UI_MODE_RFID: {
      if (c >= '0' && c <= '9' ) {
        s32 entry = c - '0';
        INFO("RFID Scan: %s", quick_hashes[entry]);
        quick_hashes_recently_selected = entry + 1;
        //got_keypad_input(c);
        current_ui_mode = UI_MODE_NORMAL;
        ansi_clear_lines(12);
      }
    } break;
  }

  //INFO_HEXBUFFER(last_input, r);
}

IO_TIMEOUT_CALLBACK(sim_loop) {
  exterior_loop();
}

IO_EVENT_CALLBACK(serial, events, ignored_id) {

  u8 data = -1;
  int r = read(serial_fd, &data, 1);
  error_check(r);
  assert(r == 1);

  serial_got_char(data);

}

void serial_printf(const char * fmt, ...) {
  va_list va;
  va_start(va, fmt);
  vdprintf(serial_fd, fmt, va);
  dprintf(serial_fd, "\n");
  va_end(va);
}




static void print_state() {

  switch (current_ui_mode) {
    case UI_MODE_NORMAL: {
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
      ansi_fmt("    |%.20s"             "|  backlight: %s\n", lcd_buffer[0], backlight_colums);
      ansi_fmt("    |%.20s"             "|\n", lcd_buffer[1]);
      ansi_fmt("    |%.20s"             "|\n", lcd_buffer[2]);
      ansi_fmt("    |%.20s"             "|\n", lcd_buffer[3]);
      ansi_fmt("    |--------------------|\n");
      ansi_fmt("\n");
      ansi_cursor_move_up(8);
    } break;
    case UI_MODE_RFID: {
      FILE* f = fopen(quick_hashes_path, "r");
      if (f) {
        while (!feof(f)) {
          char buf[1024] = "";
          int entry;
          int r = fscanf(f, "%1000d: %s\n", &entry, buf);
          if (r != 2) {
            fscanf(f, "%*[^\n]\n"); // Read through to the end of the line
          }
          //DEBUG("scan: %d, tell:%ld, entry:%d buf:%s", r, ftell(f), entry, buf);
          if (entry < 10) {
            snprintf(quick_hashes[entry], sizeof quick_hashes[entry], "%s", buf);
          }
        }
        fclose(f);
        //DEBUG("done");
      }

      ansi_clear_lines(12);
      ansi_fmt("\n");
      for (int i = 0; i < 10; i++) {
        ansi_fmt("    %d: %s\n", i, quick_hashes[i]);
      }
      ansi_fmt("\n");
      ansi_cursor_move_up(12);
    } break;
  }

  ansi_flush();

}

// static void debug_describe_fd(s32 fd) {
//   char path_buf[128] = {};
//   snprintf(path_buf, sizeof path_buf - 1, "/proc/self/fd/%d", fd);
//   char rl_buf[1024] = {};
//   readlink(path_buf, rl_buf, sizeof(rl_buf) -1 );
//   DEBUG("FD: %d: %s", fd, rl_buf);
// }

int main () { int r;

  io_initialize();
  fprintf(stderr, "ASDFASDFA\n");

  { // get term settings, and then update them
    static struct termios ts;
    r = tcgetattr(0, &ts);
    error_check(r);
    ts.c_lflag &= ~ICANON; // disable line buffering
    ts.c_lflag &= ~ECHO;   // disable echo
    r = tcsetattr(0, TCSANOW, &ts);
    error_check(r);
  }

  serial_fd = open("/build/sim_exterior.pts", O_RDWR);

  {
    char buf[1024];
    snprintf(buf, sizeof buf, "lsof -p %d", getpid());
    system(buf);
  }
  DEBUG("FD: %d", serial_fd);

  error_check(serial_fd);
  io_ADD_R(serial_fd);

  io_ctl(sim_stdin_fd, 0, 0, EPOLLIN, EPOLL_CTL_ADD);

  exterior_setup();

  for (;;) {

    print_state();
    IO_TIMER_MS_SET(sim_loop, IO_NOW_MS() + 20);
    io_process_events();
  }

}