#include <sys/epoll.h>

#include <errno.h>
#include <limits.h>
#include <assert.h>
#include "common.h"
#include "logging.h"
#include "io.h"

// main loop should call back downloader_timeout() when we are past this time, ignore if zero
// Measured in miliseconds since  00:00:00 UTC on 1 January 1970.
void download_io_event(struct epoll_event epe); // Call when we get an epoll event where (epe.data.u64 & ((1ull<<32)-1) == EVENT_TYPE_DOWNLOAD
void download_timeout(); // Call on timeout, see download_timer_epoch_ms
extern u64 download_timer_epoch_ms;



void io_process_events() {
  enum {
    EP_TIMER_none,
    EP_TIMER_download,
  } running_timer;

  u64 next_timer_epoch_ms = -1;

  if (next_timer_epoch_ms > download_timer_epoch_ms) {
    next_timer_epoch_ms = download_timer_epoch_ms;
    running_timer = EP_TIMER_download;
  }

  s32 timeout_ms = -1;
  u64 now_ms = utc_ms_since_epoch();
  if (next_timer_epoch_ms < now_ms) {
    timeout_ms = 0;
  } else if (next_timer_epoch_ms > INT_MAX + now_ms) {
    timeout_ms = INT_MAX;
  } else {
    timeout_ms = next_timer_epoch_ms - now_ms;
  }

  struct epoll_event epes[16];
  int r1 = epoll_wait(epoll_fd, epes, COUNT(epes), timeout_ms);
  assert(r1 != -1 || errno == EINTR);

  if (!r1) {
    download_timeout();
  } else {
    for (int i = 0; i < r1; i++) {
      struct epoll_event epe = epes[i];
      io_EPData data = {.data = epe.data};
      if (data.my_data.event_type == EVENT_TYPE_DOWNLOAD) {
        download_io_event(epe);
      }
    }
  }
}





