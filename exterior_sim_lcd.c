


#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "logging.h"


void _lcd_set_linenf(u8 line_i, char const *fmt, ...);
#define lcd_set_line0(fmt, ...) _lcd_set_linenf(0, fmt, #__VA_ARGS__);
#define lcd_set_line1(fmt, ...) _lcd_set_linenf(1, fmt, #__VA_ARGS__);
#define lcd_set_line2(fmt, ...) _lcd_set_linenf(2, fmt, #__VA_ARGS__);
#define lcd_set_line3(fmt, ...) _lcd_set_linenf(3, fmt, #__VA_ARGS__);


static char lcd_buffer[4][20]; // one more than visible for null terminator
void _lcd_set_linenf(u8 line_i, char const *fmt, ...) {
  va_list va;
  va_start(va, fmt);

  assert(line_i < 4);

  char buf[21];
  memset(buf, ' ', sizeof buf);
  int needed_len = vsnprintf(buf, sizeof buf, fmt, va);

  if (needed_len >= sizeof lcd_buffer[line_i]) {
    WARN("lcd line too long");
  }

  memcpy(lcd_buffer[line_i], buf, 20);


}


static void print_state() {
  printf("|----LCD SCREEN------|\n");
  for (int li = 0; li < 4; li++) {
    for (int ci = 0; ci < 20; ci++) {
        if (!isprint(lcd_buffer[li][ci])) {
            lcd_buffer[li][ci] = ' ';
        }
    }
      printf("|%.20s|\n", lcd_buffer[li]);
  }
  printf("|--------------------|\n");

}

int main () {
    printf("ASDFASDFA\n");

    lcd_set_line0("test %d", 43)
    lcd_set_line2("test2 %d", 4333)
    print_state();

}