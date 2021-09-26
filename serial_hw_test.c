#include <string.h>
#include <stdio.h>
#include <inttypes.h>
#include "common.h"
#include "logging.h"
#include "io.h"
#include "serial.h"

u64 now_ms() { return real_now_ms(); }


void serial_line_handler(char* line) {
    INFO_BUFFER(line, strlen(line));
}

IO_TIMEOUT_CALLBACK(logging_send) {
    INFO();
    dprintf(serial_fd, "TEST TEST\n");
    IO_TIMER_SET_MS(logging_send, IO_NOW_MS() + 1000);
}

int main (int argc, char ** argv) {
    setlinebuf(stderr);
    io_initialize();

    char * dev_path = *++argv;
    assert(dev_path);

    serial_io_initialize(dev_path);

    IO_TIMER_SET_MS(logging_send, IO_NOW_MS() + 1000);


    for(;;) {
        log_allowed_fails = 100000000;
        io_process_events();
    }

}