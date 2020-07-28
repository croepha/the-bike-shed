#include <assert.h>
#include <inttypes.h>

#include "common.h"
#include "logging.h"

#include "io.h"
#include "io_curl.h"

u64 download_timer_epoch_ms;

static int timer_callback(CURLM* multi, long timeout_ms_, void* u) {
  if (timeout_ms_ < 0) {
    download_timer_epoch_ms = 0;
  } else {
    download_timer_epoch_ms = utc_ms_since_epoch() + timeout_ms_;
  }

  DEBUG("timeout: %ld ms, new timer: %"PRId64"\n",
      timeout_ms_, download_timer_epoch_ms);
  return 0;
}

static CURLM * multi;
static CURLSH * share;

void download_timeout() {
  int running_handles;
  curl_multi_socket_action(multi, CURL_SOCKET_TIMEOUT, 0, &running_handles);
}

void download_io_event(struct epoll_event epe) {
  int curl_ev = 0;
  if (epe.events & EPOLLERR)
    curl_ev |= CURL_CSELECT_ERR;
  if (epe.events & EPOLLIN)
    curl_ev |= CURL_CSELECT_IN;
  if (epe.events & EPOLLOUT)
    curl_ev |= CURL_CSELECT_OUT;
  int running_handles;
  io_EPData data = {.data = epe.data};
  curl_multi_socket_action(multi, data.my_data.id, curl_ev, &running_handles);
}

static int socket_callback(CURL* easy, curl_socket_t fd, int action, void* u, void* s) {

    io_EPData data = {.my_data = {.id = fd, .event_type = EVENT_TYPE_DOWNLOAD}};
    struct epoll_event epe = {.data = data.data};

    switch (action) {
      case CURL_POLL_REMOVE: break;
      case CURL_POLL_IN : { epe.events=EPOLLIN ; } break;
      case CURL_POLL_OUT: { epe.events=EPOLLOUT; } break;
      case CURL_POLL_INOUT: { epe.events=EPOLLIN & EPOLLOUT; } break;
      default: assert(0);
    };

    if (action == CURL_POLL_REMOVE) {
        int r1 = epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, &epe);
        assert(r1==0);
    } else if(!s) {
      CURLMcode ret = curl_multi_assign(multi, fd, (void*)1);
      assert(ret == CURLM_OK);
      int r1 = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &epe);
      assert(r1==0);
    } else {
      int r1 = epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &epe);
      assert(r1==0);
    }
    return 0;
}

u8 io_curl_completed(CURL**easy, CURLcode*result, void*private) {
  CURLMsg *curl_msg;
  int curl_msg_left = 0;
  if (*easy) {
    curl_multi_remove_handle(multi, *easy);
  }
  while (( curl_msg = curl_multi_info_read(multi, &curl_msg_left) )) {
    if (curl_msg->msg == CURLMSG_DONE) {
      *result = curl_msg->data.result;
      *easy = curl_msg->easy_handle;
      curl_easy_getinfo(*easy, CURLINFO_PRIVATE, private);
      return 1;
    } else {
      DEBUG("unknown\n");
    }
  }
  return 0;
}

void io_curl_initialize() {
  curl_version_info_data data = *curl_version_info(CURLVERSION_NOW);
  assert(data.features & CURL_VERSION_ASYNCHDNS);

  curl_global_init(CURL_GLOBAL_DEFAULT);

  multi = curl_multi_init();
  curl_multi_setopt(multi, CURLMOPT_SOCKETFUNCTION, socket_callback);
  curl_multi_setopt(multi, CURLMOPT_TIMERFUNCTION , timer_callback);

  share = curl_share_init();
  curl_share_setopt(share, CURLSHOPT_SHARE, CURL_LOCK_DATA_SSL_SESSION);
  curl_share_setopt(share, CURLSHOPT_SHARE, CURL_LOCK_DATA_COOKIE);
}

//  curl_share_cleanup(share);


CURL* io_curl_create_handle() {
  CURL* ret = curl_easy_init();
  assert(ret);
  curl_multi_add_handle(multi, ret);
  curl_easy_setopt(ret, CURLOPT_SHARE, share);
  return ret;
}

