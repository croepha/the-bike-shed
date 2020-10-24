
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "logging.h"
#include "config.h"



u64 now_ms() { return real_now_ms(); }

void config_parse_line(char *line, int line_number) {
  usz len = strlen(line);
  INFO_BUFFER(line, len, "line: len:%zu data:", len);
}


int main() { int r;
  setlinebuf(stderr);
  r = alarm(1); error_check(r);

}


