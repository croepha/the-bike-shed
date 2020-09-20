
#include <stdio.h>
#include "logging.h"
#include "config.h"

char * email_from;
char * email_host;
char * email_user_pass;
char * email_rcpt;
char ** supr_child_args;
struct StringList tmp_arg;


int main (int argc, char ** argv) {
    assert(argc == 2);
    setlinebuf(stderr);
    config_load_file(*++argv);
}


