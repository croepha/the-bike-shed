
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "logging.h"
#include "config.h"


void config_parse_line(char *line, u8 print_diagnostics, int line_number) {
  usz len = strlen(line);
  INFO_BUFFER(line, len, "line: len:%zu data:", len);
}

usz  const  data_LEN = config_download_leftover_SIZE * 50;
char  data[data_LEN];
usz   data_consumed;

static void data_send(usz len) {
  assert(data_consumed + len < data_LEN);
  INFO_BUFFER( data, len, "len:%zu data:", len);
  config_download_write_callback(data + data_consumed, 1, len, 0);
  data_consumed += len;
}

int main() { int r;
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


