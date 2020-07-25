
#include <sys/time.h>
long long utc_ms_since_epoch() {
  struct timeval tv;
  gettimeofday(&tv, 0);
  return (long long)(tv.tv_sec) * 1000 + (long long)(tv.tv_usec) / 1000;
}

