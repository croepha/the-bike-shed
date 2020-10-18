
#include <string.h>

#include "logging.h"
#include "line_accumulator.h"

void line_accumulator(struct line_accumulator_Data * leftover, char* data, usz data_len, void(*line_handler)(char*)) {
  u8 line_too_long = 0;
  for (;;) {
    if (!data_len) break;
    char c = *data++;
    data_len--;
    if (c == '\n') {
      if (line_too_long) {
        ERROR("Line too long, throwing it out");
        line_too_long = 0;
      } else {
        line_handler(leftover->data);
        leftover->data[leftover->used] = 0;
        leftover->used = 0;
      }
    } else if (!line_too_long) {
      if (leftover->used >= line_accumulator_Data_SIZE - 1) {
        line_too_long = 1;
        ERROR("Couldn't find end of line, and out of space, throw out continued first line in next data set");
      } else {
        leftover->data[leftover->used++] = c;
      }
    }
  }
}
