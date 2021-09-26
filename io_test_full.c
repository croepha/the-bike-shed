#define _GNU_SOURCE         /* See feature_test_macros(7) */
#include <stdio.h>
#include <curl/curl.h>
#include <fcntl.h>
#include "email.h"
#include "io.h"
#include "io_curl.h"
#include "io_test.h"
#include "logging.h"

u8 starting_email = 0;
u64 now_ms() {
  if (starting_email) {
    return 0;
  } else {
    return real_now_ms();
  }
}


IO_TIMEOUT_CALLBACK(logging_send) {
  INFO();
  IO_TIMER_SET_MS(logging_send, -1);
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
  char *body;
} EmailCtx;


IO_CURL_SETUP(test, DLCtx, curl_type);
IO_CURL_SETUP(logging, EmailCtx, curl_type);

void test_io_curl_complete(CURL* easy, CURLcode result, DLCtx * ctx) {
  INFO("test_sort:A%02d result: %s", ctx->id, curl_easy_strerror(result));
  int r = fclose(ctx->f); error_check(r);
  events_pending--;
  curl_easy_cleanup(easy);
}

void logging_io_curl_complete(CURL* easy, CURLcode result, EmailCtx * ctx) {
  INFO("test_sort:B%02d result: %s", ctx->id, curl_easy_strerror(result));
  events_pending--;
  email_free(&ctx->email);
  free(ctx->body); ctx->body = 0;
}

DLCtx dl_ctx[10];
EmailCtx email_ctx[10];

char * email_from = "from@longlonglonglonglonglonglonglonghost.com";
char * email_host = "smtp://127.0.0.1:8025";
char * email_user_pass = "username:password";

void test_main() {
  io_initialize();
  io_curl_initialize();

  for (int i=0; i<COUNT(dl_ctx); i++) {
    char buf[1024];
    snprintf(buf, sizeof buf, "/build/testfile-%02d", i);
    unlink(buf);
    int fd = open(buf, O_CREAT | O_WRONLY, 0644);
    error_check(fd);

    int r = dprintf(fd, "%s", buf);
    error_check(r);

    r = close(fd);
    error_check(r);

  }

  system("rm -f /build/io_test_full_* /build/email_mock_io_test_full*");

  start_time = IO_NOW_MS() + 50;

  starting_email = 1;

  for (int i=0; i<COUNT(dl_ctx); i++) {
    char buf[1024];
    CURL*easy = test_io_curl_create_handle(&dl_ctx[i]);
    snprintf(buf, sizeof buf, "/build/io_test_full_%02d", i);
    dl_ctx[i].f = fopen(buf, "w"); error_check(dl_ctx[i].f?0:-1);
    dl_ctx[i].id = i;
    CURLESET(WRITEDATA, dl_ctx[i].f);
    snprintf(buf, sizeof buf, "http://127.0.0.1:9161/build/testfile-%02d", i);
    CURLESET(URL, buf);
    //CURLESET(VERBOSE, 1);
    events_pending++;
  }

  memset(email_ctx, 0, sizeof email_ctx);
  for (int i=0; i<COUNT(email_ctx); i++) {
    asprintf(&email_ctx[i].body, "body: io_test_full%02d@asdfasdf.no", i);

    CURL*easy = logging_io_curl_create_handle(&email_ctx[i]);
    email_init(&email_ctx[i].email, easy, email_ctx[i].body+6,
        email_ctx[i].body, strlen(email_ctx[i].body), "asdasdf");
    // CURLESET(VERBOSE, 1);
    events_pending++;
  }

  starting_email = 0;

  IO_TIMER_SET_MS(logging_send, start_time);
  events_pending++;

  INFO("Running all events:"); { LOGCTX("\t");
    while (events_pending > 0) {
      io_process_events();
      io_curl_process_events();
    }
  }

  for (int i=0; i<COUNT(dl_ctx); i++) {
    char buf[1024];
    // im not sure why we need a grep here, but without it cat isn't dumping the file? maybe buffering?
    snprintf(buf, sizeof buf, "cat /build/io_test_full_%02d | grep -v 'asdfasdf' ", i);
    INFO("%s", buf);
    int r = system(buf); error_check(r);
  }
  for (int i=0; i<COUNT(email_ctx); i++) {
    char buf[1024];
    snprintf(buf, sizeof buf, "cat  /build/email_mock_io_test_full%02d@asdfasdf.no", i);
    INFO("%s", buf);
    int r = system(buf); error_check(r);
  }

  curl_global_cleanup();

}


