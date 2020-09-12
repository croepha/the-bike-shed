// TODO: might want to avoid curl_easy_cleanup between emails, ie keep the connection open for reuse between emails...

#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "logging.h"
#include "email.h"
#include "io_curl.h"

#define MIN(a,b) (a < b ? a : b)

u64 now_sec();

// This could really be generalized out, like it would be nice to have a read_callback that can take
//   io_vec[]
static size_t email_read_callback(void *ptr, size_t size, size_t nmemb,
                           void *userdata) {
  struct email_Send *ctx = userdata;
  DEBUG();

  if (ctx->state == EMAIL_STATE_SENDING_HEADER) {
    assert(ctx->header_len > ctx->bytes_sent);
    size_t to_send = MIN(size*nmemb, ctx->header_len - ctx->bytes_sent);
    memcpy(ptr, ctx->header+ctx->bytes_sent, to_send);
    ctx->bytes_sent += to_send;
    if (ctx->bytes_sent == ctx->header_len) {
      ctx->bytes_sent = 0;
      ctx->state = EMAIL_STATE_SENDING_BODY;
      LOGCTX("nested_read");
      return to_send + email_read_callback(ptr + to_send, 1,
                                           size * nmemb - to_send, userdata);
      ;
    } else {
      return to_send;
    }
  } else { assert(ctx->state == EMAIL_STATE_SENDING_BODY);
    if (ctx->body_len == ctx->bytes_sent) {
      return 0;
    }
    size_t to_send = MIN(size*nmemb, ctx->body_len - ctx->bytes_sent);
    memcpy(ptr, ctx->body+ctx->bytes_sent, to_send);
    ctx->bytes_sent += to_send;
    return to_send;
  }
}


void email_init(struct email_Send *ctx, CURL*easy, char const * to_addr, char const * body_,
                size_t body_len_, char const * subject) {
  assert(!ctx->easy);
  assert(email_from && strlen(email_from));
  assert(email_host && strlen(email_host));
  assert(email_user_pass && strlen(email_user_pass));

  memset(ctx, 0, sizeof(struct email_Send));

  ctx->state = EMAIL_STATE_SENDING_HEADER;
  ctx->easy = easy;

  time_t  now_seconds = now_sec();
  struct tm now_tmstruct; gmtime_r(&now_seconds, &now_tmstruct);
  char now_string[50];
  ssize_t sr = strftime(now_string, sizeof now_string, "%a, %d %b %Y %T %z",
                        &now_tmstruct); error_check(sr);
  int r = snprintf(ctx->header, sizeof ctx->header,
                   "Date: %s\n"
                   "From: <%s>\n"
                   "To: <%s>\n"
                   "Subject: %s\n"
                   "\n",
                   now_string, email_from, to_addr, subject);
  error_check(r);
  assert(r < sizeof ctx->header);
  if (r >= sizeof ctx->header) {
    r = sizeof ctx->header - 1;
  }
  ctx->header_len = r;

  ctx->body = body_;
  ctx->body_len = body_len_;

  ctx->rcpt_list = curl_slist_append(ctx->rcpt_list, to_addr);

  /* abort if slower than 30 bytes/sec during 60 seconds */
  CURLESET(LOW_SPEED_TIME, 60L); // todo move this out
  CURLESET(LOW_SPEED_LIMIT, 30L);

  CURLESET(READFUNCTION, email_read_callback);
  CURLESET(READDATA, ctx);

  CURLESET(BUFFERSIZE, 102400L);
  CURLESET(URL, email_host);
  CURLESET(UPLOAD, 1L);
  CURLESET(USERPWD, email_user_pass);
  CURLESET(USERAGENT, "curl/7.68.0");
  CURLESET(MAXREDIRS, 50L);
  CURLESET(HTTP_VERSION, (long)CURL_HTTP_VERSION_2TLS);
  CURLESET(SSH_KNOWNHOSTS, "/root/.ssh/known_hosts");
  //CURLESET(VERBOSE, 1L);
  CURLESET(TCP_KEEPALIVE, 1L);
  CURLESET(MAIL_FROM, email_from);
  CURLESET(MAIL_RCPT, ctx->rcpt_list);
  CURLESET(INFILESIZE_LARGE, (curl_off_t)(ctx->body_len+ctx->header_len));
}

void email_free(struct email_Send *ctx) {
  io_curl_abort(ctx->easy);
  curl_slist_free_all(ctx->rcpt_list);
  curl_easy_cleanup(ctx->easy);
  ctx->easy = 0;
}


