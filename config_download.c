#include <string.h>

#include "common.h"
#include "logging.h"
#include "config.h"
#include "io.h"
#include "io_curl.h"

#include "config.h"
#include "config_download.h"


static void __line_handler(char* line) {
  config_parse_line(line, 0);
}

struct line_accumulator_Data leftover_d;

__attribute__((unused))     // TODO
static size_t config_download_write_callback(char *data, size_t size, size_t nmemb, void *userdata) {
  size = size * nmemb;
  line_accumulator(&leftover_d, data, size, __line_handler);
  return size;
}

char percon_config_etag[32];
u64  percon_config_modtime_sec;

struct config_download_Ctx static config_download_curl_ctx;
IO_CURL_SETUP(config_download, struct config_download_Ctx, curl_type);

static void io_curl_free(CURL ** easy) {
    io_curl_abort(*easy);
    curl_easy_cleanup(*easy);
    *easy = 0;
}
void config_download_io_curl_complete(CURL *easy, CURLcode result,
                                      struct config_download_Ctx *ctx) {
  assert(easy == ctx->easy);
  // TODO: What if config doesn't have a trailing new line, does that mean we
  // don't do the last config line?
  io_curl_free(&ctx->easy);
}

//u64 config_download_interval_sec = 60 * 60; // 1 Hour
u64 config_download_interval_sec = 20;
char * config_download_url = "http://127.0.0.1:9160/workspaces/the-bike-shed/shed_test_config";

void config_download_timeout() {
    io_curl_free(&config_download_curl_ctx.easy);
    config_download_io_curl_create_handle(&config_download_curl_ctx);
    memset(&leftover_d, 0, sizeof leftover_d);


    IO_TIMER_MS(config_download) = now_sec() + config_download_interval_sec;
}





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

