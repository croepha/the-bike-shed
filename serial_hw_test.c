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


void logging_send_timeout() {
    INFO();
    dprintf(serial_fd, "TEST TEST\n");
    IO_TIMER_MS(logging_send) = now_sec()*1000 + 1000;
}

int main (int argc, char ** argv) {
    setlinebuf(stderr);
    io_initialize();

    char * dev_path = *++argv;
    assert(dev_path);

    serial_io_initialize(dev_path);

    IO_TIMER_MS(logging_send) = now_sec() + 1000;


    for(;;) {
        log_allowed_fails = 100000000;
        io_process_events();
    }

}