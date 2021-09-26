
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "logging.h"
#include "config.h"
#include "config_download.h"
#include "io.h"

void __debug_config_download_complete_hook(void) {}
void config_download_finished(struct config_download_Ctx *c, u8 success) {}
IO_TIMEOUT_CALLBACK(idle) {}


u64 now_ms() { return real_now_ms(); }

void config_parse_line(char *line, int line_number) {
  usz len = strlen(line);
  INFO_BUFFER(line, len, "line: len:%zu data:", len);
}


int main() { int r;
  setlinebuf(stderr);
  r = alarm(1); error_check(r);

}


