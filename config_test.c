
#include "logging.h"

char * email_from = "tmp-from@testtest.test";
char * email_host = "smtp://127.0.0.1:8025";
char * email_user_pass = "user:pass";
char * supr_email_rcpt = "logging@tmp-test.test";
char * supr_child_argv[] = { "/bin/sh", "-c", "/usr/bin/ping 127.0.0.1 | ts", 0 };


int main () {

    INFO("email_from: '%s'", email_from);
    INFO("email_host: '%s'", email_host);
    INFO("email_user_pass: '%s'", email_user_pass);
    INFO("supr_email_rcpt: '%s'", supr_email_rcpt);
    INFO("supr_child_argv:");
    for (char**c = supr_child_argv; *c; c++) {
        INFO("\t'%s'", *c);
    }


}