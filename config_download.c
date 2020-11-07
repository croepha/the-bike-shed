#include <curl/curl.h>
//#define LOG_DEBUG
#include <string.h>

#include "common.h"
#include "logging.h"
#include "config.h"
#include "io.h"
#include "io_curl.h"
#include <inttypes.h>

#include "config.h"
#include "config_download.h"



IO_CURL_SETUP(config_download, struct config_download_Ctx, curl_type);

static void __line_handler(char* line) {
  config_parse_line(line, 0);
}

static size_t _write_function(void *contents, size_t size, size_t nmemb, void*userp) {
  struct config_download_Ctx *c = (struct config_download_Ctx *)userp;
  CURLcode cr;
  int response_code;
  cr = curl_easy_getinfo(c->easy, CURLINFO_RESPONSE_CODE, &response_code);
  error_check_curl(cr);
  size = size * nmemb;
  if (response_code == 200) {
    line_accumulator(&c->line_accumulator_data, contents, size, __line_handler);
  }
  return size;
}


#include "/build/parse_headers.re.c"

static size_t _header_callback(char *buffer, size_t _s, size_t nitems, void *userdata) {
  struct config_download_Ctx *c = userdata;
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
      c->modified_time_sec = seconds;
    } break;
  }
  return nitems;
}

void __config_download_start(struct config_download_Ctx *c, char *url,
                             char *previous_etag, u64 previous_mod_time_sec) {
  DEBUG("c:%p id:%02d", c, c->id);
  c->headers_list = NULL;
  c->curl_type = _io_curl_type_test;
  c->easy = config_download_io_curl_create_handle(c);

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
    CURL *easy = c->easy;
    CURLESET(HTTPHEADER, c->headers_list);
    CURLESET(HEADERFUNCTION, _header_callback);
    CURLESET(WRITEFUNCTION, _write_function);
    if (previous_mod_time_sec) {
      CURLESET(TIMECONDITION, (long)CURL_TIMECOND_IFMODSINCE);
      CURLESET(TIMEVALUE_LARGE, (curl_off_t)previous_mod_time_sec);
    }
    //CURLESET(COOKIEFILE, "");
    CURLESET(USERAGENT, "the-bike-shed/0");
    //CURLESET(VERBOSE, 1);
    CURLESET(MAXREDIRS, 50L);
    CURLESET(WRITEDATA, c);
    CURLESET(HEADERDATA, c);
    CURLESET(URL, url);

  }
}

static void io_curl_free(CURL ** easy) {
    io_curl_abort(*easy);
    curl_easy_cleanup(*easy);
    *easy = 0;
}

void config_download_abort(struct config_download_Ctx *c) {
  if (c->easy) {
    curl_slist_free_all(c->headers_list);
    io_curl_free(&c->easy);
  }
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

static void config_download_io_curl_complete(CURL *easy, CURLcode result,
                                  struct config_download_Ctx *c) {
  assert(easy == c->easy);
  DEBUG("c:%p", c);
  LOGCTX(" test_sort:id:%02d", c->id);
  __debug_config_download_complete_hook();
  u8 is_success = download_is_successful(result, easy);
  if (is_success == 2) {
    INFO("Download finished successfully (not modified)");
  } else if (is_success) {
    INFO("Download finished successfully");
  }
  config_download_finished(c, is_success == 1);
  config_download_abort(c);
}




// char percon_config_etag[32];
// u64  percon_config_modtime_sec;





// #include <inttypes.h>

// #include "io_curl.h"
// #include "/build/parse_headers.re.c"


// struct curl_slist* headers_list;


// static size_t _header_callback(char *buffer, size_t _s, size_t nitems, void *userdata) {
//   _WriteCtx * c = userdata;
//   buffer[nitems-2] = 0;
//   DEBUG("Got header: '%s'", buffer);
//   struct ParsedHeader header =  parse_header(buffer);
//   switch (header.type) { default: break;
//     case HEADER_TYPE_ETAG: {
//       DEBUG("HEADER_TYPE_ETAG %s\n", header.value);
//       size_t etag_len = strlen(header.value);
//       if (etag_len < sizeof c->etag) {
//         memcpy(c->etag, header.value, etag_len + 1);
//       }
//     } break;
//     case HEADER_TYPE_LAST_MODIFIED: {
//       u64 seconds = curl_getdate(header.value, 0);
//       DEBUG("HEADER_TYPE_LAST_MODIFIED %s %"PRId64"\n", header.value, seconds);
//       c->modified_time = seconds;
//     } break;
//   }
//   return nitems;
// }

