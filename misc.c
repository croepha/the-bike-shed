
#include <sys/time.h>
#include "common.h"

long long now_ms(void) {
  struct timeval tv;
  gettimeofday(&tv, 0);
  return (long long)(tv.tv_sec) * 1000 + (long long)(tv.tv_usec) / 1000;
}

