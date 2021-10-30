

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#include "logging.h"

int main () {
    int console_fd = open("/dev/tty1", O_RDWR);
    error_check(console_fd);

    for (;;) {

        char line[256] = {};
        int r = read(console_fd, line, sizeof line);
        error_check(r);
        INFO_BUFFER(line, sizeof line);

    }

}