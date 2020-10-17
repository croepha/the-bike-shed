#include "common.h"
#include "logging.h"
#include "io.h"
#include "serial.h"


int main (int argc, char ** argv) {
    char * dev_path = *++argv;
    assert(dev_path);

    io_initialize();

    int serial_fd = serial_open_115200_8n1(dev_path);

    io_ADD_R(serial_fd);








}