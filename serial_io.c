//#define LOG_DEBUG

#include <unistd.h>

#include "common.h"
#include "logging.h"
#include "io.h"
#include "serial.h"
#include "line_accumulator.h"


int serial_fd = -1;
static struct line_accumulator_Data serial_line_leftovers;

void serial_io_initialize(char* dev_path) {
    serial_fd = serial_open_115200_8n1(dev_path);
    io_ADD_R(serial_fd);
}

IO_EVENT_CALLBACK(serial, events, not_used) {
    char buf[512];
    ssz size = read(serial_fd, buf, sizeof buf);
    DEBUG("read size:%zd", size);
    error_check(size);
    if (size >= 0) {
        line_accumulator(&serial_line_leftovers, buf, size, serial_line_handler);
    }
    if (events & EPOLLERR) {
        ERROR("Serial interface is broken somehow...");
//        ERROR("Serial interface is broken somehow... reinitializing");
        // close(serial_fd);
        // serial_io_initialize('');
    }
}
