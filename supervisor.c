#define LOGF_CARBON_COPY_IMPL
#define LOG_DEBUG

//#include "config.h"

#include <sys/mman.h>
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
#include "io.h"

// TODO: Its super wierd that we need access stuff here, this
//  is because of how we use the same config code for both
//  processes, we should separate them out a bit, so we can
//  decouple the two
#include "access.h"

u64 now_ms() { return real_now_ms(); }


IO_TIMEOUT_CALLBACK(idle) {}

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
u32 day_sec_open;
u32 day_sec_close;


char access_salt_old[SALT_BUF_LEN];
char access_salt[SALT_BUF_LEN];


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
// void shed_add_philantropist_hex(char* hex) { }



u8 carbon_copy_did_header = 0;
void logf_carbon_copy_v(const char *fmt, va_list va) { int r;
  char* buf; usz buf_len;

  if (!carbon_copy_did_header) {
    carbon_copy_did_header = 1;
    supr_email_add_data_start(&buf, &buf_len);
    r = snprintf(buf, buf_len, "SUPERVISOR: ");
    error_check(r);
    supr_email_add_data_finish(r);
  }

  supr_email_add_data_start(&buf, &buf_len);
  r = vsnprintf(buf, buf_len, fmt, va);
  error_check(r);
  supr_email_add_data_finish(r);
}

void logf_carbon_copy_end() {
    carbon_copy_did_header = 0;
}


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
    int r = mlockall( MCL_CURRENT | MCL_FUTURE );
    error_check(r);

    assert(argc == 2);
    setlinebuf(stderr);
    config_load_file(*++argv);
    config_validate_or_exit();
    io_curl_initialize();
    supr_main();
}

