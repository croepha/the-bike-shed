#include "email.h"
#include "io.h"
#include "io_curl.h"
#include "io_test.h"
#include "logging.h"
#include <curl/easy.h>


void logging_send_timeout() {
  INFO();
  IO_TIMER_MS(logging_send) = -1;
  events_pending--;
}

typedef struct {
  FILE* f;
  enum _io_curl_type curl_type;
  int id;
} DLCtx;

typedef struct {
  enum _io_curl_type curl_type;
  int id;
  struct email_Send email;
} EmailCtx;


IO_CURL_SETUP(test, DLCtx, curl_type);
IO_CURL_SETUP(logging, EmailCtx, curl_type);

void test_io_curl_complete(CURL* easy, CURLcode result, DLCtx * ctx) {
  INFO("result: %s", curl_easy_strerror(result));
  int r = fclose(ctx->f); error_check(r);
  events_pending--;
  curl_easy_cleanup(easy);
}

void logging_io_curl_complete(CURL* easy, CURLcode result, EmailCtx * ctx) {
  INFO("result: %s", curl_easy_strerror(result));
  events_pending--;
  curl_easy_cleanup(easy);
}

DLCtx dl_ctx;
EmailCtx email_ctx;

void test_main() {
  io_initialize();
  io_curl_initialize();

  unlink("/build/io_test_full_00");


  start_time = utc_ms_since_epoch() + 50;

  {
    CURL*easy = test_io_curl_create_handle(&dl_ctx);
    dl_ctx.f = fopen("/build/io_test_full_00", "w"); error_check(dl_ctx.f?0:-1);
    CURLESET(WRITEDATA, dl_ctx.f);
    CURLESET(URL, "https://httpbin.org/get?id=1");
    //CURLESET(VERBOSE, 1);
    events_pending++;
  }

  {
    CURL*easy = logging_io_curl_create_handle(&email_ctx);
    dl_ctx.f = fopen("/build/io_test_full_00", "w"); error_check(dl_ctx.f?0:-1);
    //email_Init
    CURLESET(WRITEDATA, dl_ctx.f);
    CURLESET(URL, "https://httpbin.org/get?id=1");
    // CURLESET(VERBOSE, 1);
    events_pending++;
  }


  IO_TIMER_MS(logging_send) = start_time;
  events_pending++;

  INFO("Running all events:"); { LOGCTX("\t");
    while (events_pending > 0) {
      io_process_events();
      io_curl_process_events();
    }
  }

  system("cat /build/io_test_full_00 | grep -v 'date:' | grep -v 'X-Amzn-Trace-Id' ");

}


