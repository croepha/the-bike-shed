
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/signalfd.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "common.h"
#include "io.h"
#include "io_curl.h"
#include "logging.h"
#include "supervisor.h"

int   supr_read_from_child_fd = -1;
int   supr_child_write_to_fd = -1;
int   supr_signal_fd = -1;
pid_t supr_child_pid;

sigset_t blocked_signal_mask;


static void supr_start_child() { int r;
  r = fflush(stdout);                                            error_check(r);
  r = fflush(stderr);                                            error_check(r);
  supr_child_pid = fork();                                       error_check(supr_child_pid);
  if (supr_child_pid == 0) {
    r = sigprocmask(SIG_UNBLOCK, &blocked_signal_mask, NULL);  error_check(r);

    struct rlimit limits = { 1<<30, 1<<30 };
    r = setrlimit(RLIMIT_CORE, &limits);                         error_check(r);
    r = dup2(supr_child_write_to_fd, 1);                         error_check(r);
    r = dup2(supr_child_write_to_fd, 2);                         error_check(r);
    supr_exec_child();
  }
  INFO("Child:%d forked", supr_child_pid);
}

// TODO: Add a signal handler to force a flush of logs

static u8 already_got_die_request;

IO_EVENT_CALLBACK(supr_signal, epe) { int r;
    struct signalfd_siginfo siginfo;
    r = read(supr_signal_fd, &siginfo, sizeof siginfo);        error_check(r);

    switch (siginfo.ssi_signo) { SWITCH_DEFAULT_IS_UNEXPECTED;
      case SIGCHLD: {
        for(;;) {
            int wstatus;
            pid_t pid = waitpid(0, &wstatus, WNOHANG);       error_check(pid);
            if (pid == 0) break;
            // TODO do stuff... log wait status
            assert(!WIFSTOPPED(wstatus));
            assert(WIFEXITED(wstatus) || WIFSIGNALED(wstatus) );
            if (WIFEXITED(wstatus)) {
              ERROR("Child:%d exited: status:%d", pid, WEXITSTATUS(wstatus) );
            }
            if (WIFSIGNALED(wstatus)) {
              ERROR("Child:%d terminated signal:%d dump:%d", pid, WTERMSIG(wstatus), WCOREDUMP(wstatus) );
            }
            if (pid == supr_child_pid) {
              supr_test_hook_pre_restart();
              supr_start_child();
            } else {
              ERROR("Strange, pid isn't our main child... doing nothign");
            }
        }
      } break;
      case SIGUSR1: {
          INFO("Force flushing due to SIGUSR1");
          supr_email_push();
      } break;
      case SIGINT:
      case SIGTERM: {
        if (already_got_die_request) {
          WARN("already_got_die_request");
          exit(-1);
        } else {
          INFO("Force flushing before exiting");
          already_got_die_request = 1;
          supr_email_push();
        }

      } break;

    }
}

void supr_email_done_hook() {
  if (already_got_die_request) {
    exit(-1);
  }
}

IO_EVENT_CALLBACK(supr_read_from_child, epe) { int r;
  char* buf; usz buf_len; supr_email_add_data_start(&buf, &buf_len);
  r = read(supr_read_from_child_fd, buf, buf_len);   error_check(r);
  supr_email_add_data_finish(r);
}

// TODO: We should have a lock file to


void supr_main () { int r;
  setlinebuf(stderr);
  //supr_exec_child();
  io_initialize();

  r = sigemptyset(&blocked_signal_mask);                     error_check(r);
  r = sigaddset(&blocked_signal_mask, SIGCHLD);              error_check(r);
  r = sigaddset(&blocked_signal_mask, SIGINT);               error_check(r);
  r = sigaddset(&blocked_signal_mask, SIGTERM);              error_check(r);
  r = sigaddset(&blocked_signal_mask, SIGUSR1);              error_check(r);


  {
    r = sigprocmask(SIG_BLOCK, &blocked_signal_mask, NULL);                               error_check(r);
    supr_signal_fd = signalfd(-1, &blocked_signal_mask, SFD_NONBLOCK | SFD_CLOEXEC);      error_check(supr_signal_fd);

    io_ADD_R(supr_signal_fd);

  }

  {
    int sv[2];
    r = socketpair(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC | SOCK_NONBLOCK, 0, sv); error_check(r);
    supr_read_from_child_fd = sv[0];
    supr_child_write_to_fd  = sv[1];
    r = shutdown(supr_child_write_to_fd, SHUT_RD);      error_check(r);
    r = shutdown(supr_read_from_child_fd, SHUT_WR);     error_check(r);

    io_ADD_R(supr_read_from_child_fd);
  }
  supr_start_child();
  for (;;) {
    supr_test_hook_pre_wait();
    io_process_events();
    io_curl_process_events();
  }
}
