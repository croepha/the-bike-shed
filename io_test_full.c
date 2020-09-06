

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <fcntl.h>
#include "logging.h"
#include "io.h"

#include "io_test.h"


void test_main() {
    start_time = utc_ms_since_epoch() + 50;
}


int main() { int r;
  setlinebuf(stderr);
  r = alarm(1); error_check(r);

  test_main();

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