#define STRINGIFY2(a) #a
#define STRINGIFY(a) STRINGIFY(a)

#include <sys/types.h>
enum {
  EVENT_TYPE_DOWNLOAD

};

#include <stdlib.h>
#include <string.h>
#include "logging.h"

// internal:
#include <assert.h>
#include <errno.h>
#include <sys/epoll.h>
#include <unistd.h>

#include <curl/curl.h>


#include <inttypes.h>


#define COUNT(array) (sizeof(array)/sizeof((array)[0]))

typedef u_int8_t   u8;
typedef u_int16_t u16;
typedef u_int32_t u32;
typedef u_int64_t u64;
typedef   int8_t   s8;
typedef   int16_t s16;
typedef   int32_t s32;
typedef   int64_t s64;


extern int epoll_fd;


// Exported interface:
void download_initialize(); // Call once before doing anything else with the downloader
void download_timeout(); // Call on timeout, see download_timer_epoch_ms
void download_io_event(struct epoll_event epe); // Call when we get an epoll event where (epe.data.u64 & ((1ull<<32)-1) == EVENT_TYPE_DOWNLOAD


// main loop should call back downloader_timeout() when we are past this time, ignore if zero
u64 download_timer_epoch_ms;
// Measured in miliseconds since  00:00:00 UTC on 1 January 1970.


#include <sys/time.h>
long long utc_ms_since_epoch() {
  struct timeval tv;
  gettimeofday(&tv, 0);
  return (long long)(tv.tv_sec) * 1000 + (long long)(tv.tv_usec) / 1000;
}

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

void download_timeout(); // Call on timeout, see download_timer_epoch_ms
void download_io_event(struct epoll_event epe); // Call when we get an epoll event where (epe.data.u64 & ((1ull<<32)-1) == EVENT_TYPE_DOWNLOAD

static CURLM* multi;

typedef union {
  epoll_data_t data;
  struct {
    s32 id;
    u8 event_type;
  } my_data;
} MyEPData;

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
  MyEPData data = {.data = epe.data};
  curl_multi_socket_action(multi, data.my_data.id, curl_ev, &running_handles);
}

