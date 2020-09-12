#pragma once

#include "io_curl.h"
#include <stdlib.h>

typedef void CURL;

struct email_Send {
  CURL * easy;
  char const * body;
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
void email_init(struct email_Send *ctx, CURL*easy, char const * to_addr, char const * body_,
                size_t body_len_, char const * subject);
void email_free(struct email_Send *ctx);


extern char * email_from;
extern char * email_host;
extern char * email_user_pass;

