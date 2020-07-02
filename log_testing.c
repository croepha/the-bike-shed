
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>



int  _log_context_push(char* fmt, ...) __attribute__((__format__ (__printf__, 1, 2)));
void _log_context_pop(int*old_len);

#define TOKENPASTE2(a, b) a ## b
#define TOKENPASTE(a, b) TOKENPASTE2(a,b)
#define LOGCTX(fmt, ...) __attribute__((unused)) __attribute__((cleanup(_log_context_pop))) \
  int TOKENPASTE(ol, __COUNTER__) = _log_context_push(fmt,  ##__VA_ARGS__);


char buffer[64];
int  len;

int _log_context_push(char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  int r = vsnprintf(buffer+len, sizeof buffer - len, fmt, args);
  va_end(args);
  assert(r>=0);

  printf("DEBUG: %d %d\n", len, r);

  int old_len = len;
  len += r;
  if (len > sizeof buffer - 1) {
    len = sizeof buffer - 1;
    printf("buffer out of space\n");
  }
  return old_len;
}

void _log_context_pop(int*old_len) {
  len = *old_len;
  buffer[len] = 0;
}



void recurs(int depth) {
  printf("\n");
  LOGCTX("recurs:depth:%d", depth);
  {
    LOGCTX("2:%d", depth);
    printf("buffer2:%s\n", buffer);
  }

  printf("buffer:%s\n", buffer);
  printf("recurs:depth:%d\n", depth);
  if (depth < 10) {
    recurs(depth + 1);
  }
  printf("buffer:%s\n", buffer);
}

int main () {

  recurs(1);

  printf("ASDFASD\n");
}