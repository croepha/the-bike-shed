
#include <stdio.h>
#include "logging.h"
#include "config.h"
#include "access.h"
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


int main (int argc, char ** argv) {
    assert(argc == 2);
    setlinebuf(stderr);
    config_load_file(*++argv);
}

