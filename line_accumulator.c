
#include <string.h>

#include "logging.h"
#include "line_accumulator.h"

void line_accumulator(struct line_accumulator_Data * leftover, char* data, usz data_len, void(*line_handler)(char*)) {
  for (;;) {
    char* nl = memchr(data, '\n', data_len);
    if (nl) {
      *nl = 0;
      nl++;
      if (nl - data >
          line_accumulator_Data_SIZE - leftover->used) {
        ERROR("Line too long, throwing it out");
        data_len -= line_accumulator_Data_SIZE - leftover->used;
        data = nl;
        leftover->used = 0;
      } else {
        memcpy(leftover->data + leftover->used, data,
               nl - data);
        line_handler(leftover->data);
        data_len -= nl - data;
        data += nl - data;
        leftover->used = 0;
      }
    } else {
      if (leftover->used + data_len <
          line_accumulator_Data_SIZE) {
        memcpy(leftover->data + leftover->used, data,
               data_len);
        leftover->used += data_len;
      } else {
        ERROR("Couldn't find end of line, and out of space, throw out continued first line in next data set");
        leftover->used = line_accumulator_Data_SIZE;
      }
      break;
    }
  }
}
