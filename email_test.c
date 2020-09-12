#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>

#include "logging.h"

#include "io_curl.h"
#include "email.h"

u64 now_sec() { return 0; }

char * email_from = "from@longlonglonglonglonglonglonglonghost.com";
char * email_host = "smtp://127.0.0.1:8025";
char * email_user_pass = "username:password";

void io_curl_abort(CURL* easy) {}
int main() {
  system("rm -f /build/email_mock_email_test_to@longlonglonglonglonglonglonglonghost.com");

  char* body_ =
    "this is a long body\n"
    "more more lines\n"
    ;

  for (int i=0; i<1;i++) {
    struct email_Send email_ctx = {};
    email_init(
        &email_ctx, curl_easy_init(), "email_test_to@longlonglonglonglonglonglonglonghost.com", body_,
        strlen(body_),
        " asdk fja;lksf;lasjdklfa;klsdfj;akdsjf;ajd;kljadk fa;dk f;akd "
        " asdlfaskl;djfa;klsf;kajsd;klf askdf a;kldf ;lasd f;a subject!! 6");

    CURLcode ret = curl_easy_perform(email_ctx.easy);
    CURLCHECK(ret);
    email_free(&email_ctx);

    curl_global_cleanup();
    DEBUG("end?");

    system("cat /build/email_mock_email_test_to@longlonglonglonglonglonglonglonghost.com");

  }
}