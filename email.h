#pragma once

#include <stdlib.h>

typedef void CURL;

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
