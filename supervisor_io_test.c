
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <sys/wait.h>
#include "common.h"
#include "logging.h"
#include "io_curl.h"
#include "supervisor.h"

char buf[1024];

u64 now_ms() { return real_now_ms(); }

void supr_email_add_data_start(char**buf_, usz*buf_space_left) {
    *buf_ = buf;
    *buf_space_left = sizeof buf - 1;
}
void supr_email_add_data_finish(usz new_space_used) {
    assert(new_space_used < sizeof buf);
    buf[new_space_used] = 0;
    INFO_BUFFER(buf, new_space_used, "supr_email_add_data: ");
}

void supr_test_hook_pre_wait() {
    INFO();
}

char ** test_commands[] = {
    (char*[]){ "sh", "-c", "echo \"11\"; sleep .01; echo \"12\"; sleep .02; echo \"13\"; sleep .01; echo \"14\"; sleep .01; kill -ABRT $$", 0},
    (char*[]){ "sh", "-c", "echo \"21\"; sleep .01; echo \"22\"; sleep .02; echo \"23\"; sleep .01; echo \"24\"; sleep .01; kill -INT  $$", 0},
    (char*[]){ "sh", "-c", "echo \"31\"; sleep .01; echo \"32\"; sleep .02; echo \"33\"; sleep .01; echo \"34\"; sleep .01; kill -KILL $$", 0},
    (char*[]){ "sh", "-c", "echo \"41\"; sleep .01; echo \"42\"; sleep .02; echo \"43\"; sleep .01; echo \"44\"; sleep .01; exit  0", 0},
    (char*[]){ "sh", "-c", "echo \"61\"; sleep .01; echo \"62\"; sleep .02; echo \"63\"; sleep .01; echo \"64\"; sleep .01; exit  1", 0},
    0};
char *** supr_test_child_argv = test_commands;
void supr_test_hook_pre_restart() {
    supr_test_child_argv++;
    if (!*supr_test_child_argv) exit(0);
}

void supr_exec_child() { int r;
    r = execvp(**supr_test_child_argv, *supr_test_child_argv);          error_check(r);
}

void io_curl_process_events() { INFO(); }

void supr_main();
int main() { int r;
    setlinebuf(stderr);
    r = alarm(3); error_check(r);
    log_allowed_fails = 100;
    supr_main();

    INFO("Reaping child procs");
    u8 had_error = 0;
    for (;;) {
        int wstatus;
        //DEBUG("Waiting for child");
        pid_t child = wait(&wstatus);
        if (child == -1 && errno == ECHILD) {
            break;
        }
        error_check(child);
        //INFO("Child exit:%d", WEXITSTATUS(wstatus));
        if (! WIFEXITED(wstatus) || WEXITSTATUS(wstatus) != 0) {
            had_error = 1;
        }
    }
    if (had_error) {
        ERROR("Atleast one child process had an error");
    }
}
