#include <stddef.h>
#include <curl/curl.h>
#include <stdint.h>
#include <sys/types.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "logging.h"

#define MIN(a,b) (a < b ? a : b)


#define CURLCHECK(_m_curl_code) ({ \
  if (_m_curl_code != CURLE_OK) { \
    ERROR("CURLCODE:%s", curl_easy_strerror(_m_curl_code)); } \
})

#define CURLESET(m_key, ...) ({ \
  CURLcode _m_curl_code = curl_easy_setopt( \
    easy, CURLOPT_ ## m_key, ##__VA_ARGS__); \
  if (_m_curl_code != CURLE_OK) { \
    ERROR("curl_easy_setopt(CURLOPT_" #m_key ", " #__VA_ARGS__"): %s", \
    curl_easy_strerror(_m_curl_code)); } \
})

char *email_from_addr = "to@longlonglonglonglonglonglonglonghost.com";
char *email_server = "smtp://127.0.0.1:8025";
char *email_user_pass = "username:password";

struct email_Send {
  CURL*easy;
  char* body;
  struct curl_slist *rcpt_list;
  size_t bytes_sent;
  size_t header_len;
  size_t body_len;
  char header[512];
  enum EMAIL_STATE {
    EMAIL_STATE_IDLE,
    EMAIL_STATE_SENDING_HEADER,
    EMAIL_STATE_SENDING_BODY,
  } state;
};
void email_init(struct email_Send *ctx, CURL *easy, char *to_addr, char *body_,
                size_t body_len_, char *subject);
void email_free_all(struct email_Send *ctx);

static size_t email_read_callback(void *ptr, size_t size, size_t nmemb,
                           void *userdata) {
  struct email_Send *ctx = userdata;
  DEBUG("");

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
  // if (state != EMAIL_STATE_IDLE) {
  //   ERROR("An email is already in flight");
  //   return 1;
  // }

  memset(ctx, 0, sizeof(struct email_Send));

  ctx->state = EMAIL_STATE_SENDING_HEADER;
  ctx->easy = easy;

  int r = snprintf(ctx->header, sizeof ctx->header,
                   "From: <%s>\n"
                   "To: <%s>\n"
                   "Subject: %s\n"
                   "\n",
                   email_from_addr, to_addr, subject);
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
  CURLESET(URL, email_server);
  CURLESET(UPLOAD, 1L);
  CURLESET(USERPWD, email_user_pass);
  CURLESET(USERAGENT, "curl/7.68.0");
  CURLESET(MAXREDIRS, 50L);
  CURLESET(HTTP_VERSION, (long)CURL_HTTP_VERSION_2TLS);
  CURLESET(SSH_KNOWNHOSTS, "/root/.ssh/known_hosts");
  CURLESET(VERBOSE, 1L);
  CURLESET(TCP_KEEPALIVE, 1L);
  CURLESET(MAIL_FROM, email_from_addr);
  CURLESET(MAIL_RCPT, ctx->rcpt_list);
  CURLESET(INFILESIZE_LARGE, (curl_off_t)(ctx->body_len+ctx->header_len));
}

void email_free_all(struct email_Send *ctx) {
  curl_slist_free_all(ctx->rcpt_list);
  curl_easy_cleanup(ctx->easy);
}

int main() {

  char* body_ =
    "this is a long body\n"
    "more more lines\n"
    ;

  for (int i=0; i<1;i++) {
    struct email_Send email_ctx;

    CURL *hnd = curl_easy_init();
    email_init(
        &email_ctx, hnd, "to@longlonglonglonglonglonglonglonghost.com", body_,
        strlen(body_),
        " asdk fja;lksf;lasjdklfa;klsdfj;akdsjf;ajd;kljadk fa;dk f;akd "
        " asdlfaskl;djfa;klsf;kajsd;klf askdf a;kldf ;lasd f;a subject!! 6");

    CURLcode ret = curl_easy_perform(hnd);
    CURLCHECK(ret);
    email_free_all(&email_ctx);

    curl_global_cleanup();
    DEBUG("end?");

  }

}
