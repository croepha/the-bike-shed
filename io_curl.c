#include <curl/curl.h>
#include <inttypes.h>
#include <errno.h>
#include "common.h"
#include "logging.h"

#include "io.h"
#include "io_curl.h"


static int timer_callback(CURLM* multi, long timeout_ms_, void* u) {
  if (timeout_ms_ < 0) {
    IO_TIMER_MS(io_curl) = 0;
  } else {
    IO_TIMER_MS(io_curl) = utc_ms_since_epoch() + timeout_ms_;
  }

  DEBUG("timeout: %ld ms, new timer: %"PRId64"\n",
      timeout_ms_, IO_TIMER_MS(io_curl));
  return 0;
}

static CURLM * multi;
static CURLSH * share;

void io_curl_timeout() {
  int running_handles;
  CURLMcode mr = curl_multi_socket_action(multi, CURL_SOCKET_TIMEOUT, 0, &running_handles);
  error_check_curlm(mr);
}

void io_curl_io_event(struct epoll_event epe) {
  int curl_ev = 0;
  if (epe.events & EPOLLERR) curl_ev |= CURL_CSELECT_ERR;
  if (epe.events & EPOLLIN ) curl_ev |= CURL_CSELECT_IN;
  if (epe.events & EPOLLOUT) curl_ev |= CURL_CSELECT_OUT;
  int running_handles;
  io_EPData data = {.data = epe.data};
  CURLMcode mr = curl_multi_socket_action(multi, data.my_data.id, curl_ev, &running_handles);
  error_check_curlm(mr);
}

static int socket_callback(CURL* easy, curl_socket_t fd, int action, void* u, void* s) {

    io_EPData data = {.my_data = {.id = fd, .event_type = _io_socket_type_io_curl}};
    struct epoll_event epe = {.data = data.data};

    switch (action) { SWITCH_DEFAULT_IS_UNEXPECTED;
      case CURL_POLL_REMOVE: break;
      case CURL_POLL_IN   : { epe.events=EPOLLIN           ; } break;
      case CURL_POLL_OUT  : { epe.events=EPOLLOUT          ; } break;
      case CURL_POLL_INOUT: { epe.events=EPOLLIN & EPOLLOUT; } break;
    };

    if (action == CURL_POLL_REMOVE) {
        int r1 = epoll_ctl(io_epoll_fd, EPOLL_CTL_DEL, fd, &epe);
        error_check(r1);
    } else if(!s) {
      CURLMcode mr = curl_multi_assign(multi, fd, (void*)1);
      error_check_curlm(mr);
      int r1 = epoll_ctl(io_epoll_fd, EPOLL_CTL_ADD, fd, &epe);
      error_check(r1);
    } else {
      int r1 = epoll_ctl(io_epoll_fd, EPOLL_CTL_MOD, fd, &epe);
      error_check(r1);
    }
    return 0;
}

void io_curl_abort(CURL* easy) {
      CURLMcode mr = curl_multi_remove_handle(multi, easy);
      error_check_curlm(mr);
}

void io_curl_process_events() {

  CURLMsg *curl_msg;
  int curl_msg_left = 0;
  CURLcode result; CURL* easy = 0;

  while (( curl_msg = curl_multi_info_read(multi, &curl_msg_left) )) {
    if (curl_msg->msg == CURLMSG_DONE) {
      result = curl_msg->data.result;
      easy = curl_msg->easy_handle;
      CURLMcode mr = curl_multi_remove_handle(multi, easy);
      error_check_curlm(mr);

      enum _io_curl_type * private;
      CURLcode cr = curl_easy_getinfo(easy, CURLINFO_PRIVATE, &private);
      error_check_curl(cr);
#define _(name) case _io_curl_type_ ## name: { assert(__ ## name ## _io_curl_complete); __ ## name ## _io_curl_complete(easy, result, private); } break;
      switch (*private) { _IO_CURL_TYPES
        case _io_curl_type_INVALID: case _io_curl_type_COUNT:
        SWITCH_DEFAULT_IS_UNEXPECTED;
      }
#undef _
    } else {
      ERROR("unknown\n");
    }
  }
}


CURL* io_curl_create_handle(enum _io_curl_type * private) {
  CURL *easy = curl_easy_init();
  assert(easy);
  CURLESET(PRIVATE, private);
  CURLMcode mr = curl_multi_add_handle(multi, easy);
  error_check_curlm(mr);
  CURLcode cr = curl_easy_setopt(easy, CURLOPT_SHARE, share);
  error_check_curl(cr);
  return easy;
}



void io_curl_initialize() { CURLMcode mr; CURLcode cr; CURLSHcode sr;
  curl_version_info_data data = *curl_version_info(CURLVERSION_NOW);
  assert(data.features & CURL_VERSION_ASYNCHDNS);

  cr = curl_global_init(CURL_GLOBAL_DEFAULT);
  error_check_curl(cr);

  multi = curl_multi_init();
  mr = curl_multi_setopt(multi, CURLMOPT_SOCKETFUNCTION, socket_callback);
  error_check_curlm(mr);
  mr = curl_multi_setopt(multi, CURLMOPT_TIMERFUNCTION , timer_callback);
  error_check_curlm(mr);

  share = curl_share_init();
  sr = curl_share_setopt(share, CURLSHOPT_SHARE, CURL_LOCK_DATA_SSL_SESSION);
  error_check_curlsh(sr);
  sr = curl_share_setopt(share, CURLSHOPT_SHARE, CURL_LOCK_DATA_COOKIE);
  error_check_curlsh(sr);
}

//  curl_share_cleanup(share);

