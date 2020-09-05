
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/signalfd.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "logging.h"

int read_from_child_fd = -1;
int child_write_to_fd = -1;
int signal_fd = -1;
pid_t child_pid;

void start_child() { int r;
  r = fflush(stdout);                                            error_check(r);
  r = fflush(stderr);                                            error_check(r);
  child_pid = fork();                                            error_check(child_pid);
  if (child_pid == 0) {
    struct rlimit limits = { };
    r = setrlimit(RLIMIT_CORE, &limits);                         error_check(r);
    r = dup2(child_write_to_fd, 1);                              error_check(r);
    r = dup2(child_write_to_fd, 2);                              error_check(r);
    r = execl("/bin/sh", "-c", "cat /etc/lsb_release");          error_check(r);
  }
}

void sigfd_io_event(...) {
    struct signalfd_siginfo siginfo;
    int r = read(signal_fd, &siginfo, sizeof siginfo);
    error_check(r);
    for(;;) {
        int wstatus;
        pid_t pid = waitpid(0, &wstatus, WNOHANG);       error_check(pid);
        if (pid == 0) break;
        // TODO do stuff... log wait status
        start_child();
    }
}

void read_from_child_io_event(...) { int r;
  char buf[1024];
  r = read(read_from_child_fd, buf, sizeof buf - 1);   error_check(r);
  buf[r] = 0;
  printf("read_from_child: `%.*s'\n", r, buf);

}

int main () { int r;
  {
    sigset_t mask;
    r = sigemptyset(&mask);                                       error_check(r);
    r = sigaddset(&mask, SIGCHLD);                                error_check(r);
    r = sigprocmask(SIG_BLOCK, &mask, NULL);                      error_check(r);
    signal_fd = signalfd(-1, &mask, SFD_NONBLOCK | SFD_CLOEXEC);  error_check(signal_fd);
  }

  {
    int sv[2];
    r = socketpair(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC | SOCK_NONBLOCK, 0, sv); error_check(r);
    read_from_child_fd = sv[0]; child_write_to_fd  = sv[1];
    r = shutdown(child_write_to_fd , SHUT_RD);                     error_check(r);
    r = shutdown(read_from_child_fd, SHUT_WR);                     error_check(r);
  }
}

