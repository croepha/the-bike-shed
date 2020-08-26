#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>

#include "logging.h"

#include "io_curl.h"
#include "email.h"


CURL* io_curl_create_handle() {
  return curl_easy_init();
}

int main() {
  email_setup(
    "from@longlonglonglonglonglonglonglonghost.com",
    "smtp://127.0.0.1:8025",
    "username:password");

  char* body_ =
    "this is a long body\n"
    "more more lines\n"
    ;

  for (int i=0; i<1;i++) {
    struct email_Send email_ctx;
    email_init(
        &email_ctx, "to@longlonglonglonglonglonglonglonghost.com", body_,
        strlen(body_),
        " asdk fja;lksf;lasjdklfa;klsdfj;akdsjf;ajd;kljadk fa;dk f;akd "
        " asdlfaskl;djfa;klsf;kajsd;klf askdf a;kldf ;lasd f;a subject!! 6");

    CURLcode ret = curl_easy_perform(email_ctx.easy);
    CURLCHECK(ret);
    email_free(&email_ctx);

    curl_global_cleanup();
    DEBUG("end?");

  }
}