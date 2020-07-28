#include <string.h>

#include "common.h"
#include "logging.h"


void handle_line(char* line);


const usz leftover_SIZE = 16;
char leftover[leftover_SIZE];
usz  leftover_used;

// It is likely that size is large, containing many lines
void handle_data(char* data, usz size) {
  if (leftover_used) { // Rare case, resuming with some leftover data
    char* endp = memchr(data, '\n', size);
    usz next = -1;
    if (endp) {
      *endp = 0;
      next = (endp-data)+1;
    }

    if (leftover_used + next > leftover_SIZE) {
      ERROR("Line too long, throwing it out");
      if (endp) {
        leftover_used = 0;
        data += next;
        size -= next;
      } else {
        leftover_used = leftover_SIZE;
        ERROR("Couldn't find end of line, and out of space, throw out continued first line in next data set");
        return;
      }
    } else {
      memcpy(leftover + leftover_used, data, next);
      handle_line(leftover);
      data += next;
      size -= next;
    }
  }

  for(;;) {
    char* endp = memchr(data, '\n', size);
    usz next = -1;
    if (endp) {
      *endp = 0;
      next = (endp-data)+1;
      handle_line(data);
      data += next;
      size -= next;
    } else {
      if (size >= leftover_SIZE) {
        leftover_used = leftover_SIZE;
        ERROR("Leftover data would oveflow, size:%ld, throwing out the data", size);
        return;
      } else {
        memcpy(leftover, data, size);
        leftover_used = size;
        DEBUG("leftover size:%ld\n", size);
        return;
      }
    }
  }
}

#include <assert.h>

char test_buf[2048];
usz  test_buf_used;
char line_chars[128];
usz  line_lens[128];
usz  line_count;
usz  lines_done;

void handle_line(char* line) {
  usz len = strlen(line);
  DEBUG("%c, %ld", *line, len);
  if (len) {
    assert(line_chars[0] == *line);
  }
  assert(line_lens[0] == len);
  memmove(line_chars, line_chars + 1, line_count - 1);
  memmove(line_lens, line_lens + 1, line_count - 1);
  line_count--;
  lines_done++;
}

void add_test_line(usz len) {
  static char c = 'a';
  memset(test_buf + test_buf_used, c, len);
  test_buf[test_buf_used + len] = '\n';
  line_chars[line_count] = c;
  line_lens [line_count] = len;
  test_buf_used += len + 1;
  line_count++;
  c++;
  if (c > 'z') {
    c = 'a';
  }
}

int main() {
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



