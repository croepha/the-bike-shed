
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



  add_test_line(10);
  handle_data(test_buf, 11);
  assert(lines_done == 1);
  line_count = 0;
  lines_done = 0;
  test_buf_used = 0;

  add_test_line(10);
  add_test_line(10);
  handle_data(test_buf, 22);
  assert(lines_done == 2);
  line_count = 0;
  lines_done = 0;
  test_buf_used = 0;

  add_test_line(10);
  add_test_line(10);
  handle_data(test_buf, 20);
  handle_data(test_buf+20, 2);
  assert(lines_done == 2);
  line_count = 0;
  lines_done = 0;
  test_buf_used = 0;

  add_test_line(17);
  add_test_line(13);
  handle_data(test_buf, 18);
  handle_data(test_buf+18, 14);
  assert(lines_done == 2);
  line_count = 0;
  lines_done = 0;
  test_buf_used = 0;

  add_test_line(17);
  line_count--;
  add_test_line(13);
  handle_data(test_buf, 15);
  log_allowed_fails = 99;
  handle_data(test_buf+15, 17);
  log_allowed_fails = 0;
  assert(lines_done == 1);


  line_count = 0;
  lines_done = 0;
  test_buf_used = 0;
  for (int i = 0; i < 20; i ++) {
    add_test_line(0);
  }
  handle_data(test_buf, 20);
  assert(lines_done == 20);

  line_count = 0;
  lines_done = 0;
  test_buf_used = 0;
  for (int i = 0; i < 20; i ++) {
    add_test_line(1);
  }
  handle_data(test_buf, 40);
  assert(lines_done == 20);

  line_count = 0;
  lines_done = 0;
  test_buf_used = 0;
  for (int i = 0; i < 20; i ++) {
    add_test_line(1);
  }
  handle_data(test_buf, 40);
  assert(lines_done == 20);

}