static int socket_callback(CURL* easy, curl_socket_t fd, int action, void* u, void* s) {

    MyEPData data = {.my_data = {.id = fd, .event_type = EVENT_TYPE_DOWNLOAD}};
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


void download_initialize() {
  curl_version_info_data data = *curl_version_info(CURLVERSION_NOW);
  assert(data.features & CURL_VERSION_ASYNCHDNS);

  curl_global_init(CURL_GLOBAL_DEFAULT);
  multi = curl_multi_init();

  curl_multi_setopt(multi, CURLMOPT_SOCKETFUNCTION, socket_callback);
  curl_multi_setopt(multi, CURLMOPT_TIMERFUNCTION , timer_callback);
}

CURL* download_create_handle() {
  CURL* ret = curl_easy_init();
  assert(ret);
  curl_multi_add_handle(multi, ret);
  return ret;
}


#include <openssl/sha.h>

typedef struct {
    SHA256_CTX sha256;
    CURL* curl;
    double downloaded;
    u64 print_time;
    int id;
    struct curl_slist* headers_list;
    char etag[32];
    u64  modified_time;
} _WriteCtx;

static size_t _write_function(void *contents, size_t size, size_t nmemb, void*userp) {
    size_t realsize = size * nmemb;
    _WriteCtx *c = (_WriteCtx*)userp;
    c->downloaded += realsize;
    u64 now = utc_ms_since_epoch();
    if (c->print_time + 500 < now) {
        c->print_time = now;
        double total_size;
        int r1 = curl_easy_getinfo(c->curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &total_size);
        assert(!r1);
        DEBUG("%d, %f ", c->id, (float)c->downloaded/(float)total_size);
    }
    // print("size", size, "nmemb", nmemb);
    SHA256_Update(&c->sha256, contents, realsize);
    return realsize;
}

int epoll_fd = -1;
CURLSH *share;


#include "/build/parse_headers.re.c"

static size_t _header_callback(char *buffer, size_t _s, size_t nitems, void *userdata) {
  _WriteCtx * c = userdata;
  buffer[nitems-2] = 0;
  DEBUG("Got header: '%s'", buffer);
  struct ParsedHeader header =  parse_header(buffer);
  switch (header.type) { default: break;
    case HEADER_TYPE_ETAG: {
      DEBUG("HEADER_TYPE_ETAG %s\n", header.value);
      size_t etag_len = strlen(header.value);
      if (etag_len < sizeof c->etag) {
        memcpy(c->etag, header.value, etag_len + 1);
      }
    } break;
    case HEADER_TYPE_LAST_MODIFIED: {
      u64 seconds = curl_getdate(header.value, 0);
      DEBUG("HEADER_TYPE_LAST_MODIFIED %s %"PRId64"\n", header.value, seconds);
      c->modified_time = seconds;
    } break;
  }
  return nitems;
}


void _dl(_WriteCtx *c, char* url, char* previous_etag, u64 previous_mod_time) {
  memset(c, 0, sizeof *c);
  SHA256_Init(&c->sha256);
  c->headers_list = NULL;
  c->curl = download_create_handle();

  if (previous_etag) {
    char tmp[256];
    int r = snprintf(tmp, sizeof tmp, "If-None-Match: \"%s\"", previous_etag);
    if (r >= sizeof tmp) {
      ERROR("etag too big");
    } else {
      c->headers_list = curl_slist_append(c->headers_list, tmp);
    }
  }

  curl_easy_setopt(c->curl, CURLOPT_HTTPHEADER, c->headers_list);
  curl_easy_setopt(c->curl, CURLOPT_HEADERFUNCTION, _header_callback);
  curl_easy_setopt(c->curl, CURLOPT_WRITEFUNCTION, _write_function);
  curl_easy_setopt(c->curl, CURLOPT_SHARE, share);
  if (previous_mod_time) {
    curl_easy_setopt(c->curl, CURLOPT_TIMECONDITION, (long)CURL_TIMECOND_IFMODSINCE);
    curl_easy_setopt(c->curl, CURLOPT_TIMEVALUE_LARGE, (curl_off_t)previous_mod_time);
  }
  //curl_easy_setopt(c->curl, CURLOPT_COOKIEFILE, "");
  curl_easy_setopt(c->curl, CURLOPT_USERAGENT, "the-bike-shed/0");
  //curl_easy_setopt(c->curl, CURLOPT_VERBOSE, 1);
  curl_easy_setopt(c->curl, CURLOPT_MAXREDIRS, 50L);
  curl_easy_setopt(c->curl, CURLOPT_PRIVATE, c);
  curl_easy_setopt(c->curl, CURLOPT_WRITEDATA, c);
  curl_easy_setopt(c->curl, CURLOPT_HEADERDATA, c);
  curl_easy_setopt(c->curl, CURLOPT_URL, url);
  curl_easy_setopt(c->curl, CURLOPT_TCP_KEEPALIVE, 1L);

}

void _dl_free(_WriteCtx *c) {
  curl_slist_free_all(c->headers_list);
  curl_easy_cleanup(c->curl);
}

u8 get_completed_curls(CURL**easy, CURLcode*result, void*private) {
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

u8 download_is_successful(CURLcode result, CURL* easy) {
  if (result == CURLE_OK) {
    long proto;
    curl_easy_getinfo(easy, CURLINFO_PROTOCOL, &proto);
    if (proto == CURLPROTO_HTTP || proto == CURLPROTO_HTTPS ) {
      long response_code;
      curl_easy_getinfo(easy, CURLINFO_RESPONSE_CODE, &response_code);
      if (response_code == 200) {
        return 1;
      } if (response_code == 304) {
        INFO("Download finished, not modified response_code:%ld", response_code);
        return 2;
      } else {
        ERROR("got bad response_code:%ld", response_code);
        return 0;
      }
    } else {
      return 1;
    }
  } else {
    ERROR("CURL_ERROR:%s", curl_easy_strerror(result));
    return 0;
  }
}


void process_io_events() {
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
      MyEPData data = {.data = epe.data};
      if (data.my_data.event_type == EVENT_TYPE_DOWNLOAD) {
        download_io_event(epe);
      }
    }
  }
}

void _perform_all() {
  int running = 3;
  while (running > 0) {

    process_io_events();

    CURLcode result; CURL* easy = 0; _WriteCtx *c;
    while (get_completed_curls(&easy, &result, &c)) {
      LOGCTX(" id:%d", c->id);
      running --;
      u8 is_success = download_is_successful(result, easy);
      if (is_success == 1) {
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256_Final(hash, &c->sha256);

        printf("%d ", c->id);
        for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
          printf("%02x", hash[i]);
        }
        printf("\n");
      }
      _dl_free(c);
    }
  }


}

void download_test() {

  setbuf(stdout, 0);
  setbuf(stderr, 0);

  epoll_fd = epoll_create1(EPOLL_CLOEXEC);

  share = curl_share_init();
  curl_share_setopt(share, CURLSHOPT_SHARE, CURL_LOCK_DATA_SSL_SESSION);
  curl_share_setopt(share, CURLSHOPT_SHARE, CURL_LOCK_DATA_COOKIE);

  download_initialize();

  _WriteCtx c1 = {.id = 1};
  char* url = "http://127.0.0.1:9160/workspaces/the-bike-shed/README.md";
  char* url2 = "http://127.0.0.1:9161/workspaces/the-bike-shed/README.md";
  _dl(&c1, url, 0, 0);

  _WriteCtx c2 = {.id = 2};
  _dl(&c2, "ftp://127.0.0.1:232/asdfas", 0, 0);

  _WriteCtx c3 = {.id = 3};
  _dl(&c3, url2, 0, 0);

  _perform_all();

  _WriteCtx c4 = {.id = 1};
  _dl(&c4, url, c1.etag, c1.modified_time);

  _WriteCtx c5 = {.id = 2};
  _dl(&c5, url, 0, 0);

  _WriteCtx c6 = {.id = 3};
  _dl(&c6, url2, 0, 0);

  _perform_all();




  curl_share_cleanup(share);
}

int main() {
  download_test();
}