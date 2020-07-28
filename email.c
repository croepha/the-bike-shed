#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "logging.h"
#include "email.h"
#include "io_curl.h"

#define MIN(a,b) (a < b ? a : b)

static char *from_addr;
static char *smtp_server;
static char *user_pass;


void email_setup(char* from_addr_, char* smtp_server_, char* user_pass_) {
  from_addr = from_addr_;
  smtp_server = smtp_server_;
  user_pass = user_pass_;
}


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

void email_init(struct email_Send *ctx, CURL *easy, char *to_addr, char *body_,
                size_t body_len_, char *subject) {
  assert(from_addr && strlen(from_addr));
  assert(smtp_server && strlen(smtp_server));
  assert(user_pass && strlen(user_pass));

  memset(ctx, 0, sizeof(struct email_Send));

  ctx->state = EMAIL_STATE_SENDING_HEADER;
  ctx->easy = easy;

  int r = snprintf(ctx->header, sizeof ctx->header,
                   "From: <%s>\n"
                   "To: <%s>\n"
                   "Subject: %s\n"
                   "\n",
                   from_addr, to_addr, subject);
  DEBUG("r:%d", r);
  assert(r>=0);
  assert(r<sizeof ctx->header);
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
  CURLESET(URL, smtp_server);
  CURLESET(UPLOAD, 1L);
  CURLESET(USERPWD, user_pass);
  CURLESET(USERAGENT, "curl/7.68.0");
  CURLESET(MAXREDIRS, 50L);
  CURLESET(HTTP_VERSION, (long)CURL_HTTP_VERSION_2TLS);
  CURLESET(SSH_KNOWNHOSTS, "/root/.ssh/known_hosts");
  CURLESET(VERBOSE, 1L);
  CURLESET(TCP_KEEPALIVE, 1L);
  CURLESET(MAIL_FROM, from_addr);
  CURLESET(MAIL_RCPT, ctx->rcpt_list);
  CURLESET(INFILESIZE_LARGE, (curl_off_t)(ctx->body_len+ctx->header_len));
}

void email_free(struct email_Send *ctx) {
  curl_slist_free_all(ctx->rcpt_list);
  curl_easy_cleanup(ctx->easy);
}
