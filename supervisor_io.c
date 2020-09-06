
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/signalfd.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "io.h"
#include "logging.h"

int   supr_read_from_child_fd = -1;
int   supr_child_write_to_fd = -1;
int   supr_signal_fd = -1;
pid_t supr_child_pid;

void supr_start_child() { int r;
  r = fflush(stdout);                                            error_check(r);
  r = fflush(stderr);                                            error_check(r);
  supr_child_pid = fork();                                       error_check(supr_child_pid);
  if (supr_child_pid == 0) {
    struct rlimit limits = { 1<<30, 1<<30 };
    r = setrlimit(RLIMIT_CORE, &limits);                         error_check(r);
    r = dup2(supr_child_write_to_fd, 1);                         error_check(r);
    r = dup2(supr_child_write_to_fd, 2);                         error_check(r);
    r = execl("/bin/sh", "-c", "echo SSS; sleep 1; cat /etc/lsb_release");          error_check(r);
  }
  fprintf(stderr, "Child:%d forked \n", supr_child_pid);
}

void supr_signal_fd_io_event(struct epoll_event epe) { int r;
    struct signalfd_siginfo siginfo;
    r = read(supr_signal_fd, &siginfo, sizeof siginfo);        error_check(r);
    assert(siginfo.ssi_signo == SIGCHLD);
    for(;;) {
        int wstatus;
        pid_t pid = waitpid(0, &wstatus, WNOHANG);       error_check(pid);
        if (pid == 0) break;
        // TODO do stuff... log wait status
        assert(!WIFSTOPPED(wstatus));
        assert(WIFEXITED(wstatus) || WIFSIGNALED(wstatus) );
        if (WIFEXITED(wstatus)) {
          fprintf(stderr, "Child:%d exited: status:%d\n", pid, WEXITSTATUS(wstatus) );
        }
        if (WIFSIGNALED(wstatus)) {
          fprintf(stderr, "Child:%d terminated signal:%d dump:%d\n", pid, WTERMSIG(wstatus), WCOREDUMP(wstatus) );
        }
        if (pid == supr_child_pid) {
          supr_start_child();
        } else {
          fprintf(stderr, "Strange, pid isn't our main child... doing nothign\n");
        }

    }
}

void supr_read_from_child_io_event(struct epoll_event epe) { int r;
  char buf[1024];
  r = read(supr_read_from_child_fd, buf, sizeof buf - 1);   error_check(r);
  buf[r] = 0;
  printf("read_from_child: `%.*s'\n", r, buf);
}

int main () { int r;
  {
    sigset_t mask;
    r = sigemptyset(&mask);                                       error_check(r);
    r = sigaddset(&mask, SIGCHLD);                                error_check(r);
    r = sigprocmask(SIG_BLOCK, &mask, NULL);                      error_check(r);
    supr_signal_fd = signalfd(-1, &mask, SFD_NONBLOCK | SFD_CLOEXEC);      error_check(supr_signal_fd);

    io_EPData data = {.my_data = { .event_type = _io_socket_type_supr_signal_fd }};
    struct epoll_event epe = {.data = data.data, .events = EPOLLIN};
    r = epoll_ctl(io_epoll_fd, EPOLL_CTL_ADD, supr_signal_fd, &epe); error_check(r);

  }

  {
    int sv[2];
    r = socketpair(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC | SOCK_NONBLOCK, 0, sv); error_check(r);
    supr_read_from_child_fd = sv[0];
    supr_child_write_to_fd  = sv[1];
    r = shutdown(supr_child_write_to_fd, SHUT_RD);      error_check(r);
    r = shutdown(supr_read_from_child_fd, SHUT_WR);     error_check(r);

    io_EPData data = {.my_data = { .event_type = _io_socket_type_supr_read_from_child }};
    struct epoll_event epe = {.data = data.data, .events = EPOLLIN};
    r = epoll_ctl(io_epoll_fd, EPOLL_CTL_ADD, supr_read_from_child_fd, &epe); error_check(r);
  }
  supr_start_child();
  for (;;) { io_process_events(); }
}

