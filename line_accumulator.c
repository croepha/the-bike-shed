
#include <string.h>

#include "logging.h"
#include "line_accumulator.h"

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
