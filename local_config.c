


extern char* local_config_path;

#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "common.h"
#include "logging.h"

static FILE* config_file;

void local_config_init() { int r;
  config_file = fopen(local_config_path, "r+");
  assert(config_file);


  u8 handling_error = 0;
  for (;;) {
    char _[12];
    char* has_line = fgets(_, sizeof _, config_file);
    if (!has_line) break;
    if (!memchr(_, '\n', sizeof _)) {
      handling_error = 1;
      ERROR("Missing newline from config, throwing out until newline");
      continue;
    }
    if (handling_error) {
      handling_error = 0;
      ERROR("continued throwing out");
      continue;
    }



    printf("READ_LINE: %s\n", _);
  }
  r = fseek(config_file, SEEK_SET, 0);
  assert(r!=-1);
  fprintf(config_file, "aaaaaaaaaa\n");
  fprintf(config_file, "aaaaaaaaaaa\n");
  fprintf(config_file, "aaaaaaaaaaaa\n");
  fprintf(config_file, "aaaaaaaaaaaaa\n");
  fprintf(config_file, "aaaaaaaaaaaaaa\n");
  fflush(config_file);

}


char* local_config_path = "/build/local_config_test";

int main () {
  FILE* f = fopen(local_config_path, "w");
  fprintf(f, "line1\n");
  fprintf(f, "line2\n");
  fprintf(f, "line3\n");
  fprintf(f, "line4\n");
  fclose(f);


  local_config_init();
}