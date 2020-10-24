#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>


#include "common.h"
#include "logging.h"
#include "line_accumulator.h"


static void line_handler(char* line) {
  usz len = strlen(line);
  INFO_BUFFER(line, len, "line: len:%zu data:", len);
}

struct line_accumulator_Data leftover_d = {};



usz  const  data_LEN = line_accumulator_Data_SIZE * 50;
char  data[data_LEN];
usz   data_consumed;

static void data_send(usz len) {
  assert(data_consumed + len < data_LEN);
  INFO_BUFFER( data, len, "len:%zu data:", len);
  line_accumulator(&leftover_d, data + data_consumed, len, line_handler);
  data_consumed += len;
}


int main( ) { int r;

  setlinebuf(stderr);
  r = alarm(1); error_check(r);

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
  data_send(line_accumulator_Data_SIZE - 1);
  data_send(line_accumulator_Data_SIZE + 0);
  data_send(line_accumulator_Data_SIZE + 1);
  data_send(0);
  data_send(1);

  data_send(line_accumulator_Data_SIZE - 1);
  data_send(line_accumulator_Data_SIZE + 0);
  data_send(line_accumulator_Data_SIZE + 1);




}