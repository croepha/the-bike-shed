

//#include "config.h"

#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include "common.h"
#include "email.h"
#include "logging.h"
#include "io_curl.h"
#include "supervisor.h"

u64 now_sec() { return time(0); }

char * email_from = "tmp-from@testtest.test";
char * email_host = "smtp://127.0.0.1:8025";
char * email_user_pass = "user:pass";
char * supr_child_args[] = { "/bin/sh", "-c", "/usr/bin/ping 127.0.0.1 | ts", 0 };
char * supr_email_rcpt = "logging@tmp-test.test";

void supr_exec_child() { int r;
    r = execvp(*supr_child_args, supr_child_args);          error_check(r);
}
void supr_test_hook_pre_restart() {}
void supr_test_hook_pre_wait() {}

void supr_main();
int main () {
    setlinebuf(stderr);
    io_curl_initialize();
    supr_main();
}