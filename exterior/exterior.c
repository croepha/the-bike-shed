#include <stdio.h>
#include <string.h>

#include "exterior.h"
#include "state_machine.h"
#include "log_lite.h"

#define line_accumulator_Data_SIZE 512
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

enum STATE current_state = MAIN_MENU;
enum USER user = GENERAL_DOOR;
struct internal_state shed_data;
char output[4][20];


static void got_interior_line(char*line) {
  if (strncmp(line, "TEXT_SHOW1 ", 11) == 0) {
    sprintf(shed_data.rpi_output[0], "%.20s", line+11);
    backlight_level = backlight_level_MAX;
    till_autosleep_ms = 5000;
  } else if (strncmp(line, "TEXT_SHOW2 ", 11) == 0) {
    sprintf(shed_data.rpi_output[1], "%.20s", line+11);
    backlight_level = backlight_level_MAX;
    till_autosleep_ms = 5000;
  } else if (strncmp(line, "SLEEP", 5) == 0) {
    till_autosleep_ms = 0;
  } else {
    INFO("got_interior_line unkown %s\n", line);
  }
}

// Sets the internal data to null values. (NOTE: sometimes individual values are replaced in code)
static void reset_input() {
    shed_data.key = ' ';
    memset(shed_data.pin_so_far, '\0', sizeof shed_data.pin_so_far);
    memset(shed_data.option_so_far, '\0', sizeof shed_data.option_so_far);
}


void serial_got_char(char data) {
  line_accumulator(&interior_line, &data, 1, got_interior_line);
}


void got_keypad_input(char key) {
  till_autosleep_ms = 5000;
  backlight_level = backlight_level_MAX;
  // We can use strlen because the array is reset to '\0's.
  size_t entered_option_used = strlen(shed_data.option_so_far);
  size_t entered_pin_used = strlen(shed_data.pin_so_far);
  u8 entering_option = shed_data.key == '#';

  if (key == '*') {
    reset_input();
    shed_data.key = '*';
  } else if (entering_option) {
    if (key == '#') {
      reset_input();
      shed_data.key = '#';
    } else {
      shed_data.option_so_far[entered_option_used++] = key;
      if (entered_option_used >= 3) {
        // NOTE: I'm doing this to change the state machine in what I think is the cleanest way. 
        // Otherwise this conditional is duplicated.
        shed_data.key = '*';
      }
    }
  } else {
    if (key == '#') {
      shed_data.key = '#';
      if (shed_data.option_so_far[0]) {
        memset(shed_data.option_so_far, 0, sizeof shed_data.option_so_far);
        entered_option_used = 0;
      }
    } else {
      if (entered_pin_used < 10) {
        shed_data.pin_so_far[entered_pin_used++] = key;
      }
    }
  }
}


void exterior_setup() {
    serial_printf("INFO Started Up");
    serial_printf("EXTERIOR_RESTART\n");
}

void exterior_loop() {

  unsigned long uptime_delta_ms_old = last_uptime_ms;
  last_uptime_ms = now_ms();
  long uptime_delta_ms = last_uptime_ms - uptime_delta_ms_old;
  if (uptime_delta_ms > 10000) { uptime_delta_ms = 10; } // Overflow or reset...

  if (till_autosleep_ms > 0) {
    till_autosleep_ms -= uptime_delta_ms;
    if (till_autosleep_ms <= 0) {
      till_autosleep_ms = 0;
    }
  }


  if (till_autosleep_ms <= 0 && backlight_level > 0) {
    reset_input();
    shed_data.reset = 1;
    unsigned long delta_level = (backlight_level_MAX / 1000) * uptime_delta_ms;
    if (delta_level > backlight_level) {
      backlight_level = 0;
    } else {
      backlight_level -= delta_level;
    }
  }

  /// fill in shed data
  // shed_data.rfid
  // shed_data.rpi_output
  user = determine_user(current_state, user, shed_data);
  current_state = determine_next_state(current_state, user, shed_data);
  pin_backlight_pwm_set(backlight_level);

  // Output scan data to RPi.
  if (current_state == RFID_BADGE) {
    char rfid_id[1024];
    s16 rfid_id_len = rfid_id_scan(rfid_id, sizeof rfid_id);
    if (rfid_id_len) {
      char rfid_str[21];
      for (int i = 0; i< rfid_id_len; i ++ ) {
        sprintf(rfid_str + i*2, "%02x", rfid_id[i]);
      }

      INFO("RFID %s\n", rfid_str);

      serial_printf("SCAN_START\n");
<<<<<<< HEAD
      if (strlen(shed_data.option_so_far) > 0) {
          serial_printf("OPTION %s\n", shed_data.option_so_far);
      }
      serial_printf("PIN %s\n", shed_data.pin_so_far);
      serial_printf("RFID %s\n", rfid_str);
      serial_printf("SCAN_FINISHED\n");
      shed_data.rfid_set = 1;
    }
  } else {
    shed_data.rfid_set = 0;
  }

  // Set output string.
  display(current_state, user, shed_data, output);

  // Set LCD output.
  lcd_set_line0("%s", output[0]);
  lcd_set_line1("%s", output[1]);
  lcd_set_line2("%s", output[2]);
  lcd_set_line3("%s", output[3]);

  shed_data.reset = 0;
  //fprintf(stderr, "sim loop\n");
=======
      if (entered_option_used) {
        serial_printf("OPTION %s\n", entered_option);
      }
      serial_printf("PIN %s\n", entered_pin);
      serial_printf("RFID %s\n", rfid_str);
      serial_printf("SCAN_FINISHED\n");
      reset_input();
    }
  }

>>>>>>> 527d9c7095ed13069e4020b406d22b85edc8a054
}
