#include "common.h"
#include "logging.h"
#include "io.h"
#include "serial.h"


int main (int argc, char ** argv) {
    char * tty_path = *++argv;
    assert(tty_path);

    io_initialize();



}