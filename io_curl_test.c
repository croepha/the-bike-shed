
// TODO: Write test for timeout on really slow url
#define LOG_DEBUG
#include <stdlib.h>
#include <string.h>

// internal:
#include <errno.h>
#include <sys/epoll.h>
#include <unistd.h>

#include <curl/curl.h>


#include <inttypes.h>
#include "common.h"
#include "logging.h"
#include "io.h"
#include "io_curl.h"
#include "downloading.h"

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
    enum _io_curl_type curl_type;
} _WriteCtx;

u64 now_sec() { return 0; }

static size_t _write_function(void *contents, size_t size, size_t nmemb, void*userp) {
    size_t realsize = size * nmemb;
    _WriteCtx *c = (_WriteCtx*)userp;
    c->downloaded += realsize;
    u64 now = now_ms();
    if (c->print_time + 500 < now) {
        c->print_time = now;
        double total_size;
        CURLcode cr = curl_easy_getinfo(c->curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &total_size);
        error_check_curl(cr);
        DEBUG("%d, %f ", c->id, (float)c->downloaded/(float)total_size);
    }
    // print("size", size, "nmemb", nmemb);
    SHA256_Update(&c->sha256, contents, realsize);
    return realsize;
}


#include "/build/parse_headers.re.c"

static size_t _header_callback(char *buffer, size_t _s, size_t nitems, void *userdata) {
  _WriteCtx * c = userdata;
  buffer[nitems-2] = 0;
  DEBUG_BUFFER(buffer, nitems, "Got header:");
  struct ParsedHeader header =  parse_header(buffer);
  switch (header.type) { default: break;
    case HEADER_TYPE_ETAG: {
      DEBUG("HEADER_TYPE_ETAG %s", header.value);
      size_t etag_len = strlen(header.value);
      if (etag_len < sizeof c->etag) {
        memcpy(c->etag, header.value, etag_len + 1);
      }
    } break;
    case HEADER_TYPE_LAST_MODIFIED: {
      u64 seconds = curl_getdate(header.value, 0);
      DEBUG_BUFFER(header.value, strlen(header.value), "HEADER_TYPE_LAST_MODIFIED %"PRId64"", seconds);
      c->modified_time = seconds;
    } break;
  }
  return nitems;
}



IO_CURL_SETUP(test, _WriteCtx, curl_type);




int pending_events;
static void _dl(_WriteCtx *c, char* url, char* previous_etag, u64 previous_mod_time) {
  DEBUG("c:%p id:%02d", c, c->id);
  SHA256_Init(&c->sha256);
  c->headers_list = NULL;
  c->curl_type = _io_curl_type_test;
  c->curl = test_io_curl_create_handle(c);

  if (previous_etag) {
    char tmp[256];
    int r = snprintf(tmp, sizeof tmp, "If-None-Match: \"%s\"", previous_etag);
    if (r >= sizeof tmp) {
      ERROR("etag too big");
    } else {
      c->headers_list = curl_slist_append(c->headers_list, tmp);
    }
  }

  {
    CURL* easy = c->curl;
    CURLESET(HTTPHEADER, c->headers_list);
    CURLESET(HEADERFUNCTION, _header_callback);
    CURLESET(WRITEFUNCTION, _write_function);
    if (previous_mod_time) {
      CURLESET(TIMECONDITION, (long)CURL_TIMECOND_IFMODSINCE);
      CURLESET(TIMEVALUE_LARGE, (curl_off_t)previous_mod_time);
    }
    //CURLESET(COOKIEFILE, "");
    CURLESET(USERAGENT, "the-bike-shed/0");
    //CURLESET(VERBOSE, 1);
    CURLESET(MAXREDIRS, 50L);
    CURLESET(WRITEDATA, c);
    CURLESET(HEADERDATA, c);
    CURLESET(URL, url);

  }
  pending_events ++;

}

static void _dl_free(_WriteCtx *c) {
  curl_slist_free_all(c->headers_list);
  curl_easy_cleanup(c->curl);

}

static u8 download_is_successful(CURLcode result, CURL* easy) { CURLcode cr;
  if (result == CURLE_OK) {
    long proto;
    cr = curl_easy_getinfo(easy, CURLINFO_PROTOCOL, &proto);
    error_check_curl(cr);

    if (proto == CURLPROTO_HTTP || proto == CURLPROTO_HTTPS ) {
      long response_code;
      cr = curl_easy_getinfo(easy, CURLINFO_RESPONSE_CODE, &response_code);
      error_check_curl(cr);
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

static void test_io_curl_complete(CURL *easy, CURLcode result, _WriteCtx *c) {
  DEBUG("c:%p", c);
  LOGCTX(" test_sort:id:%02d", c->id);
  pending_events --;
  log_allowed_fails = 100;
  u8 is_success = download_is_successful(result, easy);
  if (is_success == 1) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_Final(hash, &c->sha256);
    INFO_HEXBUFFER(hash, SHA256_DIGEST_LENGTH);
  }
  _dl_free(c);
}


static void _perform_all() {
  while (pending_events > 0) {
    io_process_events();
    io_curl_process_events();
  }
}



void logging_send_timeout() { }

static void download_test() {

  setbuf(stdout, 0);
  setbuf(stderr, 0);

  io_initialize();
  io_curl_initialize();

  _WriteCtx c1 = {.id = 1};
  char* url = "http://127.0.0.1:9160/workspaces/the-bike-shed/README.md";
  char* url2 = "http://127.0.0.1:9161/workspaces/the-bike-shed/README.md";
  _dl(&c1, url, 0, 0);

  _WriteCtx c2 = {.id = 2};
  _dl(&c2, "ftp://127.0.0.1:232/asdfas", 0, 0);

  _WriteCtx c3 = {.id = 3};
  _dl(&c3, url2, 0, 0);

  _perform_all();

  _WriteCtx c4 = {.id = 4};
  _dl(&c4, url, c1.etag, c1.modified_time);

  _WriteCtx c5 = {.id = 5};
  _dl(&c5, url, 0, 0);

  _WriteCtx c6 = {.id = 6};
  _dl(&c6, url2, 0, 0);

  _perform_all();

}

int main() {
  download_test();
}
