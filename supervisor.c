#define LOG_DEBUG

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
#include "config.h"

u64 now_ms() { return real_now_ms(); }


u32 supr_email_low_threshold_bytes = 1 << 14; // 16 KB
u32 supr_email_rapid_threshold_secs = 20; // 20 Seconds  Prevent emails from being sent more often than this
u32 supr_email_low_threshold_secs = 60 * 10; // 1 Minutes
u32 supr_email_timeout_secs = 60 * 2;       // 2 Minutes



char * email_from;
char * email_host;
char * email_user_pass;
char * email_rcpt;
char ** supr_child_args;
struct StringList tmp_arg;
// char * email_from = "tmp-from@testtest.test";
// char * email_host = "smtp://127.0.0.1:8025";
// char * email_user_pass = "user:pass";
// //char * supr_child_args[] = { "/bin/sh", "-c", "/usr/bin/ping 127.0.0.1 | ts", 0 };
// char * supr_email_rcpt = "logging@tmp-test.test";

void supr_exec_child() { int r;
    INFO("count:%d", tmp_arg.count);
    assert(tmp_arg.count > 0);
    char * argv[32] = {}; // TODO...
    assert( tmp_arg.count + 1 <= COUNT(argv) );
    string_list_copy_to_array(argv, &tmp_arg);
    DEBUG("ARGV START:");
    for (char ** c = argv; *c; c++) {
        DEBUG("\tARGV:%s", *c);
    }
    r = execvp(*argv, argv);  error_check(r);
}
void supr_test_hook_pre_restart() {}
void supr_test_hook_pre_wait() {}
void shed_add_philantropist_hex(char* hex) { }

static void config_validate_or_exit() {
    log_allowed_fails = 1000;
    u8 errors = 0;
    #define _(v) if (&v && !v) { ERROR("config not set for " #v); errors++; }
    _(email_from)
    _(email_rcpt)
    _(email_host)
    _(email_user_pass)
    #undef _
    if (errors) { exit(-1); }
}

int main (int argc, char ** argv) {
    assert(argc == 2);
    setlinebuf(stderr);
    config_load_file(*++argv);
    config_validate_or_exit();
    io_curl_initialize();
    supr_main();
}