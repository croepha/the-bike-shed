#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "logging.h"

#include "_curl.h"
#include "email.h"


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
