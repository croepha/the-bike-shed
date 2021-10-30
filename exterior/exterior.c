
#include "../common.h"
#include "../logging.h"
#include "exterior.h"
#include <stdio.h>
#include <string.h>



static const u32 line_accumulator_Data_SIZE = 512;
struct line_accumulator_Data {
  usz used;
  char data[line_accumulator_Data_SIZE];
};

void line_accumulator(struct line_accumulator_Data * leftover, char* data, usz data_len, void(*line_handler)(char*));
void line_accumulator(struct line_accumulator_Data * leftover, char* data, usz data_len, void(*line_handler)(char*)) {
  for (;;) {
    if (!data_len) break;
    char c = *data++;
    data_len--;
    if (c == '\n') {
      if (leftover->used == line_accumulator_Data_SIZE) {
        ERROR("Line too long, throwing it out");
        leftover->used = 0;
      } else {
        leftover->data[leftover->used] = 0;
        line_handler(leftover->data);
        leftover->used = 0;
      }
    } else if (leftover->used != line_accumulator_Data_SIZE) {
      if (leftover->used >= line_accumulator_Data_SIZE - 1) {
        ERROR("Couldn't find end of line, and out of space, throw out continued first line in next data set");
        leftover->used = line_accumulator_Data_SIZE;
      } else {
        leftover->data[leftover->used++] = c;
      }
    }
  }
}



long till_autosleep_ms = 0;
unsigned int backlight_level = 0;
unsigned long last_uptime_ms = 0;
struct line_accumulator_Data interior_line;


static void got_interior_line(char*line) {
  if (strncmp(line, "TEXT_SHOW1 ", 11) == 0) {
    lcd_set_line2("%20s", line+11);
    backlight_level = backlight_level_MAX;
    till_autosleep_ms = 5000;
  } else if (strncmp(line, "TEXT_SHOW2 ", 11) == 0) {
    lcd_set_line3("%20s", line+11);
    backlight_level = backlight_level_MAX;
    till_autosleep_ms = 5000;
  } else if (strncmp(line, "SLEEP", 5) == 0) {
    till_autosleep_ms = 0;
  } else {
    INFO("got_interior_line unkown %s\n", line);
  }
}


u8 entering_option = 0;
char entered_pin[11];
int  entered_pin_used = 0;
char entered_option[4];
int  entered_option_used = 0;

static void reset_input() {
    entering_option = 0;
    entered_pin_used = 0;
    entered_option_used = 0;
    memset(entered_pin, 0, sizeof entered_pin);
    memset(entered_option, 0, sizeof entered_option);

    lcd_set_line0("                    ");
    lcd_set_line1("                    ");

}

static void draw_input_lines() {
  if (1) {
    // Show pin
    lcd_set_line0("PIN:%12s%c", entered_pin, entering_option?' ':'<');
  } else {
    // Hide pin
    char buf[24];
    for (int i = 0; i < entered_pin_used; i++) {
      buf[i] = '*';
    }
    buf[entered_pin_used] = 0;
    lcd_set_line0("PIN:%12s%c", buf, entering_option?' ':'<');
  }
  if (entered_option_used || entering_option) {
    lcd_set_line1("OPT:%12s%c", entered_option, entering_option?'<':' ');
  } else {
    lcd_set_line1("                   ");
  }
}



void serial_got_char(char data) {
  line_accumulator(&interior_line, &data, 1, got_interior_line);
}


void got_keypad_input(char key) {
  till_autosleep_ms = 5000;
  backlight_level = backlight_level_MAX;

  if (key == '*') {
    reset_input();
  } else if (entering_option) {
    if (key == '#') {
      entering_option = 0;
    } else {
      entered_option[entered_option_used++] = key;
      if (entered_option_used >= 3) {
        entering_option = 0;
      }
    }
  } else {
    if (key == '#') {
      entering_option = 1;
      if (entered_option[0]) {
        memset(entered_option, 0, sizeof entered_option);
        entered_option_used = 0;
      }
    } else {
      if (entered_pin_used < 10) {
        entered_pin[entered_pin_used++] = key;
      }
    }
  }
  draw_input_lines();

}


void setup() {
    serial_printf("INFO Started Up");
    serial_printf("EXTERIOR_RESTART\n");
}

void loop() {

  unsigned long uptime_delta_ms_old = last_uptime_ms;
  last_uptime_ms = now_ms();
  long uptime_delta_ms = last_uptime_ms - uptime_delta_ms_old;
  if (uptime_delta_ms > 10000) { uptime_delta_ms = 10; } // Overflow or reset...

  if (till_autosleep_ms > 0) {
    till_autosleep_ms -= uptime_delta_ms;
    if (till_autosleep_ms <= 0) {
       till_autosleep_ms = 0;
       reset_input();
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

  if (entered_pin_used) {
    char rfid_id[1024];
    s16 rfid_id_len = rfid_id_scan(rfid_id, sizeof rfid_id);
    if (rfid_id_len) {
    char rfid_str[21];
    for (int i = 0; i< rfid_id_len; i ++ ) {
      sprintf(rfid_str + i*2, "%02x", rfid_id[i]);
    }

    INFO("RFID %s\n", rfid_str);

    serial_printf("SCAN_START\n");
    if (entered_option_used) {
        serial_printf("OPTION %s\n", entered_option);
    }
    serial_printf("PIN %s\n", entered_pin);
    serial_printf("RFID %s\n", rfid_str);
    serial_printf("SCAN_FINISHED\n");
    reset_input();
    }
  }



  char rfid_id[1024];
  s16 rfid_id_len = rfid_id_scan(rfid_id, sizeof rfid_id);
  if (rfid_id_len) {
    INFO_HEXBUFFER(rfid_id, rfid_id_len, "got rifd scan:");
    serial_printf("GOT_RFID\n");

  }

  //fprintf(stderr, "sim loop\n");
}
