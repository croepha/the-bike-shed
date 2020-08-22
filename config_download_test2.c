
#include <errno.h>
#include <stdio.h>
#include "config_download.c"
#include "logging.h"

void handle_data(char* data, usz size);

void handle_line(char* line) {
  INFO("line: %s", line);
}

usz  const  data_LEN = leftover_SIZE * 50;
char  data[data_LEN];
usz   data_consumed;

void data_send(usz len) {
  INFO("Handling data len:%ld", len);
  assert(data_consumed + len < data_LEN);
  handle_data(data, len);
  data_consumed += len;
}

int main() { int r;

  {
    usz data_filled = 0;
    for (int i = 0; ; i++) {
      usz size_left = data_LEN - data_filled;
      r = snprintf(data + data_filled, size_left, "Some Line %d\n", i);
      error_check(r);
      if (r >= size_left) { break; }
      data_filled +=  r;
    }
  }
  data_send(0);
  data_send(1);
  data_send(leftover_SIZE - 1);
  data_send(leftover_SIZE + 0);
  data_send(leftover_SIZE + 1);
  data_send(0);
  data_send(1);
  data_send(leftover_SIZE - 1);
  data_send(leftover_SIZE + 0);
  data_send(leftover_SIZE + 1);

}


