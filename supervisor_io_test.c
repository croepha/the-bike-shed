
#include <unistd.h>
#include <errno.h>
#include "common.h"
#include "logging.h"

char buf[1024];

void supr_email_add_data_start(char**buf_, usz*buf_space_left) {
    *buf_ = buf;
    *buf_space_left = sizeof buf - 1;
}
void supr_email_add_data_finish(usz new_space_used) {
    assert(new_space_used < sizeof buf);
    buf[new_space_used] = 0;
    INFO_BUFFER(buf, new_space_used, "supr_email_add_data: ");
}


char** supr_child_argv;


void supr_exec_child() { int r;
    r = execvp(*supr_child_argv, supr_child_argv);          error_check(r);
}


void supr_main ();
int main(int argc, char** argv) {
    // for (int i=0; i<argc; i++) {
    //     INFO("Arg %d:%s", i, argv[i]);
    // }
    log_allowed_fails = 100;
    supr_child_argv = ++argv;
    supr_main(argc, argv);
}
