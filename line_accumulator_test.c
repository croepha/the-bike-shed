#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>


#include "common.h"
#include "logging.h"
#include "line_accumulator.h"



static void __line_handler(char* line) {
    INFO_BUFFER(line, strlen(line), "line:");
}

struct line_accumulator_Data leftover_d = {};


// It is likely that size is large, containing many lines
static size_t config_download_write_callback(char *data, size_t size, size_t nmemb, void *userdata) {
  size = size * nmemb;
  line_accumulator(&leftover_d, data, size, __line_handler);
  return size;
}

static const usz config_download_leftover_SIZE = line_accumulator_Data_SIZE;
usz  const  data_LEN = config_download_leftover_SIZE * 50;
char  data[data_LEN];
usz   data_consumed;

static void data_send(usz len) {
  assert(data_consumed + len < data_LEN);
  INFO_BUFFER( data, len, "len:%zu data:", len);
  config_download_write_callback(data + data_consumed, 1, len, 0);
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
  data_send(config_download_leftover_SIZE - 1);
  data_send(config_download_leftover_SIZE + 0);
  data_send(config_download_leftover_SIZE + 1);
  data_send(0);
  data_send(1);

  data_send(config_download_leftover_SIZE - 1);
  data_send(config_download_leftover_SIZE + 0);
  data_send(config_download_leftover_SIZE + 1);




}